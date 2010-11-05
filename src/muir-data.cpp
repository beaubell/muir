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

#include <gd.h>
#include <gdfontl.h>
#include "H5Cpp.h"

#include <iostream>  // std::cout
#include <iomanip>   // std::setprecision()
#include <exception>
#include <stdexcept> // std::runtime_error
#include <cmath>
#include <complex>
#include <fftw3.h>


#include <cassert>

#define QUOTEME_(x) #x
#define QUOTEME(x) QUOTEME_(x)


// Location Constants
const H5std_string PULSEWIDTH_PATH("/Raw11/Data/Pulsewidth");
const H5std_string BAUDLENGTH_PATH("/Raw11/Data/TxBaud");
const H5std_string EXPERIMENTFILE_PATH("/Setup/Experimentfile");
const H5std_string RADACTIME_PATH("/Time/RadacTime");
const H5std_string SAMPLEDATA_PATH("/Raw11/Data/Samples/Data");
const H5std_string SAMPLERANGE_PATH("/Raw11/Data/Samples/Range");
const H5std_string FRAMECOUNT_PATH("/Raw11/Data/RadacHeader/FrameCount");

const H5std_string DECODEDDATA_PATH ("/Decoded/Data");
const H5std_string DECODEDRANGE_PATH("/Decoded/Range");
const H5std_string DECODEDRADAC_PATH("/Decoded/RadacTime");
const H5std_string DECODEDFRAME_PATH("/Decoded/FrameCount");

// Constructor
MuirData::MuirData(const std::string &filename_in, int option)
: _filename(filename_in),
  _h5file( _filename.c_str(), H5F_ACC_RDONLY ),
  _pulsewidth(0),
  _txbaud(0),
  _phasecode(),
  _sample_data(boost::extents[1][1][1][2]),
  _decoded_data(boost::extents[1][1][1]),
  _sample_range(NULL),
  _framecount(NULL)
{
    if (option == 0)
    {
        MuirHD5 file_in(_filename, H5F_ACC_RDONLY);
    // Read Pulsewidth
    _pulsewidth = file_in.read_scalar_float(PULSEWIDTH_PATH);

    // Read TXBuad
    _txbaud = file_in.read_scalar_float(BAUDLENGTH_PATH);

 //   std::cout << "Pulsewidth: " << _pulsewidth << std::endl;
 //   std::cout << "TX Baud   : " << _txbaud << std::endl;
 //   std::cout << "PW/TXBaud : " << _pulsewidth/_txbaud << std::endl;

    // Read Phasecode and run sanity checks
    read_phasecode(file_in);

 //   std::cout << "Phase Code: ";
 //   for(int i = 0; i < _phasecode.size(); i++)
 //     std::cout << _phasecode[i] << ",";

 //   std::cout << std::endl;
    //std::cout << "Phase Code Len: " << _phasecode.size() << std::endl;

    // Check to see if Pulsewidth/TXBaud equals the amount of phase code parsed.
//    if(_phasecode.size() != _pulsewidth/_txbaud)
//       throw(std::runtime_error(std::string(__FILE__) + ": " + std::string(QUOTEME(__LINE__)) + "  " +
//                                std::string("Phase code parsed doesn't equal Pulsewidth/TXBaud! ")));

    // Read start/stop times.
    read_times();

    // Dynamically allocate data storage arrays
    //_sample_data  = (SampleDataArray) new float[10][500][1100][2];
    
    //_fftw_data    = (FFTWDataArray) new float[10][500][1100][2]; 
    _sample_range = (SampleRangeArray) new float[1][1100];
	_framecount = (FrameCountArray) new float[10][500];

    // Read in sample data
    file_in.read_4D_float(SAMPLEDATA_PATH, _sample_data);

    read_samplerange();
	read_framecount();
    }
	
    if (option == 1)
    {

        // Dynamically allocate data storage arrays
        //_sample_data  = NULL;
        //_fftw_data    = (FFTWDataArray) new float[10][500][1100][2]; 
        _sample_range = (SampleRangeArray) new float[1][1100];
        _framecount = (FrameCountArray) new float[10][500];

        read_decoded_data(filename_in);
    }
    
}

// Destructor
MuirData::~MuirData()
{
    //delete[] _sample_data;
    delete[] _sample_range;
    delete[] _framecount;
    //delete[] _fftw_data;

}


void MuirData::read_phasecode(const MuirHD5 &file_in)
{
    std::string experimentfile = file_in.read_string(EXPERIMENTFILE_PATH);

    // Parse experimentfile...
    std::size_t index_min = experimentfile.find(";Code=");
    std::size_t index_max = experimentfile.find("\n\n[Common Parameters]");

    std::string phasecode_bulk = experimentfile.substr(index_min,index_max-index_min);
    std::size_t size = phasecode_bulk.length();

    _phasecode.clear();

    // Get values for phasecode and dump them into string.
    for (std::size_t i = 0; i < size; i++)
    {
        if (phasecode_bulk[i] == '+')
        {
            _phasecode.push_back(1);
            continue;
        }
 
        if (phasecode_bulk[i] == '-')
        {
            _phasecode.push_back(-1);
            continue;
        }
    }
}


void MuirData::read_times()
{

    // Get Dataset
    const std::string &dataset_name = RADACTIME_PATH;
    H5::DataSet dataset = _h5file.openDataSet( dataset_name );

    // Get Type class
    H5T_class_t type_class = dataset.getTypeClass();

    // Check to see if we are dealing with floats
    if( type_class != H5T_FLOAT )
        throw(std::runtime_error(std::string(__FILE__) + ":" + std::string(QUOTEME(__LINE__)) + "  " +
                std::string("Expecting H5T_FLOAT Type in ") + dataset_name + " from " + _filename));

    // Get size of datatpe and verify
    H5::FloatType floattype = dataset.getFloatType();
    size_t size = floattype.getSize();
    if(size != 8)
        throw(std::runtime_error(std::string(__FILE__) + ":" + std::string(QUOTEME(__LINE__)) + "  " +
                std::string("Expecting float size to be 8 (double) in ") + dataset_name + " from " + _filename));

    // Get dataspace handle
    H5::DataSpace dataspace = dataset.getSpace();
    
    // Get rank and verify
    int rank = dataspace.getSimpleExtentNdims();
    if(rank != 2)
        throw(std::runtime_error(std::string(__FILE__) + ":" + std::string(QUOTEME(__LINE__)) + "  " +
                std::string("Expecting rank to be 2 dimensions in ") + dataset_name + " from " + _filename));

    // Get dimensions and verify
    hsize_t dimsm[2];
    int ndims = dataspace.getSimpleExtentDims( dimsm, NULL);

    assert(rank == ndims == 2);
    
    if(dimsm[0] != 10 || dimsm[1] != 2)
        throw(std::runtime_error(std::string(__FILE__) + ":" + std::string(QUOTEME(__LINE__)) + "  " +
              std::string("Expecting dimensions (10,2) in ") + dataset_name + " from " + _filename));
    
#if 0  //** NOT USED **
                
    H5::DataSpace memspace( rank, dimsm );
    
    /*
    * Define memory hyperslab.   
    */

    hsize_t      offset_out[2];	// hyperslab offset in memory
    hsize_t      count_out[2];	// size of the hyperslab in memory
    offset_out[0] = 0;
    offset_out[1] = 0;
    count_out[0]  = dimsm[0];
    count_out[1]  = dimsm[1];


    memspace.selectHyperslab( H5S_SELECT_SET, count_out, offset_out );

#endif

    for (hsize_t j = 0; j < dimsm[0]; j++)
    {
        for (hsize_t i = 0; i < dimsm[1]; i++)
        {
            _time[j][i] = 0;
        }
    }

#if 0
    // Get attributes
    H5::Attribute myatt_out = dataset.openAttribute("Descriptions");

    H5::StrType strdatatype(H5::PredType::C_S1, myatt_out.getStorageSize());
    std::string strreadbuf;

    myatt_out.read(strdatatype, strreadbuf);
    std::cout << "Description: " << strreadbuf << std::endl;
#endif


    // get data
    dataset.read( _time, H5::PredType::NATIVE_DOUBLE );


    return;
}


void MuirData::read_samplerange()
{

    // Get Dataset
    const std::string &dataset_name = SAMPLERANGE_PATH;
    H5::DataSet dataset = _h5file.openDataSet( dataset_name );

    // Get Type class
    H5T_class_t type_class = dataset.getTypeClass();

    // Check to see if we are dealing with floats
    if( type_class != H5T_FLOAT )
        throw(std::runtime_error(std::string(__FILE__) + ":" + std::string(QUOTEME(__LINE__)) + "  " +
                std::string("Expecting H5T_FLOAT Type in ") + dataset_name + " from " + _filename));

    // Get size of datatpe and verify
    H5::FloatType floattype = dataset.getFloatType();
    size_t size = floattype.getSize();
    if(size != 4)
        throw(std::runtime_error(std::string(__FILE__) + ":" + std::string(QUOTEME(__LINE__)) + "  " +
                std::string("Expecting float size to be 4 (float) in ") + dataset_name + " from " + _filename));

    // Get dataspace handle
    H5::DataSpace dataspace = dataset.getSpace();

    // Get rank and verify
    int rank = dataspace.getSimpleExtentNdims();
    if(rank != 2)
        throw(std::runtime_error(std::string(__FILE__) + ":" + std::string(QUOTEME(__LINE__)) + "  " +
                std::string("Expecting rank to be 2 dimensions in ") + dataset_name + " from " + _filename));

    // Get dimensions and verify
    hsize_t dimsm[2];
    int ndims = dataspace.getSimpleExtentDims( dimsm, NULL);

    if(dimsm[0] != 1 || dimsm[1] != 1100 )
        throw(std::runtime_error(std::string(__FILE__) + ":" + std::string(QUOTEME(__LINE__)) + "  " +
                std::string("Expecting dimensions (1,1100) in ") + dataset_name + " from " + _filename));


    hsize_t i, j;
    for (j = 0; j < dimsm[0]; j++)
    {
        for (i = 0; i < dimsm[1]; i++)
        {
            (*_sample_range)[j][i] = 0;
        }
    }

    // Get data
    dataset.read(_sample_range, H5::PredType::NATIVE_FLOAT);

    return;
}


// READ FRAMECOUNTS
void MuirData::read_framecount()
{
	
    // Get Dataset
    const std::string &dataset_name = FRAMECOUNT_PATH;
    H5::DataSet dataset = _h5file.openDataSet( dataset_name );
	
    // Get Type class
    H5T_class_t type_class = dataset.getTypeClass();
	
    // Check to see if we are dealing with floats
    if( type_class != H5T_INTEGER )
        throw(std::runtime_error(std::string(__FILE__) + ":" + std::string(QUOTEME(__LINE__)) + "  " +
								 std::string("Expecting H5T_INTEGER Type in ") + dataset_name + " from " + _filename));
	
    // Get size of datatpe and verify
    H5::IntType inttype = dataset.getIntType();
    size_t size = inttype.getSize();
    if(size != 4)
        throw(std::runtime_error(std::string(__FILE__) + ":" + std::string(QUOTEME(__LINE__)) + "  " +
								 std::string("Expecting float size to be 4 (integer) in ") + dataset_name + " from " + _filename));
	
    // Get dataspace handle
    H5::DataSpace dataspace = dataset.getSpace();
	
    // Get rank and verify
    int rank = dataspace.getSimpleExtentNdims();
    if(rank != 2)
        throw(std::runtime_error(std::string(__FILE__) + ":" + std::string(QUOTEME(__LINE__)) + "  " +
								 std::string("Expecting rank to be 2 dimensions in ") + dataset_name + " from " + _filename));
	
    // Get dimensions and verify
    hsize_t dimsm[2];
    int ndims = dataspace.getSimpleExtentDims( dimsm, NULL);
	
    if(dimsm[0] != 10 || dimsm[1] != 500 )
        throw(std::runtime_error(std::string(__FILE__) + ":" + std::string(QUOTEME(__LINE__)) + "  " +
								 std::string("Expecting dimensions (10,500) in ") + dataset_name + " from " + _filename));
	
	
    hsize_t i, j;
    for (j = 0; j < dimsm[0]; j++)
    {
        for (i = 0; i < dimsm[1]; i++)
        {
            (*_framecount)[j][i] = 0;
        }
    }
	
    // Get data
    dataset.read(_framecount, H5::PredType::NATIVE_UINT);
	
    return;
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

void MuirData::save_2dplot(const std::string &output_file)
{
    // Image and Dataset Variables
    std::size_t delta_t = 1;

    // Get dataset shape
    const SampleDataArray::size_type *array_dims = _sample_data.shape();
    assert(_sample_data.num_dimensions() == 4);

    SampleDataArray::size_type dataset_count = array_dims[0];  // # sets
    SampleDataArray::size_type dataset_width = array_dims[1];  // Rows per set
    SampleDataArray::size_type dataset_height = array_dims[2]; // Range bins

    std::size_t axis_x_height = 40;
    std::size_t axis_y_width  = 40;
    std::size_t border = 1;
    std::size_t colorbar_width = 60;

    std::size_t start_frame = (*_framecount)[0][0];
    std::size_t end_frame = (*_framecount)[dataset_count-1][dataset_width-1];
    std::size_t num_frames = end_frame - start_frame;

    std::size_t width            = (num_frames)/delta_t+(border*4)+ colorbar_width + axis_y_width;
    std::size_t height           = dataset_height + (2*border) + axis_x_height; 
    //int         bit_depth        = 8;

    // Open File for Writing
    FILE *fp = fopen(output_file.c_str(), "wb");
    if (!fp)
    {
        throw(std::runtime_error(std::string(__FILE__) + ":" + std::string(QUOTEME(__LINE__)) + "  " +
                std::string("Unable to open file for output (") + output_file + ")"));
    }

    // Allocate Image Object
    gdImagePtr im;
    im = gdImageCreate(width, height);

    if (!im)
        throw(std::runtime_error(std::string(__FILE__) + ":" + std::string(QUOTEME(__LINE__)) + "  " +
                std::string("Failed to create GD Image Pointer.")));

    // 
    int black;
    int white;

    unsigned int palette[256];

    for (int i = 0; i < 32;i++) // blk -> blue
    {
        palette[i] = gdImageColorAllocate(im, 0, 0, i*8); 
    }
    black = palette[0];

    for (int i = 0; i < 64;i++) // blue -> light blue
    {
        palette[32 + i] = gdImageColorAllocate(im, 0, i*4, 255); 
    }
    for (int i = 0; i < 64;i++) // light blue -> green
    {
        palette[96 + i] = gdImageColorAllocate(im, 0, 255, 255-i*4); 
    }
    for (int i = 0; i < 64;i++) // green -> yellow
    {
        palette[160 + i] = gdImageColorAllocate(im, i*4, 255, 0);
    }
    for (int i = 0; i < 31;i++) // yellow -> red (one less so we can allocate white)
    {
        palette[224 + i] = gdImageColorAllocate(im, 255-i*2, 255-i*8, 0);
    }

    white = gdImageColorAllocate(im, 255, 255, 255);
    palette[255] = white;

    // Finding maxes and mins
    std::cerr << "Determining data mins and maxes for color pallete" << std::endl;
    float data_min = log10(norm(std::complex<float>(_sample_data[0][0][0][0], _sample_data[0][0][0][1]))+1)*10;
    float data_max = log10(norm(std::complex<float>(_sample_data[0][0][0][0], _sample_data[0][0][0][1]))+1)*10;

    for (SampleDataArray::size_type set = 0; set < dataset_count; set++)
    {
        for (SampleDataArray::size_type col = 0; col < dataset_width; col++)
        {
            for (SampleDataArray::size_type row = 0; row < dataset_height; row++)
            {
                float sample = log10(norm(std::complex<float>(_sample_data[set][col][row][0], _sample_data[set][col][row][1]))+1)*10;
                data_min = std::min(data_min,sample);
                data_max = std::max(data_max,sample);
            }
        }
    }

    std::cerr << "Found MIN/MAX: " << data_min << "," << data_max << std::endl;

    // Write data
    std::cerr << "CREATING IMAGE" << std::endl;

    size_t imageset_width = (dataset_width/delta_t);

    // Signed iteration variable to silence openmp warnings
    #pragma omp parallel for
	for (signed int set = 0; set < static_cast<signed int>(dataset_count); set++)
	{
		std::size_t frameoffset = ((*_framecount)[set][0]-start_frame)/delta_t;
		
    
		for (std::size_t i = 0; i < dataset_height; i++)
		{

            for (std::size_t k = 0; k < imageset_width; k++)
            {

                if (delta_t == 1)
                {
                    float sample = log10(norm(std::complex<float>(_sample_data[set][k][i][0], _sample_data[set][k][i][1]))+1)*10;
                    unsigned char pixel = static_cast<unsigned char>((sample-data_min)/(data_max-data_min)*255);
                    gdImageSetPixel(im, (axis_y_width + border) + frameoffset + k, dataset_height-i, pixel);
                }
                else if (delta_t == 2)
                {
                    float col1 = log10(norm(std::complex<float>(_sample_data[set][delta_t*k][i][0], _sample_data[set][delta_t*k][i][1])))*10;
                    float col2 = log10(norm(std::complex<float>(_sample_data[set][delta_t*k+1][i][0], _sample_data[set][delta_t*k+1][i][1])))*10;
                    unsigned char pixel = static_cast<unsigned char>((((col1 + col2)/2.0)-data_min)/(data_max-data_min)*255);
                    gdImageSetPixel(im, (axis_y_width + border) + frameoffset + k, dataset_height-i, pixel);
                }
                else if (delta_t == 4) // It might be better if this was a loop.....  just a thought.  It does look rather loopworthy...
                {
                    float col1 = log10(norm(std::complex<float>(_sample_data[set][delta_t*k][i][0], _sample_data[set][delta_t*k][i][1])))*10;
                    float col2 = log10(norm(std::complex<float>(_sample_data[set][delta_t*k+1][i][0], _sample_data[set][delta_t*k+1][i][1])))*10;
                    float col3 = log10(norm(std::complex<float>(_sample_data[set][delta_t*k+2][i][0], _sample_data[set][delta_t*k+2][i][1])))*10;
                    float col4 = log10(norm(std::complex<float>(_sample_data[set][delta_t*k+3][i][0], _sample_data[set][delta_t*k+3][i][1])))*10;
                    unsigned char pixel = static_cast<unsigned char>((((col1 + col2 + col3 + col4)/4.0)-data_min)/(data_max-data_min)*255);
                    gdImageSetPixel(im, (axis_y_width + border) + frameoffset + k, dataset_height-i, pixel);
                }
            }

			// Color bar
			for (std::size_t col = 0; col < colorbar_width/3; col++)
				gdImageSetPixel(im, width-1-colorbar_width+col, dataset_height-i, static_cast<unsigned char>((float(i)/dataset_height)*(255.0)));

		}

        //if (!(i%10))
            std::cout << "SET: " << set << std::endl;


    }

    // Fontness
    gdFontPtr font = gdFontGetLarge();

    // Bottom Axis
    std::size_t xaxis_offset = (axis_y_width + border);
    std::size_t xaxis_end = num_frames/delta_t;
    std::size_t xaxis_height1 = axis_x_height / 4;
    std::size_t xaxis_height2 = axis_x_height / 8;
    std::size_t xaxis_yoffset = dataset_height+border*2;

    gdImageFilledRectangle(im, 0, xaxis_yoffset, width, height, white);
    gdImageFilledRectangle(im, 0, 0, axis_y_width-border, height, white);
    for (std::size_t x = 0; x < xaxis_end; x = x + 5)
    {
        gdImageLine(im, xaxis_offset + x, xaxis_yoffset, xaxis_offset + x, (xaxis_yoffset + ((x%10)?(xaxis_height2):xaxis_height1)), black); 
    }
    for (std::size_t set = 0; set < dataset_count; set++)
    {
        std::size_t frameoffset = ((*_framecount)[set][0]-start_frame)/delta_t + axis_y_width + border;
        gdImageLine(im, frameoffset, xaxis_yoffset, frameoffset, xaxis_yoffset + xaxis_height1*2 , black);
        
        char buf1[80];
        char buf2[80];
        time_t then;
        struct tm *ts;
        then = static_cast<time_t>(_time[set][0]/1000000.0);
        ts = gmtime(&then);
        //strftime(buf, sizeof(buf), "%a %Y-%m-%d %H:%M:%S %Z", ts);
        strftime(buf1, sizeof(buf1), "%H:%M:%S", ts);
        sprintf(buf2,"%s:%06.3f",buf1, (_time[set][0]/1000000-then)*1000);
        gdImageString(im, font, frameoffset + 2, xaxis_yoffset + xaxis_height1 + 2, reinterpret_cast<unsigned char*>(buf2), black);
        //gdImageString(im, font, frameoffset + 2, xaxis_yoffset + xaxis_height1 + 2, reinterpret_cast<unsigned char*>(buf), black);
    }


    // Print date
    {
    char gmtbuf[80];
    char lclbuf[80];
    char buf2[200];
    time_t then;
    struct tm *ts;
    then = static_cast<time_t>(_time[0][0]/1000000.0);
    ts = gmtime(&then);
    strftime(gmtbuf, sizeof(gmtbuf), "%a %Y-%m-%d %H:%M:%S %Z", ts);
    ts = localtime(&then);
    strftime(lclbuf, sizeof(lclbuf), "%a %Y-%m-%d %H:%M:%S %Z", ts);
    sprintf(buf2,"Experiment Date: %s (%s)   File: %s   Column Width: %lu Frame(s)", gmtbuf, lclbuf,  _filename.c_str(), static_cast<unsigned long>(delta_t));
    gdImageString(im, font, axis_y_width + border, xaxis_yoffset + xaxis_height1*2 + 5, reinterpret_cast<unsigned char*>(buf2), black);
    }

    // Left Axis
    for (std::size_t y = 50; y <= dataset_height; y = y + 80)
    {
        gdImageLine(im, axis_y_width/2, y, axis_y_width, y, black);
        char buf[80];
        sprintf(buf,"%-3.1fkm",((*_sample_range)[0][1100-y])/1000);
        gdImageStringUp(im, font, 5, y+border+20, reinterpret_cast<unsigned char*>(buf), black);
        //std::cout << y << ":" << 1099-y << "-" << (*_sample_range)[0][1099-y]/1000 << " " << std::endl;
    }

    // Color Bar Text
    gdImageFilledRectangle(im, width-colorbar_width+colorbar_width/3, 0, width, height, white);
    for (std::size_t y = 50; y <= dataset_height; y = y + 80)
    {
        gdImageLine(im, width-1-colorbar_width+colorbar_width/3, y, width-1-colorbar_width+colorbar_width/2, y, black);
        char buf[80];
        sprintf(buf,"%-3.1fdb",(static_cast<float>(dataset_height-(y-border))/static_cast<float>(dataset_height)*(data_max-data_min))+data_min);
        gdImageStringUp(im, font, width-20, y+border+20, reinterpret_cast<unsigned char*>(buf), black);
        //std::cout << y << ":" << 1099-y << "-" << (*_sample_range)[0][1099-y]/1000 << " " << std::endl;
    }

    // Done with file
    std::cerr << "WRITING IMAGE " << std::endl;
    gdImagePngEx(im, fp, 9);
    gdImageDestroy(im);
    fclose(fp);

}


void MuirData::save_fftw_2dplot(const std::string &output_file)
{
    // Image and Dataset Variables
    std::size_t delta_t = 1;

    // Get dataset shape
    const DecodedDataArray::size_type *array_dims = _decoded_data.shape();
    assert(_decoded_data.num_dimensions() == 3);

    DecodedDataArray::size_type dataset_count = array_dims[0];  // # sets
    DecodedDataArray::size_type dataset_width = array_dims[1];  // Rows per set
    DecodedDataArray::size_type dataset_height = array_dims[2]; // Range bins

    std::size_t axis_x_height = 40;
    std::size_t axis_y_width  = 40;
    std::size_t border = 1;
    std::size_t colorbar_width = 60;

    std::size_t start_frame = (*_framecount)[0][0];
    std::size_t end_frame = (*_framecount)[dataset_count-1][dataset_width-1];
    std::size_t num_frames = end_frame - start_frame;

    std::size_t width            = (num_frames)/delta_t+(border*4)+ colorbar_width + axis_y_width;
    std::size_t height           = dataset_height + (2*border) + axis_x_height; 
    //int         bit_depth        = 8;

    // Open File for Writing
    FILE *fp = fopen(output_file.c_str(), "wb");
    if (!fp)
    {
        throw(std::runtime_error(std::string(__FILE__) + ":" + std::string(QUOTEME(__LINE__)) + "  " +
                std::string("Unable to open file for output (") + output_file + ")"));
    }

    // Allocate Image Object
    gdImagePtr im;
    im = gdImageCreate(width, height);

    if (!im)
        throw(std::runtime_error(std::string(__FILE__) + ":" + std::string(QUOTEME(__LINE__)) + "  " +
                std::string("Failed to create GD Image Pointer.")));

    // Initialize color pallete
    int black;
    int white;

    unsigned int palette[256];

    palette[0] = gdImageColorAllocate(im, 0, 0, 0); 
    black = palette[0];

    for (int i = 1; i < 85;i++) // blue -> red
    {
        palette[i] = gdImageColorAllocate(im, 0, 0, i*3); 
    }
    for (int i = 0; i < 85;i++) // blue -> red
    {
        palette[85+i] = gdImageColorAllocate(im, 0, i*3, 255-i*3); 
    }
    for (int i = 0; i < 85;i++) // blue -> red
    {
        palette[170+i] = gdImageColorAllocate(im, i*3, 255-i*3,0 ); 
    }
    white = gdImageColorAllocate(im, 255, 255, 255);
    palette[255] = white;

    // Finding maxes and mins
    std::cerr << "Determining data mins and maxes for color pallete" << std::endl;
    float data_min = log10(_decoded_data[0][0][0]+1)*10;
    float data_max = log10(_decoded_data[0][0][0]+1)*10;

    for (SampleDataArray::size_type set = 0; set < dataset_count; set++)
    {
        for (SampleDataArray::size_type col = 0; col < dataset_width; col++)
        {
            for (SampleDataArray::size_type row = 0; row < dataset_height; row++)
            {
                float sample = log10(_decoded_data[set][col][row]+1)*10;
                data_min = std::min(data_min,sample);
                data_max = std::max(data_max,sample);
            }
        }
    }

    std::cerr << "Found MIN/MAX: " << data_min << "," << data_max << std::endl;

    // Write data
    std::cerr << "CREATING IMAGE" << std::endl;

    size_t imageset_width = (dataset_width/delta_t);

    // Signed iteration variable to silence openmp warnings
    #pragma omp parallel for
    for (signed int set = 0; set < static_cast<signed int>(dataset_count); set++)
	{
		std::size_t frameoffset = ((*_framecount)[set][0]-start_frame)/delta_t;

		for (unsigned int i = 0; i < dataset_height ;i++)
		{

            for (std::size_t k = 0; k < imageset_width; k++)
            {

                if (delta_t == 1)
                {
                    float sample = log10(_decoded_data[set][k][i]+1)*10;
                    unsigned char pixel = static_cast<unsigned char>((sample-data_min)/(data_max-data_min)*255);
                    gdImageSetPixel(im, (axis_y_width + border) + frameoffset + k, dataset_height-i, pixel);
                }
                else if (delta_t == 2)
                {
                    float col1 = log10(_decoded_data[set][delta_t*k][i]+1)*10;
                    float col2 = log10(_decoded_data[set][delta_t*k+1][i]+1)*10;
                    unsigned char pixel = static_cast<unsigned char>((((col1 + col2)/2.0)-data_min)/(data_max-data_min)*255);
                    gdImageSetPixel(im, (axis_y_width + border) + frameoffset + k, dataset_height-i, pixel);
                }
                else if (delta_t == 4) // It might be better if this was a loop.....  just a thought.  It does look rather loopworthy...
                {
                    float col1 = log10(_decoded_data[set][delta_t*k][i]+1)*10;
                    float col2 = log10(_decoded_data[set][delta_t*k+1][i]+1)*10;
                    float col3 = log10(_decoded_data[set][delta_t*k+2][i]+1)*10;
                    float col4 = log10(_decoded_data[set][delta_t*k+3][i]+1)*10;
                    unsigned char pixel = static_cast<unsigned char>((((col1 + col2 + col3 + col4)/4.0)-data_min)/(data_max-data_min)*255);
                    gdImageSetPixel(im, (axis_y_width + border) + frameoffset + k, dataset_height-i, pixel);
                }
            }
			// Color bar
			for (std::size_t col = 0; col < colorbar_width/3; col++)
				gdImageSetPixel(im, width-1-colorbar_width+col, dataset_height-i, static_cast<unsigned char>((float(i)/dataset_height)*(255.0)));

		}

        //if (!(i%10))
            std::cout << "SET: " << set << std::endl;

    }

    // Fontness
    gdFontPtr font = gdFontGetLarge();

    // Bottom Axis
    std::size_t xaxis_offset = (axis_y_width + border);
    std::size_t xaxis_end = num_frames/delta_t;
    std::size_t xaxis_height1 = axis_x_height / 4;
    std::size_t xaxis_height2 = axis_x_height / 8;
    std::size_t xaxis_yoffset = dataset_height+border*2;

    gdImageFilledRectangle(im, 0, xaxis_yoffset, width, height, white);
    gdImageFilledRectangle(im, 0, 0, axis_y_width-border, height, white);
    for (std::size_t x = 0; x < xaxis_end; x = x + 5)
    {
        gdImageLine(im, xaxis_offset + x, xaxis_yoffset, xaxis_offset + x, (xaxis_yoffset + ((x%10)?(xaxis_height2):xaxis_height1)), black); 
    }
    for (std::size_t set = 0; set < dataset_count; set++)
    {
        std::size_t frameoffset = ((*_framecount)[set][0]-start_frame)/delta_t + axis_y_width + border;
        gdImageLine(im, frameoffset, xaxis_yoffset, frameoffset, xaxis_yoffset + xaxis_height1*2 , black);
        
        char buf1[80];
        char buf2[80];
        time_t then;
        struct tm *ts;
        then = static_cast<time_t>(_time[set][0]/1000000.0);
        ts = gmtime(&then);
        //strftime(buf, sizeof(buf), "%a %Y-%m-%d %H:%M:%S %Z", ts);
        strftime(buf1, sizeof(buf1), "%H:%M:%S", ts);
        sprintf(buf2,"%s:%06.3f",buf1, (_time[set][0]/1000000-then)*1000);
        gdImageString(im, font, frameoffset + 2, xaxis_yoffset + xaxis_height1 + 2, reinterpret_cast<unsigned char*>(buf2), black);
        //gdImageString(im, font, frameoffset + 2, xaxis_yoffset + xaxis_height1 + 2, reinterpret_cast<unsigned char*>(buf), black);
    }


    // Print date
    {
    char gmtbuf[80];
    char lclbuf[80];
    char buf2[200];
    time_t then;
    struct tm *ts;
    then = static_cast<time_t>(_time[0][0]/1000000.0);
    ts = gmtime(&then);
    strftime(gmtbuf, sizeof(gmtbuf), "%a %Y-%m-%d %H:%M:%S %Z", ts);
    ts = localtime(&then);
    strftime(lclbuf, sizeof(lclbuf), "%a %Y-%m-%d %H:%M:%S %Z", ts);
		sprintf(buf2,"Experiment Date: %s (%s)   File: %s   Column Width: %lu Frame(s)", gmtbuf, lclbuf,  _filename.c_str(), static_cast<unsigned long>(delta_t));
    gdImageString(im, font, axis_y_width + border, xaxis_yoffset + xaxis_height1*2 + 5, reinterpret_cast<unsigned char*>(buf2), black);
    }

    // Left Axis
    for (std::size_t y = 50; y <= dataset_height; y = y + 80)
    {
        gdImageLine(im, axis_y_width/2, y, axis_y_width, y, black);
        char buf[80];
        sprintf(buf,"%-3.1fkm",((*_sample_range)[0][1100-y])/1000);
        gdImageStringUp(im, font, 5, y+border+20, reinterpret_cast<unsigned char*>(buf), black);
        //std::cout << y << ":" << 1099-y << "-" << (*_sample_range)[0][1099-y]/1000 << " " << std::endl;
    }

    // Color Bar Text
    gdImageFilledRectangle(im, width-colorbar_width+colorbar_width/3, 0, width, height, white);
    for (std::size_t y = 50; y <= dataset_height; y = y + 80)
    {
        gdImageLine(im, width-1-colorbar_width+colorbar_width/3, y, width-1-colorbar_width+colorbar_width/2, y, black);
        char buf[80];
        sprintf(buf,"%-3.1fdb",(static_cast<float>(dataset_height-(y-border))/static_cast<float>(dataset_height)*(data_max-data_min))+data_min);
        gdImageStringUp(im, font, width-20, y+border+20, reinterpret_cast<unsigned char*>(buf), black);
        //std::cout << y << ":" << 1099-y << "-" << (*_sample_range)[0][1099-y]/1000 << " " << std::endl;
    }

    // Done with file
    std::cerr << "WRITING IMAGE " << std::endl;
    gdImagePngEx(im, fp, 9);
    gdImageDestroy(im);
    fclose(fp);

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

    int N[1] = {max_rows};

    // Calculate each row
    #pragma omp parallel for
    for(int phase_code_offset = 0; phase_code_offset < static_cast<int>(max_rows); phase_code_offset++)
    {

        // Setup for row
        fftw_complex *in, *out;
        fftw_plan p;

        #pragma omp critical (fftw)
        {
            std::cout << "FFTW Row:" << phase_code_offset << std::endl;
            in  = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * max_rows*max_sets*max_cols);
            out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * max_rows*max_sets*max_cols);
            p = fftw_plan_many_dft(1, N, max_sets*max_cols, in, NULL, 1, max_rows, out, NULL, 1, max_rows, FFTW_FORWARD, FFTW_MEASURE | FFTW_DESTROY_INPUT);
        }

        // Copy data into fftw vector, apply phasecode, and zero out the rest
        for(SampleDataArray::size_type row = 0; row < max_rows; row++)
        {
            if((row >= phase_code_offset) && (row < (phasecode_size + phase_code_offset)))
            {
                float phase_multiplier = _phasecode[row-phase_code_offset];

                for(SampleDataArray::size_type set = 0; set < max_sets; set++)
                    for(SampleDataArray::size_type col = 0; col < max_cols; col++)
                    {
                        SampleDataArray::size_type index = set*(max_rows*max_cols) + col*(max_rows) + row;
                        in[index][0] = _sample_data[set][col][row][0] * phase_multiplier;
                        in[index][1] = _sample_data[set][col][row][1] * phase_multiplier;
                    }
            }
            else // ZEROS!
            {
                for(std::size_t set = 0; set < max_sets; set++)
                    for(std::size_t col = 0; col < max_cols; col++)
                    {
                        std::size_t index = set*(max_rows*max_cols) + col*(max_rows) + row;
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
                for(std::size_t row = 0; row < max_rows; row++)
                {
                    std::size_t index = set*(max_rows*max_cols) + col*(max_rows) + row;

                    float power = norm(std::complex<float>(out[index][0], out[index][1]));
                    if (power > max_power)
                    {
                        max_value[0] = out[index][0];
                        max_value[1] = out[index][1];
                        max_power = power;
                    }

                }

                //(*_fftw_data)[set][col][phase_code_offset][0] = max_value[0];
                //(*_fftw_data)[set][col][phase_code_offset][1] = max_value[1];
                _decoded_data[set][col][phase_code_offset] = max_power;
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
    const H5std_string GROUP_PATH("/Decoded");

    // Open File for Writing
    H5::H5File h5file( output_file.c_str(), H5F_ACC_TRUNC );

    // Create group
    h5file.createGroup(GROUP_PATH);

    /// Prepare and write decoded sample data
    {
        // Specify Dimensions
        hsize_t rank = 3;
        hsize_t dimsf[rank];
        dimsf[0] = 10;
        dimsf[1] = 500;
        dimsf[2] = 1100;
        //dimsf[3] = 2;
    
        // Create dataspace
        H5::DataSpace dataspace( rank, dimsf );
    
        // Define Datatype
        H5::FloatType datatype( H5::PredType::NATIVE_FLOAT );
        datatype.setOrder( H5T_ORDER_LE);
    
        // Create a new dataset within the file...
        H5::DataSet dataset = h5file.createDataSet( DECODEDDATA_PATH, datatype, dataspace);
    
        // Write data
        dataset.write(_decoded_data.data(), H5::PredType::NATIVE_FLOAT);
    }
    
    /// Prepare and write range data
    {
        // Specify Dimensions
        hsize_t rank = 2;
        hsize_t dimsf[rank];
        dimsf[0] = 1;
        dimsf[1] = 1100;
    
        // Create dataspace
        H5::DataSpace dataspace( rank, dimsf );
    
        // Define Datatype
        H5::FloatType datatype( H5::PredType::NATIVE_FLOAT );
        datatype.setOrder( H5T_ORDER_LE);;
    
        // Create a new dataset within the file...
        H5::DataSet dataset = h5file.createDataSet( DECODEDRANGE_PATH, datatype, dataspace);
    
        // Write data
        dataset.write(_sample_range, H5::PredType::NATIVE_FLOAT);
    }
    
    /// Prepare and write radac data
    {
        // Specify Dimensions
        hsize_t rank = 2;
        hsize_t dimsf[rank];
        dimsf[0] = 10;
        dimsf[1] = 2;
    
        // Create dataspace
        H5::DataSpace dataspace( rank, dimsf );
    
        // Define Datatype
        H5::FloatType datatype( H5::PredType::NATIVE_DOUBLE );
        datatype.setOrder( H5T_ORDER_LE);;
    
        // Create a new dataset within the file...
        H5::DataSet dataset = h5file.createDataSet( DECODEDRADAC_PATH, datatype, dataspace);
    
        // Write data
        dataset.write(_time, H5::PredType::NATIVE_DOUBLE);
    }
    
    /// Prepare and write framecount data
    {
        // Specify Dimensions
        hsize_t rank = 2;
        hsize_t dimsf[rank];
        dimsf[0] = 10;
        dimsf[1] = 500;
    
        // Create dataspace
        H5::DataSpace dataspace( rank, dimsf );
    
        // Define Datatype
        H5::IntType datatype( H5::PredType::NATIVE_UINT );
        datatype.setOrder( H5T_ORDER_LE);;
    
        // Create a new dataset within the file...
        H5::DataSet dataset = h5file.createDataSet( DECODEDFRAME_PATH, datatype, dataspace);
    
        // Write data
        dataset.write(_framecount, H5::PredType::NATIVE_UINT);
    }
    h5file.close();
    return;


}

void MuirData::read_decoded_data(const std::string &input_file)
{
    // Open file
    H5::H5File h5file( input_file.c_str(), H5F_ACC_RDONLY );

    // Get Dataset
    const std::string &dataset_name = DECODEDDATA_PATH;
    H5::DataSet dataset = h5file.openDataSet( dataset_name );

    // Get Type classype::NATIVE_FLOAT
    H5T_class_t type_class = dataset.getTypeClass();

    // Check to see if we are dealing with floats
    if( type_class != H5T_FLOAT )
        throw(std::runtime_error(std::string(__FILE__) + ":" + std::string(QUOTEME(__LINE__)) + "  " +
                std::string("Expecting H5T_FLOAT Type in ") + dataset_name + " from " + _filename));

    // Get size of datatpe and verify
    H5::FloatType floattype = dataset.getFloatType();
    size_t size = floattype.getSize();
    if(size != 4)
        throw(std::runtime_error(std::string(__FILE__) + ":" + std::string(QUOTEME(__LINE__)) + "  " +
                std::string("Expecting float size to be 4 (float) in ") + dataset_name + " from " + _filename));

    // Get dataspace handle
    H5::DataSpace dataspace = dataset.getSpace();

    // Get rank and verify
    int rank = dataspace.getSimpleExtentNdims();
    if(rank != 3)
        throw(std::runtime_error(std::string(__FILE__) + ":" + std::string(QUOTEME(__LINE__)) + "  " +
                std::string("Expecting rank to be 3 dimensions in ") + dataset_name + " from " + _filename));

    // Get dimensions and verify
    hsize_t dimsm[rank];
    dataspace.getSimpleExtentDims( dimsm, NULL);

    if(dimsm[0] != 10 || dimsm[1] != 500 || dimsm[2] != 1100 )
        throw(std::runtime_error(std::string(__FILE__) + ":" + std::string(QUOTEME(__LINE__)) + "  " +
                std::string("Expecting dimensions (10,500,1100) in ") + dataset_name + " from " + _filename));

    // Initialize decoded boost multi_array;
    _decoded_data.resize(boost::extents[dimsm[0]][dimsm[1]][dimsm[2]]);

    hsize_t i, j, k;

    for (j = 0; j < dimsm[0]; j++)
    {
        for (i = 0; i < dimsm[1]; i++)
        {
            for (k = 0; k < dimsm[2]; k++)
                _decoded_data[j][i][k] = 0;
        }
    }

    // Get data
    dataset.read(_decoded_data.data(), H5::PredType::NATIVE_FLOAT);

    h5file.close();
    return;
}
