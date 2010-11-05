
#include "muir-hd5.h"

#define QUOTEME_(x) #x
#define QUOTEME(x) QUOTEME_(x)

MuirHD5::MuirHD5(const std::string &filename_in, const unsigned int flags)
    :_h5file( _filename.c_str(), flags )
{
}

MuirHD5::~MuirHD5()
{
}

float MuirHD5::read_scalar_float(const H5std_string &dataset_name) const
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


std::string MuirHD5::read_string(const H5std_string &dataset_name) const
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

// READ FRAMECOUNTS
void MuirHD5::read_2D_uint(const H5std_string &dataset_name, Muir2DArrayUI &in) const
{

    // Get Dataset
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

    //if(dimsm[0] != 10 || dimsm[1] != 500 )
    //    throw(std::runtime_error(std::string(__FILE__) + ":" + std::string(QUOTEME(__LINE__)) + "  " +
    //            std::string("Expecting dimensions (10,500) in ") + dataset_name + " from " + _filename));

    // Initialize boost multi_array;
    in.resize(boost::extents[dimsm[0]][dimsm[1]]);

    hsize_t i, j;
    for (j = 0; j < dimsm[0]; j++)
    {
        for (i = 0; i < dimsm[1]; i++)
        {
            in[j][i] = 0;
        }
    }

    // Get data
    dataset.read(in.data(), H5::PredType::NATIVE_UINT);

    return;
}

void MuirHD5::read_2D_float(const H5std_string &dataset_name, Muir2DArrayF &in) const
{

    // Get Dataset
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

    //if(dimsm[0] != 1 || dimsm[1] != 1100 )
    //    throw(std::runtime_error(std::string(__FILE__) + ":" + std::string(QUOTEME(__LINE__)) + "  " +
    //            std::string("Expecting dimensions (1,1100) in ") + dataset_name + " from " + _filename));

    // Initialize boost multi_array;
    in.resize(boost::extents[dimsm[0]][dimsm[1]]);

    hsize_t i, j;
    for (j = 0; j < dimsm[0]; j++)
    {
        for (i = 0; i < dimsm[1]; i++)
        {
            in[j][i] = 0;
        }
    }

    // Get data
    dataset.read(in.data(), H5::PredType::NATIVE_FLOAT);

    return;
}

void MuirHD5::read_2D_double(const H5std_string &dataset_name, Muir2DArrayD &in) const
{

    // Get Dataset
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

     // Initialize boost multi_array;
    in.resize(boost::extents[dimsm[0]][dimsm[1]]);

    for (hsize_t j = 0; j < dimsm[0]; j++)
    {
        for (hsize_t i = 0; i < dimsm[1]; i++)
        {
            in[j][i] = 0;
        }
    }

    // get data
    dataset.read( in.data(), H5::PredType::NATIVE_DOUBLE );

    return;
}

void MuirHD5::read_4D_float(const H5std_string &dataset_name, Muir4DArrayF &in) const
{

    // Get Dataset
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

    //if(dimsm[0] != 10 || dimsm[1] != 500 || dimsm[2] != 1100 || dimsm[3] != 2)
    //    throw(std::runtime_error(std::string(__FILE__) + ":" + std::string(QUOTEME(__LINE__)) + "  " +
    //            std::string("Expecting dimensions (10,500,1100,2) in ") + dataset_name + " from " + _filename));

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

    // Initialize boost multi_array;
    in.resize(boost::extents[dimsm[0]][dimsm[1]][dimsm[2]][dimsm[3]]);


    hsize_t i, j, k, l;

    for (j = 0; j < dimsm[0]; j++)
    {
        for (i = 0; i < dimsm[1]; i++)
        {
            for (k = 0; k < dimsm[2]; k++)
                for (l = 0; l < dimsm[3]; l++) 
                    in[j][i][k][l] = 0;
        }
    }

    // Get data
    dataset.read(in.data(), H5::PredType::NATIVE_FLOAT);

    return;
}
