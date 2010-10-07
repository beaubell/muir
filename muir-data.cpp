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

#include <png.h>
#include "H5Cpp.h"

#include <iostream>  // std::cout
#include <iomanip>   // std::setprecision()
#include <exception>
#include <stdexcept> // std::runtime_error
#include <cmath>
#include <complex>

#define QUOTEME_(x) #x
#define QUOTEME(x) QUOTEME_(x)


// Location Constants
const H5std_string PULSEWIDTH_PATH("/Raw11/Data/Pulsewidth");
const H5std_string BAUDLENGTH_PATH("/Raw11/Data/TxBaud");
const H5std_string EXPERIMENTFILE_PATH("/Setup/Experimentfile");
const H5std_string RADACTIME_PATH("/Time/RadacTime");
const H5std_string SAMPLEDATA_PATH("/Raw11/Data/Samples/Data");
const H5std_string SAMPLERANGE_PATH("/Raw11/Data/Samples/Range");


// Constructor
MuirData::MuirData(const std::string &filename_in)
: _filename(filename_in),
  _h5file( _filename.c_str(), H5F_ACC_RDONLY ),
  _pulsewidth(0),
  _txbaud(0)
{
    // Read Pulsewidth
    _pulsewidth = read_scalar_float(PULSEWIDTH_PATH);

    // Read TXBuad
    _txbaud = read_scalar_float(BAUDLENGTH_PATH);

 //   std::cout << "Pulsewidth: " << _pulsewidth << std::endl;
 //   std::cout << "TX Baud   : " << _txbaud << std::endl;
 //   std::cout << "PW/TXBaud : " << _pulsewidth/_txbaud << std::endl;

    // Read Phasecode and run sanity checks
//    read_phasecode();

 //   std::cout << "Phase Code: ";
 //   for(int i = 0; i < _phasecode.size(); i++)
 //     std::cout << _phasecode[i] << ",";

 //   std::cout << std::endl;
 //   std::cout << "Phase Code Len: " << _phasecode.size() << std::endl;

    // Check to see if Pulsewidth/TXBaud equals the amount of phase code parsed.
//    if(_phasecode.size() != _pulsewidth/_txbaud)
//       throw(std::runtime_error(std::string(__FILE__) + ": " + std::string(QUOTEME(__LINE__)) + "  " +
//                                std::string("Phase code parsed doesn't equal Pulsewidth/TXBaud! ")));

    // Read start/stop times.
    read_times();

    // Dynamically allocate data storage arrays
    _sample_data  = (SampleDataArray) new float[10][500][1100][2]; 
    _sample_range = (SampleRangeArray) new float[1][1100];


    read_sampledata();
    read_samplerange();

}

// Destructor
MuirData::~MuirData()
{
    delete[] (*_sample_data);
    delete[] (*_sample_range);

}


float MuirData::read_scalar_float(const H5std_string &dataset_name)
{

   float data_out[1] = {0.0f};

   // Get dataset
   H5::DataSet dataset = _h5file.openDataSet( dataset_name );

   // Get Type class
   H5T_class_t type_class = dataset.getTypeClass();

   // Check to see if we are dealing with floats
   if( type_class != H5T_FLOAT )
      throw(std::runtime_error(std::string(__FILE__) + ":" + std::string(QUOTEME(__LINE__)) + "  " +
                               std::string("Expecting FLOAT Type from ") + dataset_name + " in " + _filename));

   // Read scalar float
   dataset.read(data_out, H5::PredType::NATIVE_FLOAT);

   return data_out[0];
}


std::string MuirData::read_string(const H5std_string &dataset_name)
{

   H5std_string buffer("");

   // Get dataset
   H5::DataSet dataset = _h5file.openDataSet( dataset_name );

   // Get Type class
   H5T_class_t type_class = dataset.getTypeClass();

   // Check to see if we are dealing with floats
   if( type_class != H5T_STRING )
      throw(std::runtime_error(std::string(__FILE__) + ":" + std::string(QUOTEME(__LINE__)) + "  " +
                               std::string("Expecting STRING Type from ") + dataset_name + " in " + _filename));

   H5::DataType dtype = dataset.getDataType();

   //hsize_t size = dataset.getStorageSize ();
   //std::cout << "Storage size required: " << size << std::endl;

   // Read string
   dataset.read( buffer, dtype);

   return buffer;
}


void MuirData::read_phasecode()
{
    std::string experimentfile = read_string(EXPERIMENTFILE_PATH);

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

    for (int j = 0; j < dimsm[0]; j++)
    {
        for (int i = 0; i < dimsm[1]; i++)
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


void MuirData::read_sampledata()
{

    // Get Dataset
    const std::string &dataset_name = SAMPLEDATA_PATH;
    H5::DataSet dataset = _h5file.openDataSet( dataset_name );

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
    if(rank != 4)
        throw(std::runtime_error(std::string(__FILE__) + ":" + std::string(QUOTEME(__LINE__)) + "  " +
              std::string("Expecting rank to be 4 dimensions in ") + dataset_name + " from " + _filename));

    // Get dimensions and verify
    hsize_t dimsm[4];
    int ndims = dataspace.getSimpleExtentDims( dimsm, NULL);

    if(dimsm[0] != 10 || dimsm[1] != 500 || dimsm[2] != 1100 || dimsm[3] != 2)
        throw(std::runtime_error(std::string(__FILE__) + ":" + std::string(QUOTEME(__LINE__)) + "  " +
              std::string("Expecting dimensions (10,500,1100,2) in ") + dataset_name + " from " + _filename));


#if 0  // memspaces and hyperslabs are not used
    H5::DataSpace memspace( rank, dimsm );

    /*
    * Define memory hyperslab.
    */
    hsize_t      offset_out[4];	// hyperslab offset in memory
    hsize_t      count_out[4];	// size of the hyperslab in memory
    offset_out[0] = 0;
    offset_out[1] = 0;
    offset_out[2] = 0;
    offset_out[3] = 0;
    count_out[0]  = dimsm[0];
    count_out[1]  = dimsm[1];
    count_out[2]  = dimsm[2];
    count_out[3]  = dimsm[3];

    memspace.selectHyperslab( H5S_SELECT_SET, count_out, offset_out );

#endif

    hsize_t i, j, k, l;

    for (j = 0; j < dimsm[0]; j++)
    {
        for (i = 0; i < dimsm[1]; i++)
        {
        for (k = 0; k < dimsm[2]; k++)
            for (l = 0; l < dimsm[3]; l++) 
                (*_sample_data)[j][i][k][l] = 0;
        }
    }

    // Get data
    dataset.read(_sample_data, H5::PredType::NATIVE_FLOAT);

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


    hsize_t i, j, k, l;
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



void MuirData::print_onesamplecolumn(float (&sample)[1100][2], float (&range)[1100])
{
    std::cout << std::setprecision(6);
    std::cout.setf(std::ios::fixed,std::ios::floatfield);

    for (int k = 0; k < 1100; k++)
        //std::cout << "" << (range[k]/1000) << " " << log10(sqrt(pow(sample[k][0],2) + pow(sample[k][1],2)))*10 << " " << std::endl;
        std::cout << "" << (range[k]/1000) << " " << log10(norm(std::complex<double>(sample[k][0], sample[k][1]))+1)*10-10 << " " << std::endl;
    std::cout << std::endl;
}

void MuirData::print_onesamplecolumn(std::size_t run, std::size_t column)
{
    print_onesamplecolumn((*_sample_data)[run][column], (*_sample_range)[0]);
}


void MuirData::save_2dplot(const std::string &output_file)
{
    FILE *fp = fopen(output_file.c_str(), "wb");
    if (!fp)
    {
        throw(std::runtime_error(std::string(__FILE__) + ":" + std::string(QUOTEME(__LINE__)) + "  " +
                std::string("Unable to open file for output (") + output_file + ")"));
    }


// XXX
    //png_structp png_ptr = png_create_write_struct
    //        (PNG_LIBPNG_VER_STRING, (png_voidp)user_error_ptr,
    //         user_error_fn, user_warning_fn);
    
    png_structp png_ptr = png_create_write_struct
            (PNG_LIBPNG_VER_STRING, NULL,
             NULL, NULL);

    if (!png_ptr)
        throw(std::runtime_error(std::string(__FILE__) + ":" + std::string(QUOTEME(__LINE__)) + "  " +
                std::string("Failed to create png_ptr.")));

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
    {
        png_destroy_write_struct(&png_ptr,
                                  (png_infopp)NULL);
        throw(std::runtime_error(std::string(__FILE__) + ":" + std::string(QUOTEME(__LINE__)) + "  " +
                std::string("Failed to create info_ptr.")));
    }

    // XXX FIXME - No idea what this does..  seems to be a location to jump back to when libpng doesn't know what to do.
    if (setjmp(png_jmpbuf(png_ptr)))
    {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        fclose(fp);
        throw(std::runtime_error(std::string(__FILE__) + ":" + std::string(QUOTEME(__LINE__)) + "  " +
                std::string("OMG Epic Fail while writing png.")));
    }

    // Associate file pointer to png
    png_init_io(png_ptr, fp);

    // Bah, don't need feedback
    //png_set_write_status_fn(png_ptr, write_row_callback);

	std::size_t dataset_width  = 500*10; // 500 pulses * 10 sets
	std::size_t dataset_height = 1100; // Range bins
	
    // Set up header
	png_uint_32 width            = dataset_width+9+3+20;
	png_uint_32 height           = dataset_height+2; 
	int         bit_depth        = 8;
	int         color_type       = PNG_COLOR_TYPE_PALETTE;
	int         interlace_type   = PNG_INTERLACE_NONE;
	int         compression_type = PNG_COMPRESSION_TYPE_DEFAULT;
	int         filter_method    = PNG_FILTER_TYPE_DEFAULT;

	png_set_IHDR(png_ptr, info_ptr, width, height,
				 bit_depth, color_type, interlace_type,
				 compression_type, filter_method);
    

    png_color palette[256];
    for (int i = 0; i < 32;i++) // blk -> blue
    {
        palette[i].red = 0;
        palette[i].green = 0;
        palette[i].blue = i*8;
    }
    for (int i = 0; i < 64;i++) // blue -> light blue
    {
        palette[32 + i].red = 0;
        palette[32 + i].green = i*4;
        palette[32 + i].blue = 255;
    }
    for (int i = 0; i < 64;i++) // light blue -> green
    {
        palette[96 + i].red = 0;
        palette[96 + i].green = 255;
        palette[96 + i].blue = 255-i*4;
    }
    for (int i = 0; i < 64;i++) // green -> yellow
    {
        palette[160 + i].red = i*4;
        palette[160 + i].green = 255;
        palette[160 + i].blue = 0;
    }
    for (int i = 0; i < 32;i++) // yellow -> red
    {
        palette[224 + i].red = 255-i*2;
        palette[224 + i].green = 255-i*8;
        palette[224 + i].blue = 0;
    }
    
    png_set_PLTE(png_ptr, info_ptr, palette, 256);

    // Write data
    {
        //int png_transforms = PNG_TRANSFORM_IDENTITY;
        //png_write_png(png_ptr, info_ptr, png_transforms, NULL);
        //typedef float (*SampleDataArray)[10][500][1100][2];
        std::cout << "WRITE INFO " << std::endl;
        png_set_invert_alpha(png_ptr);
        png_write_info(png_ptr, info_ptr);
        png_set_packing(png_ptr);

        png_write_flush(png_ptr);

        // Prepare data;

        png_byte row[width];
		for (std::size_t i = 0; i<width ; i++)
			row[i] = 0;
	
		// black top border
		png_write_row(png_ptr, row);		

        for (int i = dataset_height-1; i >= 0 ;i--)
        {
            for (std::size_t j = 0; j < 10;j++)
                for (std::size_t k = 0; k < 500;k++)
                    row[j*500 +k+j+1] = std::min(255.0,log10(norm(std::complex<float>((*_sample_data)[j][k][i][0], (*_sample_data)[j][k][i][1])))*10*4);
            for (std::size_t j = 0; j < 20;j++)
				row[dataset_width+2+9+j] = (float(i)/dataset_height)*(256.0);
			
			std::cout << "ROW: " << i << std::endl;
            png_write_row(png_ptr, row);
            
			

        }

	// Bottom border
	for (std::size_t i = 0; i<width ; i++)
		row[i] = 0;
	png_write_row(png_ptr, row);
		
		// Done with file
        png_write_end(png_ptr, info_ptr);

        char buf[80];
        time_t then;
        struct tm *ts;

        // Display time (for now)
        for(int i = 0; i < 10; i++)
        {
            for(int j = 0; j < 2; j++)
            {
                then = _time[i][j]/1000000;
                ts = localtime(&then);
                strftime(buf, sizeof(buf), "%a %Y-%m-%d %H:%M:%S %Z", ts);
                //std::cout << _time[i][0] << "," << _time[i][1] << std::endl;
                std::cout << buf << "(" << (_time[i][j]/1000000-then)*1000 << "ms)" << std::endl << (j?"--\n":"");
            }
        }
    }
}



