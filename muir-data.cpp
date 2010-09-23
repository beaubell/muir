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
#include <iostream>
#include <exception>
#include <stdexcept>

#define QUOTEME_(x) #x
#define QUOTEME(x) QUOTEME_(x)


// Location Constants
const H5std_string PULSEWIDTH_PATH("/Raw11/Data/Pulsewidth");
const H5std_string BAUDLENGTH_PATH("/Raw11/Data/TxBaud");
const H5std_string EXPERIMENTFILE_PATH("/Setup/Experimentfile");

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

    // Read ExperimentFile
    std::string phase_code = read_phasecode();
    std::cout << "Phase Code: " << phase_code << std::endl;
    std::cout << "Phase Code Len: " << phase_code.length() << std::endl;

    // Check to see if Pulsewidth/TXBaud equals the amount of phase code parsed.
    if(phase_code.length() + 1 != _pulsewidth/_txbaud)
       throw(std::runtime_error(std::string(__FILE__) + ": " + std::string(QUOTEME(__LINE__)) + "  " +
                                std::string("Phase code parsed doesn't equal Pulsewidth/TXBaud! ")));


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
      throw;

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
      throw;

   H5::DataType dtype = dataset.getDataType();

   //hsize_t size = dataset.getStorageSize ();

   //std::cout << "Storage size required: " << size << std::endl;

   // Read string
   dataset.read( buffer, dtype);

   return buffer;
}


std::string MuirData::read_phasecode()
{
    std::string experimentfile = read_string(EXPERIMENTFILE_PATH);

    //std::cout << experimentfile << std::endl;

    // Parse experimentfile...
    std::size_t index_min = experimentfile.find(";Code=");
    std::size_t index_max = experimentfile.find("\n\n[Common Parameters]");

    std::string phasecode_bulk = experimentfile.substr(index_min,index_max-index_min);
    std::size_t size = phasecode_bulk.length();

    std::string phasecode;

    // Get values for phasecode and dump them into string.
    for (std::size_t i = 0; i < size; i++)
    {
        if (phasecode_bulk[i] == '+')
        {
           phasecode += '1';
           continue;
        }
 
        if (phasecode_bulk[i] == '-')
        {
           phasecode += '0';
           continue;
        }
    }

    return phasecode;
}
