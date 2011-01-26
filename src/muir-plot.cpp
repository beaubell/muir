//
// C++ Implementation: muir-plot
//
// Description: Plot MUIR experiment data.
//
//
// Author: Beau V.C. Bellamy <bvbellamy@arsc.edu>
//         Arctic Region Supercomputing Center
// 
//
//

#include "muir-plot.h"
#include "muir-types.h"

#include <gd.h>
#include <gdfontl.h>

#include <iostream>  // std::cout
#include <iomanip>   // std::setprecision()
#include <cassert>

#define QUOTEME_(x) #x
#define QUOTEME(x) QUOTEME_(x)

void save_2dplot(const MuirData &data, const std::string &output_file)
{
    // Get references to data
    const Muir4DArrayF &_sample_data = data.get_sample_data();
    const Muir2DArrayF  &_sample_range = data.get_sample_range();
    const Muir2DArrayUI &_framecount = data.get_framecount();
    const Muir2DArrayD  &_time = data.get_time();
    const std::string   &_filename = data.get_filename();

    // Image and Dataset Variables
    std::size_t delta_t = 1;

    // Get dataset shape
    const Muir4DArrayF::size_type *array_dims = _sample_data.shape();
    assert(_sample_data.num_dimensions() == 4);

    Muir4DArrayF::size_type dataset_count = array_dims[0];  // # sets
    Muir4DArrayF::size_type dataset_width = array_dims[1];  // Rows per set
    Muir4DArrayF::size_type dataset_height = array_dims[2]; // Range bins

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


void save_fftw_2dplot(const MuirData &data, const std::string &output_file)
{
    // Get references to data
    const Muir3DArrayF  &_decoded_data = data.get_decoded_data();
    const Muir2DArrayF  &_sample_range = data.get_sample_range();
    const Muir2DArrayUI &_framecount = data.get_framecount();
    const Muir2DArrayD  &_time = data.get_time();
    const std::string   &_filename = data.get_filename();

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

