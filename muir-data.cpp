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

#include <gd.h>
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
const H5std_string FRAMECOUNT_PATH("/Raw11/Data/RadacHeader/FrameCount");

// Constructor
MuirData::MuirData(const std::string &filename_in)
: _filename(filename_in),
  _h5file( _filename.c_str(), H5F_ACC_RDONLY ),
  _pulsewidth(0),
  _txbaud(0),
  _phasecode(),
  _sample_data(NULL),
  _sample_range(NULL),
  _framecount(NULL)
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
	_framecount = (FrameCountArray) new float[10][500];


    read_sampledata();
    read_samplerange();
	read_framecount();
	
	//for (int i = 0; i < 10; i++)
	//	for (int j = 0 ; j < 500; j++)
	//		std::cout << i << "," << j << ":" << (*_framecount)[i][j] << std::endl;

}

// Destructor
MuirData::~MuirData()
{
    delete[] _sample_data;
    delete[] _sample_range;
    delete[] _framecount;

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

void MuirData::print_onesamplecolumn(std::size_t run, std::size_t column)
{
    print_onesamplecolumn((*_sample_data)[run][column], (*_sample_range)[0]);
}


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
            then = _time[i][k]/1000000;
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
    std::size_t dataset_width  = 500;
    std::size_t dataset_count  = 10;   // # sets
    std::size_t dataset_height = 1100; // Range bins

    std::size_t axis_x_height = 10;
    std::size_t border = 1;

    std::size_t start_frame = (*_framecount)[0][0];
    std::size_t end_frame = (*_framecount)[9][499];
    std::size_t num_frames = end_frame - start_frame;

    std::size_t width            = (num_frames)/delta_t+(border*4)+20;
    std::size_t height           = dataset_height+(2*border)+axis_x_height; 
    int         bit_depth        = 8;

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

    // Write data
    std::cerr << "CREATING IMAGE" << std::endl;

    size_t imageset_width = (dataset_width/delta_t);

    for (unsigned int i = 0; i < dataset_height ;i++)
    {
        for (std::size_t set = 0; set < dataset_count;set++)
        {
            std::size_t frameoffset = ((*_framecount)[set][0]-start_frame)/delta_t;

            for (std::size_t k = 0; k < imageset_width; k++)
            {

                if (delta_t == 1)
                {
                    unsigned char pixel = static_cast<unsigned int>(std::min(254.0,log10(norm(std::complex<float>((*_sample_data)[set][k][i][0], (*_sample_data)[set][k][i][1])))*10*4));
                    gdImageSetPixel(im, border + frameoffset + k, dataset_height-i, pixel);
                }
                else if (delta_t == 2)
                {
                    float col1 = std::min(254.0,log10(norm(std::complex<float>((*_sample_data)[set][delta_t*k][i][0], (*_sample_data)[set][delta_t*k][i][1])))*10*4);
                    float col2 = std::min(254.0,log10(norm(std::complex<float>((*_sample_data)[set][delta_t*k+1][i][0], (*_sample_data)[set][delta_t*k+1][i][1])))*10*4);
                    gdImageSetPixel(im, border + frameoffset + k, dataset_height-i, static_cast<unsigned char>((col1 + col2)/2.0));
                }
                else if (delta_t == 4) // It might be better if this was a loop.....  just a thought.  It does look rather loopworthy...
                {
                    float col1 = std::min(254.0,log10(norm(std::complex<float>((*_sample_data)[set][delta_t*k][i][0], (*_sample_data)[set][delta_t*k][i][1])))*10*4);
                    float col2 = std::min(254.0,log10(norm(std::complex<float>((*_sample_data)[set][delta_t*k+1][i][0], (*_sample_data)[set][delta_t*k+1][i][1])))*10*4);
                    float col3 = std::min(254.0,log10(norm(std::complex<float>((*_sample_data)[set][delta_t*k+2][i][0], (*_sample_data)[set][delta_t*k+2][i][1])))*10*4);
                    float col4 = std::min(254.0,log10(norm(std::complex<float>((*_sample_data)[set][delta_t*k+3][i][0], (*_sample_data)[set][delta_t*k+3][i][1])))*10*4);
                    gdImageSetPixel(im, border + frameoffset + k, dataset_height-i, static_cast<unsigned char>((col1 + col2 + col3 + col4)/4.0));
                }
            }
        }

        // Color bar
        for (std::size_t col = 0; col < 20; col++)
            gdImageSetPixel(im, width-col-2, dataset_height-i, static_cast<unsigned char>((float(i)/dataset_height)*(256.0)));


        if (!(i%10))
            std::cout << "ROW: " << i << std::endl;


    }

    // Bottom Axis
    for (std::size_t x = border; x<(width-border*2-20); x = x + 5)
        gdImageLine(im, x, (dataset_height+border*2), x, (dataset_height + ((x-border)%10?(axis_x_height/2):axis_x_height)), white); 


    // Done with file
    std::cerr << "WRITING IMAGE " << std::endl;
    gdImagePngEx(im, fp, 9);
    gdImageDestroy(im);
    fclose(fp);

}

