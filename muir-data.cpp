//
// C++ Implementation: muir-data
//
// Description: 
//
//
// Author: Beau V.C. Bellamy <bvbellamy@alaska.edu>, (C) 2010
//
// Copyright: See COPYING file that comes with this distribution
//
//


#include "muir-data.h"
#include "H5Cpp.h"
#include <iostream>  // std::cout
#include <iomanip>   // std::setprecision()
#include <exception>
#include <stdexcept> // std::runtime_error

#define QUOTEME_(x) #x
#define QUOTEME(x) QUOTEME_(x)


// Location Constants
const H5std_string PULSEWIDTH_PATH("/Raw11/Data/Pulsewidth");
const H5std_string BAUDLENGTH_PATH("/Raw11/Data/TxBaud");
const H5std_string EXPERIMENTFILE_PATH("/Setup/Experimentfile");
const H5std_string RADACTIME_PATH("/Time/RadacTime");

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

    std::cout << "Pulsewidth: " << _pulsewidth << std::endl;
    std::cout << "TX Baud   : " << _txbaud << std::endl;
    std::cout << "PW/TXBaud : " << _pulsewidth/_txbaud << std::endl;

    // Read Phasecode and run sanity checks
    read_phasecode();

    std::cout << "Phase Code: ";
    for(int i = 0; i < _phasecode.size(); i++)
      std::cout << _phasecode[i] << ",";

    std::cout << std::endl;
    std::cout << "Phase Code Len: " << _phasecode.size() << std::endl;

    // Check to see if Pulsewidth/TXBaud equals the amount of phase code parsed.
    if(_phasecode.size() != _pulsewidth/_txbaud)
       throw(std::runtime_error(std::string(__FILE__) + ": " + std::string(QUOTEME(__LINE__)) + "  " +
                                std::string("Phase code parsed doesn't equal Pulsewidth/TXBaud! ")));

    // Read start/stop times.
    read_times();
}

// Destructor
MuirData::~MuirData()
{


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
   dataset.read( data_out, H5::PredType::NATIVE_FLOAT);


   return data_out[0];
}


std::string MuirData::read_string(const H5std_string &dataset_name)
{

   H5std_string buffer;

   // Get dataset
   H5::DataSet dataset = _h5file.openDataSet( dataset_name );

   // Get Type class
   H5T_class_t type_class = dataset.getTypeClass();

   // Check to see if we are dealing with floats
   if( type_class != H5T_STRING )
      throw(std::runtime_error(std::string(__FILE__) + ":" + std::string(QUOTEME(__LINE__)) + "  " +
                               std::string("Expecting STRING Type from ") + dataset_name + " in " + _filename));

   H5::DataType dtype = dataset.getDataType();

   //hsize_t size = dataset.getStorageSize ();LOAT 

   //std::cout << "Storage size required: " << size << std::endl;

   // Read string
   dataset.read( buffer, dtype);

   return buffer;
}


void MuirData::read_phasecode()
{
    std::string experimentfile = read_string(EXPERIMENTFILE_PATH);

    //std::cout << experimentfile << std::endl;

    // Parse experimentfile...
    std::size_t index_min = experimentfile.find(";Code=");
    std::size_t index_max = experimentfile.find("\n\n[Common Parameters]");

    std::string phasecode_bulk = experimentfile.substr(index_min,index_max-index_min);
    std::size_t size = phasecode_bulk.length();

    //std::string phasecode;

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

    //return phasecode;
}


void MuirData::read_times()
{

    // Get Dataset
    H5::DataSet dataset = _h5file.openDataSet( RADACTIME_PATH );

    // Get Type class
    H5T_class_t type_class = dataset.getTypeClass();

    // Check to see if we are dealing with floats
    if( type_class != H5T_FLOAT )
       throw;

    H5::DataType dtype = dataset.getDataType();
    H5::DataType ftype = dataset.getFloatType();
    /*
     * Get class of datatype and print message if it's an integer.
       */
      //cout << type_class << endl;

    std::cout << "Data set has FLOAT type" << std::endl;

    H5::FloatType floattype = dataset.getFloatType();

    /*
          * Get order of datatype and print message if it's a little endian.
          */
      H5std_string order_string;
      H5T_order_t order = floattype.getOrder( order_string );
      std::cout << order_string << std::endl;

         /*
          * Get size of the data element stored in file and print it.
          */
      size_t size = floattype.getSize();
      std::cout << "Data size is " << size << std::endl;


      /*
       * Get dataspace of the dataset.
       */
      H5::DataSpace dataspace = dataset.getSpace();

      /*
       * Get the number of dimensions in the dataspace.
       */
      int rank = dataspace.getSimpleExtentNdims();

      /*
       * Get the dimension size of each dimension in the dataspace and
       * display them.
       */
      hsize_t dimensions[20];
      int ndims = dataspace.getSimpleExtentDims( dimensions, NULL);
      //cout << "rank " << rank << ", dimensions " <<
	//      (unsigned long)(dimensions[0]) << " x " <<
	//      (unsigned long)(dimensions[1]) << " x " <<
        //      (unsigned long)(dimensions[2]) << endl;

      hsize_t dimsm[20];              /* memory space dimensions */
      dimsm[0] = dimensions[0];
      dimsm[1] = dimensions[1];

      if (rank > 2)
       dimsm[2] = dimensions[2];
      else
       dimsm[2] = 1;

      if (rank > 3)
       dimsm[3] = dimensions[3];
      else
       dimsm[3] = 1;

      std::cout << "rank " << rank << ", dimensions " <<
              (unsigned long)(dimsm[0]) << " x " <<
              (unsigned long)(dimsm[1]) << " x " <<
              (unsigned long)(dimsm[2]) << " x " <<
              (unsigned long)(dimsm[3]) << std::endl;


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

      hsize_t i, j, k, l;
      //float* data_out = new float[dimsm[0] * dimsm[1] * dimsm[2] * dimsm[3] ]; /* output buffer */
      double data_out[dimsm[0]][dimsm[1]][dimsm[2]][dimsm[3]];

      for (j = 0; j < dimsm[0]; j++)
      {
         for (i = 0; i < dimsm[1]; i++)
         {
            for (k = 0; k < dimsm[2]; k++)
               for (l = 0; l < dimsm[3]; l++) 
                  data_out[j][i][k][l] = 0;

            //std::cout << j << ":" << i << ":" << k << " ";
         }
      }

      // Get attributes
      H5::Attribute myatt_out = dataset.openAttribute("Descriptions");

      H5::StrType strdatatype(H5::PredType::C_S1, myatt_out.getStorageSize());
      std::string strreadbuf;

      myatt_out.read(strdatatype, strreadbuf);
      std::cout << "Description: " << strreadbuf << std::endl;

      /*
       * Read data from hyperslab in the file into the hyperslab in
       * memory and display the data.
       */
      dataset.read( data_out, H5::PredType::NATIVE_DOUBLE, memspace, dataspace );

      std::cout << std::setprecision(6);
      std::cout.setf(std::ios::fixed,std::ios::floatfield);

      for (j = 0; j < dimsm[0]; j++)
      {
	for (i = 0; i < dimsm[1]; i++)
           for (k = 0; k < dimsm[2]; k++)
              for (l = 0; l < dimsm[3]; l++)
	         std::cout << data_out[j][i][k][l]/1000000 << " ";

	std::cout << std::endl << "CELL: " << j << ":" << i << ":" << k << std::endl;
      }


    return;
}

