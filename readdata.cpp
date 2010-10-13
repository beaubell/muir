// 
//
//
#include <cstdio>
#include <iostream>
#include <string>

#ifndef H5_NO_NAMESPACE
#ifndef H5_NO_STD
    using std::cout;
    using std::endl;
#endif  // H5_NO_STD
#endif

#include "muir-data.h"

#include "H5Cpp.h"

#ifndef H5_NO_NAMESPACE
    using namespace H5;
#endif

int main (int argc, const char * argv[])
{
   // There must at least be a file specified
   if ( argc < 2 )
   {
       cout << "usage: readdata hdf5file {[run#] [column#]}" << endl;
       return 1;
   }

   // Load muir data from file
   MuirData data(argv[1]);

   // Print a run and column if specified
   if ( argc > 3 )
   data.print_onesamplecolumn(atoi(argv[2]), atoi(argv[3]));

   data.save_2dplotgd("plot.png");

   return 0;  // successfully terminated
}

