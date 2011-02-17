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
#include "muir-process.h"

#include <iostream>  // std::cout
#include <iomanip>   // std::setprecision()
#include <exception>
#include <stdexcept> // std::runtime_error
#include <cmath>
#include <complex>


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

int MuirData::decode(int id)
{
    // Use OpenCL Decoding
    int err = process_data(_sample_data, _phasecode, _decoded_data);
    
    return err;
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
