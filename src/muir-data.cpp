//
// C++ Implementation: muir-data
//
// Description: Read and store MUIR experiment data.
//
//
// Author: Beau V.C. Bellamy <bvbellamy@arsc.edu>
//         Arctic Region Supercomputing Center
// 
//
//

#include "muir-data.h"
#include "muir-hd5.h"
#include "muir-constants.h"
#include "muir-utility.h"

#include <iostream>  // std::cout
#include <iomanip>   // std::setprecision()
#include <exception>
#include <stdexcept> // std::runtime_error
#include <cmath>
#include <complex>
#include <fftw3.h>

#include <cassert>


// Constructor
MuirData::MuirData(const std::string &filename_in, int option)
: _filename(filename_in),
  _pulsewidth(0),
  _txbaud(0),
  _phasecode(),
  _sample_data(boost::extents[1][1][1][2]),
  _decoded_data(boost::extents[1][1][1]),
  _sample_range(boost::extents[1][1]),
  _framecount(boost::extents[1][1]),
  _time(boost::extents[1][2])
{
    if (option == 0)
    {
        MuirHD5 file_in(_filename, H5F_ACC_RDONLY);
        // Read Pulsewidth
        _pulsewidth = file_in.read_scalar_float(RTI_RAWPULSEWIDTH_PATH);

        // Read TXBuad
        _txbaud = file_in.read_scalar_float(RTI_RAWTXBAUD_PATH);

        // Read Phasecode
        if (!read_phasecode(file_in, _phasecode))
            std::cout << "File: " << _filename << ", doesn't contain a phase code!" << std::endl;

        // Read in experiment data
        file_in.read_4D_float (RTI_RAWSAMPLEDATA_PATH , _sample_data);
        file_in.read_2D_double(RTI_RADACTIME_PATH     , _time);
        file_in.read_2D_float (RTI_RAWSAMPLERANGE_PATH, _sample_range);
        file_in.read_2D_uint  (RTI_RAWFRAMECOUNT_PATH , _framecount);

    }

    if (option == 1)
    {

        read_decoded_data(filename_in);
    }

}

// Destructor
MuirData::~MuirData()
{

}


void MuirData::print_onesamplecolumn(float (&sample)[1100][2], float (&range)[1100])
{
    std::cout << std::setprecision(6);
    std::cout.setf(std::ios::fixed,std::ios::floatfield);

    for (int k = 0; k < 1100; k++)
        //std::cout << "" << (range[k]/1000) << " " << log10(sqrt(pow(sample[k][0],2) + pow(sample[k][1],2)))*10 << " " << std::endl;
        std::cout << "" << (range[k]/1000) << " " << log10(norm(std::complex<double>(sample[k][0], sample[k][1]))+1)*10-10 << " " << std::endl;
    std::cout << std::endl;
}

/*
void MuirData::print_onesamplecolumn(std::size_t run, std::size_t column)
{
    print_onesamplecolumn((*_sample_data)[run][column], (*_sample_range)[0]);
}
*/

void MuirData::print_stats()
{

    char buf[80];
    time_t then;
    struct tm *ts;

    // Display time (for now)
    for(int i = 0; i < 10; i++)
    {
        for(int k = 0; k < 2; k++)
        {
            then = static_cast<time_t>(_time[i][k]/1000000.0);
            ts = localtime(&then);
            strftime(buf, sizeof(buf), "%a %Y-%m-%d %H:%M:%S %Z", ts);
            //std::cout << _time[i][0] << "," << _time[i][1] << std::endl;
            std::cout << buf << "(" << (_time[i][k]/1000000-then)*1000 << "ms)" << std::endl << (k?"--\n":"");
        }

    }

    std::cout << "Pulsewidth: " << _pulsewidth << std::endl;
    std::cout << "TX Baud   : " << _txbaud << std::endl;
    std::cout << "PW/TXBaud : " << _pulsewidth/_txbaud << std::endl;		

    double pulse_time_sum = 0;

    // Display pulse deltas for each set
    for(int i = 0; i < 10; i++)
    {
        double beg_t = _time[i][0];
        double end_t = _time[i][1];
        double pulse_time_d = (end_t - beg_t)/500;
        pulse_time_sum += pulse_time_d;
        std::cout << "Pulse time delta set"<< i << ": " << pulse_time_d << std::endl;
    }

    double pulse_time_avg = pulse_time_sum /10;
    std::cout << "Pulse time averge: " << pulse_time_avg << std::endl;


    // Display missed pulses
    for(int i = 0; i < 9; i++)
    {
        double end_t = _time[i][1];
        double beg_t = _time[i+1][0];

        double gap_t = beg_t - end_t;

        std::cout << "Time gap between run " << i << " and " << i+1 << ":" << gap_t << "us, Missed Pulses:" << (gap_t)/(pulse_time_avg) << std::endl;

    }

}


void MuirData::process_fftw()
{
    const SampleDataArray::size_type *array_dims = _sample_data.shape();
    assert(_sample_data.num_dimensions() == 4);

    SampleDataArray::size_type max_sets = array_dims[0];
    SampleDataArray::size_type max_cols = array_dims[1];
    SampleDataArray::size_type max_rows = array_dims[2];

    std::size_t phasecode_size = _phasecode.size();

    // Initialize decoded data boost multi_array;
    _decoded_data.resize(boost::extents[max_sets][max_cols][max_rows]);

    #pragma omp critical (fftw)
    {
        fftw_init_threads();
        fftw_plan_with_nthreads(2);
    }

    // Calculate each row
    #pragma omp parallel for
    for(int phase_code_offset = 0; phase_code_offset < static_cast<int>(max_rows); phase_code_offset++)
    {

        // Setup for row
        fftw_complex *in, *out;
        fftw_plan p;
        int fft_size = max_rows;  // Also used for normalization
        int N[1] = {fft_size};

        #pragma omp critical (fftw)
        {
            std::cout << "FFTW Row:" << phase_code_offset << std::endl;
            in  = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * fft_size*max_sets*max_cols);
            out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * fft_size*max_sets*max_cols);
            p = fftw_plan_many_dft(1, N, max_sets*max_cols, in, NULL, 1, fft_size, out, NULL, 1, fft_size, FFTW_FORWARD, FFTW_MEASURE | FFTW_DESTROY_INPUT);
        }

        // Copy data into fftw vector, apply phasecode, and zero out the rest
        for(SampleDataArray::size_type row = 0; row < fft_size; row++)
        {
            if((row >= phase_code_offset) && (row < (phasecode_size + phase_code_offset)))
            {
                float phase_multiplier = _phasecode[row-phase_code_offset];

                for(SampleDataArray::size_type set = 0; set < max_sets; set++)
                    for(SampleDataArray::size_type col = 0; col < max_cols; col++)
                    {
                        SampleDataArray::size_type index = set*(fft_size*max_cols) + col*(fft_size) + row;
                        in[index][0] = _sample_data[set][col][row][0] * phase_multiplier;
                        in[index][1] = _sample_data[set][col][row][1] * phase_multiplier;
                    }
            }
            else // ZEROS!
            {
                for(std::size_t set = 0; set < max_sets; set++)
                    for(std::size_t col = 0; col < max_cols; col++)
                    {
                        std::size_t index = set*(fft_size*max_cols) + col*(fft_size) + row;
                        in[index][0] = 0;
                        in[index][1] = 0;
                    }
            }
        }

        // Execute FFTW
        fftw_execute(p);

        // Output FFTW data
        for(std::size_t set = 0; set < max_sets; set++)
        {
            for(std::size_t col = 0; col < max_cols; col++)
            {
                fftw_complex max_value = {0.0,0.0};
                float        max_power = 0.0;

                // Iterate through the column spectra and find the max value
                for(std::size_t row = 0; row < fft_size; row++)
                {
                    std::size_t index = set*(fft_size*max_cols) + col*(fft_size) + row;

                    float power = norm(std::complex<float>(out[index][0], out[index][1]));
                    if (power > max_power)
                    {
                        max_value[0] = out[index][0];
                        max_value[1] = out[index][1];
                        max_power = power;
                    }

                }

                // Assign and normalize
                _decoded_data[set][col][phase_code_offset] = max_power/fft_size; 
            }
        }

        #pragma omp critical (fftw)
        {
            fftw_destroy_plan(p);
            fftw_free(in);
            fftw_free(out);
        }
    }

    #pragma omp critical (fftw)
    fftw_cleanup_threads();
}

void MuirData::save_decoded_data(const std::string &output_file)
{
    // Open File for Writing
    MuirHD5 h5file( output_file.c_str(), H5F_ACC_TRUNC );

    // Create group
    h5file.createGroup(RTI_DECODEDDIR_PATH);

    // Prepare and write decoded sample data
    h5file.write_3D_float(RTI_DECODEDDATA_PATH, _decoded_data);

    // Prepare and write range data
    h5file.write_2D_float(RTI_DECODEDRANGE_PATH, _sample_range);

    // Prepare and write radac data
    h5file.write_2D_double(RTI_DECODEDRADAC_PATH, _time);

    // Prepare and write framecount data
    h5file.write_2D_uint(RTI_DECODEDFRAME_PATH, _framecount);

    h5file.close();
    return;


}

void MuirData::read_decoded_data(const std::string &input_file)
{
    // Open file
    MuirHD5 h5file( input_file.c_str(), H5F_ACC_RDONLY );

    // Get data
    h5file.read_3D_float(RTI_DECODEDDATA_PATH, _decoded_data);

    // Get range data
    h5file.read_2D_float(RTI_DECODEDRANGE_PATH, _sample_range);

    // Get radac data
    h5file.read_2D_double(RTI_DECODEDRADAC_PATH, _time);

    // Get framecount data
    h5file.read_2D_uint(RTI_DECODEDFRAME_PATH, _framecount);

    // close file
    h5file.close();
    return;
}
