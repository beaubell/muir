//
// C++ Implementation: Muir OpenGL data rendering
//
// Description: Loads and renders muir data.
//
//
// Author: Beau V.C. Bellamy <bvbellamy@arsc.edu>
//         Arctic Region Supercomputing Center
//
//
//

#include "muir-gl-data.h"
#include "muir-constants.h"

#include <cstdlib>
#include <iostream>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/convenience.hpp>
namespace FS = boost::filesystem;


Muirgl_Data::Muirgl_Data(FS::path file)
    : texnames(),
    texwidth(0),
    texheight(0),
    dataw(0),
    datah(0),
    radacstart(0.0),
    radacend(0.0),
    file_raw(),
    file_decoded(file),
    sample_range(boost::extents[1][1]),
    framecount(boost::extents[1][1]),
    radac_time(boost::extents[1][2]),
    _sets(0),
    _staged(0)
{

    // Open file
    MuirHD5 h5file( file.string().c_str(), H5F_ACC_RDONLY );

    // Get metadata
    h5file.read_2D_double(RTI_DECODEDRADAC_PATH, radac_time);
    h5file.read_2D_float(RTI_DECODEDRANGE_PATH, sample_range);
    h5file.read_2D_uint(RTI_DECODEDFRAME_PATH, framecount);

    const Muir2DArrayD::size_type *radac_dims = radac_time.shape();
    //const Muir2DArrayF::size_type *range_dims = sample_range.shape();
    //const Muir2DArrayUI::size_type *frame_dims = framecount.shape();
    _sets = radac_dims[0];
    
    radacstart = radac_time[0][0];
    radacend = radac_time[_sets-1][1];
    std::cout << "Radactime: " << radacstart << ":" << radacend << " " << _sets << std::endl;
}

void Muirgl_Data::stage()
{
    if (_staged == 1)
    {
        return;
    }

    std::cout << "Staging: " << file_decoded.string() << std::endl;

    unsigned int width, height;
    GLfloat * data;

    // Open file
    MuirHD5 h5file( file_decoded.string().c_str(), H5F_ACC_RDONLY );

    // Get data
    Muir3DArrayF decoded_data;
    h5file.read_3D_float(RTI_DECODEDDATA_PATH, decoded_data);

    // Get dataset shape
    const Muir3DArrayF::size_type *array_dims = decoded_data.shape();
    assert(decoded_data.num_dimensions() == 3);

    Muir3DArrayF::size_type dataset_count = array_dims[0];  // # sets
    Muir3DArrayF::size_type dataset_width = array_dims[1];  // frames per set
    Muir3DArrayF::size_type dataset_height = array_dims[2]; // Range bins

    width = dataset_width;
    height = dataset_height;

    texnames.resize(_sets);
    glGenTextures( _sets, &texnames[0] );

    for (unsigned int set = 0; set < dataset_count; set++)
    {
        texwidth = width;
        texheight = height;
        dataw = dataset_width;
        datah = dataset_height;

        data = reinterpret_cast<GLfloat*>(malloc( width * height*sizeof(GLfloat) ));

        for (Muir3DArrayF::size_type col = 0; col < width ;col++)
        {
            for (Muir3DArrayF::size_type row = 0; row < height; row++)
            {
                data[row*width + col] = 0.0;
            }
        }

        for (Muir3DArrayF::size_type col = 0; col < dataset_width ;col++)
        {
            for (Muir3DArrayF::size_type row = 0; row < dataset_height; row++)
            {
                //float pixel = log10(decoded_data[set][col][row]+1)*10;
                float pixel = decoded_data[set][col][row];
                data[row*width + col] = pixel;

                //max = std::max(max, pixel);
                //min = std::min(min, pixel);
            }
        }

        //shader_data_max = std::log10(max)*10.0f;
        //shader_data_min = std::log10(min+0.01f)*10.0f;
        //muir_GPU_send_variables();
        //glGenTextures( 1, &texnum );

        // select our current texture
        glBindTexture(GL_TEXTURE_RECTANGLE_ARB, texnames[set] );

        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP);

        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        //In this case, the driver will convert your 32 bit float to 16 bit float
        glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_LUMINANCE32F_ARB, width, height, 0, GL_LUMINANCE, GL_FLOAT, data);

        // free buffer
        free( data );

        //std::cout << "Loaded set: " << file_decoded << ":" << set << "   texnum:" << texnames[set] <<
        //" Glerror?: " << glGetError() << std::endl;
    }

    // Textures are setup!
    _staged = true;
}

void Muirgl_Data::release()
{
    if (_staged == 0)
    {
        return;
    }
    else
    {
        _staged = false;
    }

    // Delete textures!
    glDeleteTextures( _sets, &texnames[0] );

    std::cout << "Release: " << file_decoded.string() << std::endl;
}

void Muirgl_Data::render(const double radac_min,const bool texture_smooth)
{
    if (_staged == 0)
    {
        return;
    }

    for (unsigned int i = 0; i < _sets; i++)
    {
        double x1 = (radac_time[i][0]-radac_min)/10000.0;
        double x2 = (radac_time[i][1]-radac_min)/10000.0;

        glBindTexture( GL_TEXTURE_RECTANGLE_ARB, texnames[i] );
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, texture_smooth?GL_LINEAR:GL_NEAREST);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, texture_smooth?GL_LINEAR:GL_NEAREST);

        glBegin( GL_QUADS );
        glTexCoord2d(0.0  ,0.0  ); glVertex2d(x1,0.0);
        glTexCoord2d(dataw,0.0  ); glVertex2d(x2,0.0);
        glTexCoord2d(dataw,datah); glVertex2d(x2,datah);
        glTexCoord2d(0.0  ,datah); glVertex2d(x1,datah);
        glEnd();
    }
}
