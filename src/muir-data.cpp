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

#include <gd.h>
#include <gdfontl.h>

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

    std::size_t start_frame = _framecount[0][0];
    std::size_t end_frame = _framecount[dataset_count-1][dataset_width-1];
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
        std::size_t frameoffset = (_framecount[set][0]-start_frame)/delta_t;


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
        std::size_t frameoffset = (_framecount[set][0]-start_frame)/delta_t + axis_y_width + border;
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
        sprintf(buf,"%-3.1fkm",(_sample_range[0][1100-y])/1000);
        gdImageStringUp(im, font, 5, y+border+20, reinterpret_cast<unsigned char*>(buf), black);
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

    std::size_t start_frame = _framecount[0][0];
    std::size_t end_frame = _framecount[dataset_count-1][dataset_width-1];
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
        std::size_t frameoffset = (_framecount[set][0]-start_frame)/delta_t;

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
        std::size_t frameoffset = (_framecount[set][0]-start_frame)/delta_t + axis_y_width + border;
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
        sprintf(buf,"%-3.1fkm",(_sample_range[0][1100-y])/1000);
        gdImageStringUp(im, font, 5, y+border+20, reinterpret_cast<unsigned char*>(buf), black);
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
