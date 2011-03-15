//
//

#include "muir-config.h"
#include "muir-hd5.h"
#include "muir-constants.h"
#include "muir-types.h"

#include <string>
#include <iostream>
#include <limits>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/convenience.hpp>

namespace fs = boost::filesystem;

// Prototypes
void print_help (void);
double diff_sum(Muir3DArrayF &standard, Muir3DArrayF &test);

int main (const int argc, const char * argv[])
{
    std::cout << "MUIR Validate, Version " << PACKAGE_VERSION << std::endl;
    
    // There must at least be a file specified
    if ( argc < 3 )
    {
        print_help();
        return 1;
    }

    std::vector<fs::path> files;

    for (int argi = 1; argi < argc; argi++)
    {

        fs::path path1(argv[argi]);

        // If not a command, must be a file
        if (fs::is_directory(path1))
        {
            std::cout << "ERROR!  You specified a directory! No!" << std::endl;
            return 434;
            for (fs::directory_iterator dirI(path1); dirI!=fs::directory_iterator(); ++dirI)
            {
                //std::cout << dirI->string() << std::endl;

                if (!fs::is_directory(*dirI))
                    files.push_back(*dirI);
                /// FIXME Doesn't scan higher directories
            }
        }
        else
        {
            // Just add this file
            files.push_back(path1);
        }
    }

    // Open file
    MuirHD5 file_standard( files[0].string(), H5F_ACC_RDONLY );

    // Open file
    MuirHD5 file_test( files[1].string(), H5F_ACC_RDONLY );

    // Get data
    Muir3DArrayF data_standard;
    Muir3DArrayF data_test;

    file_standard.read_3D_float(RTI_DECODEDDATA_PATH, data_standard);
    file_test.read_3D_float(RTI_DECODEDDATA_PATH, data_test);

    diff_sum(data_standard, data_test);

    return 0;  // successfully terminated
}

double diff_sum(Muir3DArrayF &standard, Muir3DArrayF &test)
{
    const Muir3DArrayF::size_type *array_dims = standard.shape();
    assert(standard.num_dimensions() == 3);

    Muir4DArrayF::size_type max_sets = array_dims[0];
    Muir4DArrayF::size_type max_cols = array_dims[1];
    Muir4DArrayF::size_type max_range = array_dims[2];

    float max_standard = -std::numeric_limits<float>::infinity( ), max_test = -std::numeric_limits<float>::infinity( );
    for (size_t set = 0; set < max_sets; set++)
        for (size_t col = 0; col < max_cols; col++)
            for (size_t range = 0; range < max_range; range++)
            {
                max_standard = std::max(max_standard, standard[set][col][range]);
                max_test = std::max(max_test, test[set][col][range]);
            }

    std::cout << "Maximums found, standard: " << max_standard << std::endl;
    std::cout << "                test    : " << max_test << std::endl;
    std::cout << "          Ratios s/t: " << max_standard/max_test << std::endl;
    std::cout << "          Ratios t/s: " << max_test/max_standard << std::endl;

    double accumulator = 0.0 ,sc_accumulator = 0.0;
    for (size_t set = 0; set < max_sets; set++)
        for (size_t col = 0; col < max_cols; col++)
            for (size_t range = 0; range < max_range; range++)
            {
                accumulator    += abs(standard[set][col][range] - test[set][col][range]);
                sc_accumulator += abs(standard[set][col][range]/max_standard - test[set][col][range]/max_test);
            }

    std::cout << "       Sum Difference: " << accumulator << std::endl;
    std::cout << "Scaled Sum Difference: " << sc_accumulator << std::endl;
    
    
    return accumulator;
}


void print_help ()
{
    std::cout << "usage: muir-validate standard.h5 test.h5" << std::endl;

}

