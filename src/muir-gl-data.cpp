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
    _staged(0),
    type(FILE_DECODED)
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

    // Determine File Type
    try {
        h5file.openDataSet(RTI_DECODEDDATA_PATH);
    }
    catch(...)
    {
        type = FILE_COMPLEX;
    }
}

void Muirgl_Data::load3D(const MuirHD5 &file, const std::string& path)
{
    unsigned int width, height;
    GLfloat * data;

    Muir3DArrayF decoded_data;
    file.read_3D_float(path, decoded_data);

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

}

void Muirgl_Data::load4D(const MuirHD5 &file, const std::string& path)
{
    unsigned int width, height;
    GLfloat * data;
    
    Muir4DArrayF decoded_data;
    file.read_4D_float(path, decoded_data);
    
    // Get dataset shape
    const Muir4DArrayF::size_type *array_dims = decoded_data.shape();
    assert(decoded_data.num_dimensions() == 4);
    
    Muir4DArrayF::size_type dataset_count = array_dims[0];  // # sets
    Muir4DArrayF::size_type dataset_width = array_dims[1];  // frames per set
    Muir4DArrayF::size_type dataset_height = array_dims[2]; // Range bins
    
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
                float pixel = sqrtf(powf(decoded_data[set][col][row][0],2) + powf(decoded_data[set][col][row][1],2));
                data[row*width + col] = pixel;
                
                //max = std::max(max, pixel);
                //min = std::min(min, pixel);
            }
        }
        
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


}

void Muirgl_Data::stage()
{
    if (_staged == 1)
    {
        return;
    }

    std::cout << "Staging: " << file_decoded.string() << std::endl;


    // Open file
    MuirHD5 h5file( file_decoded.string().c_str(), H5F_ACC_RDONLY );

    // Get data
    if(type == FILE_DECODED)
        load3D(h5file, RTI_DECODEDDATA_PATH);
    else
        load4D(h5file, std::string("/Decoded/Intermediate/Diff"));

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
        double x1 = (radac_time[i][0]-radac_min);
        double x2 = (radac_time[i][1]-radac_min);

        glBindTexture( GL_TEXTURE_RECTANGLE_ARB, texnames[i] );
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, texture_smooth?GL_LINEAR:GL_NEAREST);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, texture_smooth?GL_LINEAR:GL_NEAREST);

        double range_min = sample_range[0][0];
        double range_max = sample_range[0][datah-1];
        
        glBegin( GL_QUADS );
        glTexCoord2d(0.0  ,0.0  ); glVertex2d(x1,range_min);
        glTexCoord2d(dataw,0.0  ); glVertex2d(x2,range_min);
        glTexCoord2d(dataw,datah); glVertex2d(x2,range_max);
        glTexCoord2d(0.0  ,datah); glVertex2d(x1,range_max);
        glEnd();
    }
}
