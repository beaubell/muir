//
// C++ Executable: muir-compare
//
// Description: Executable to compare two or more output files.
//
//
// Author: Beau V.C. Bellamy <bvbellamy@arsc.edu>
//         (c) 2011, Arctic Region Supercomputing Center
//
//
//

#include "muir-config.h"
#include "muir-hd5.h"
#include "muir-constants.h"
#include "muir-types.h"
#include "muir-validate-lib.h"

#include <string>
#include <iostream>
#include <limits>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/convenience.hpp>

namespace fs = boost::filesystem;

// Prototypes
void print_help (void);
double diff_sum(const Muir3DArrayF &standard, const Muir3DArrayF &test, Muir3DArrayF &output);

int main (const int argc, const char * argv[])
{
    std::cout << "MUIR Compare, Version " << PACKAGE_VERSION << std::endl;
    
    // There must be at least two files specified
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
    Muir3DArrayF data_output;

    file_standard.read_3D_float(RTI_DECODEDDATA_PATH, data_standard);
    file_test.read_3D_float(RTI_DECODEDDATA_PATH, data_test);

    unsigned int fftsize_standard = file_standard.read_scalar_uint(RTI_DECODEDFFTSIZE_PATH);
    unsigned int fftsize_test = file_test.read_scalar_uint(RTI_DECODEDFFTSIZE_PATH);

    std::string srcfile_standard = file_standard.read_string(RTI_DECODEDSOURCEFILE_PATH);
    std::string srcfile_test     = file_test.read_string(RTI_DECODEDSOURCEFILE_PATH);

    std::cout << std::endl;
    std::cout << "File 1: " << files[0].string() << std::endl;
    std::cout << " -Source File: " << srcfile_standard << std::endl;
    std::cout << " -FFT Size   : " << fftsize_standard << std::endl << std::endl;
    std::cout << "File 2: " << files[1].string() << std::endl;
    std::cout << " -Source File: " << srcfile_test << std::endl;
    std::cout << " -FFT Size   : " << fftsize_test << std::endl << std::endl;

    if(srcfile_standard != srcfile_test)
    {
        std::cout << "** Warning, Source files do not match." << std::endl;
    }
    if(fftsize_standard != fftsize_test)
    {
        std::cout << "** Warning, FFT sizes do not match." << std::endl;
    }

    std::cout << "Calculating differences..." << std::endl;
    diff_sum(data_standard, data_test, data_output);

    std::cout << "Verifying Timing Information..." << std::endl;
    unsigned int negs = 0;
    negs = check_timing(file_standard);
    if (negs)
        std::cout << "Warning! " << file_standard.getFileName() << " has negative timing information." << std::endl;

    negs = check_timing(file_test);
    if (negs)
        std::cout << "Warning! " << file_test.getFileName() << " has negative timing information." << std::endl;

    // Output file
    if(files.size() == 3)
    {
        if(fs::is_regular_file(files[2]))
        {
            std::cout << "Output File: " << files[2].string() << " exists!" << std::endl;
            return 1;
        }

        MuirHD5 file_output( files[2].string(), H5F_ACC_TRUNC);

        // Create group
        file_output.createGroup(RTI_DECODEDDIR_PATH);

        // Prepare and write decoded sample data
        file_output.write_3D_float(RTI_DECODEDDATA_PATH, data_output);

        Muir2DArrayF _sample_range;
        // Get range data
        file_standard.read_2D_float(RTI_DECODEDRANGE_PATH, _sample_range);
        // Prepare and write range data
        file_output.write_2D_float(RTI_DECODEDRANGE_PATH, _sample_range);

        Muir2DArrayD _time;
        // Get radac data
        file_standard.read_2D_double(RTI_DECODEDRADAC_PATH, _time);
        // Prepare and write radac data
        file_output.write_2D_double(RTI_DECODEDRADAC_PATH, _time);

        Muir2DArrayUI _framecount;
        // Get framecount data
        file_standard.read_2D_uint(RTI_DECODEDFRAME_PATH, _framecount);
        // Prepare and write framecount data
        file_output.write_2D_uint(RTI_DECODEDFRAME_PATH, _framecount);

    }

    return 0;  // successfully terminated
}

void print_help ()
{
    std::cout << "usage: muir-validate standard.h5 test.h5" << std::endl;

}


