// 
//
//

#include <iostream>
#include <string>

#ifndef H5_NO_NAMESPACE
#ifndef H5_NO_STD
    using std::cout;
    using std::endl;
#endif  // H5_NO_STD
#endif

#include "H5Cpp.h"

#ifndef H5_NO_NAMESPACE
    using namespace H5;
#endif

int main (int argc, const char * argv[])
{
   // 
   if ( argc < 3 )
   {
       cout << "usage: readdata hdf5file dataset_path" << endl;
       return 1;
   }

   // Pull in filepath and dataset from arguments
   H5std_string file_name( argv[1] );
   H5std_string dataset_name ( argv[2] );

   /*
    * Try block to detect exceptions raised by any of the calls inside it
    */
   try
   {
      /*
       * Turn off the auto-printing when failure occurs so that we can
       * handle the errors appropriately
       */
      //Exception::dontPrint();

      /*
       * Open the specified file and the specified dataset in the file.
       */
      H5File file( file_name, H5F_ACC_RDONLY );
      DataSet dataset = file.openDataSet( dataset_name );

      /*
       * Get the class of the datatype that is used by the dataset.
       */
      H5T_class_t type_class = dataset.getTypeClass();

      /*
       * Get class of datatype and print message if it's an integer.
       */
      //cout << type_class << endl;

      if( type_class == H5T_FLOAT )
      {
	 cout << "Data set has FLOAT type" << endl;

         /*
	  * Get the integer datatype
          */
	 FloatType floattype = dataset.getFloatType();

         /*
          * Get order of datatype and print message if it's a little endian.
          */
	 H5std_string order_string;
         H5T_order_t order = floattype.getOrder( order_string );
	 cout << order_string << endl;

         /*
          * Get size of the data element stored in file and print it.
          */
         size_t size = floattype.getSize();
         cout << "Data size is " << size << endl;
      }

      

      /*
       * Get dataspace of the dataset.
       */
      DataSpace dataspace = dataset.getSpace();

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

      cout << "rank " << rank << ", dimensions " <<
              (unsigned long)(dimsm[0]) << " x " <<
              (unsigned long)(dimsm[1]) << " x " <<
              (unsigned long)(dimsm[2]) << " x " <<
              (unsigned long)(dimsm[3]) << endl;


      DataSpace memspace( rank, dimsm );

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
      float data_out[dimsm[0]][dimsm[1]][dimsm[2]][dimsm[3]];

      for (j = 0; j < dimsm[0]; j++)
      {
         for (i = 0; i < dimsm[1]; i++)
         {
            for (k = 0; k < dimsm[2]; k++)
               for (l = 0; l < dimsm[3]; l++) 
                  data_out[j][i][k][l] = 0;

            cout << j << ":" << i << ":" << k << " ";
         }
      }



      /*
       * Read data from hyperslab in the file into the hyperslab in
       * memory and display the data.
       */
      dataset.read( data_out, PredType::NATIVE_FLOAT, memspace, dataspace );

      for (j = 0; j < dimsm[0]; j++)
      {
	for (i = 0; i < dimsm[1]; i++)
           for (k = 0; k < dimsm[2]; k++)
              for (l = 0; l < dimsm[3]; l++)
	         cout << data_out[j][i][k][l] << " ";

	cout << endl << "CELL: " << j << ":" << i << ":" << k << endl;
      }

   }  // end of try block

   // catch failure caused by the H5File operations
   catch( FileIException error )
   {
      error.printError();
      return -1;
   }

   // catch failure caused by the DataSet operations
   catch( DataSetIException error )
   {
      error.printError();
      return -1;
   }

   // catch failure caused by the DataSpace operations
   catch( DataSpaceIException error )
   {
      error.printError();
      return -1;
   }

   // catch failure caused by the DataSpace operations
   catch( DataTypeIException error )
   {
      error.printError();
      return -1;
   }

   return 0;  // successfully terminated
}

