//
// C++ Executable: muir-validate
//
// Description: Executable to validate MUIR processing routines. (GPU versus CPU)
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
#include "muir-utility.h"
#include "muir-validate-lib.h"
#include "muir-process.h"
#include "muir-process-cl.h"
#include "muir-process-cpu.h"

#include <string>
#include <iostream>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/convenience.hpp>

namespace fs = boost::filesystem;

// Prototypes
void print_help (void);

int main (const int argc, const char * argv[])
{
    std::cout << "MUIR Validate, Version " << PACKAGE_VERSION << std::endl;

    // There must be only one file specified
    if ( argc < 2 )
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
        }
        else
        {
            // Just add this file
            files.push_back(path1);
        }
    }

    // Open file
    MuirHD5 unprocessed_file( files[0].string(), H5F_ACC_RDONLY );


    // Get data
    std::cout << "Reading file: " << unprocessed_file.getFileName() << std::endl;
    Muir4DArrayF unprocessed_data;
    unprocessed_file.read_4D_float(RTI_RAWSAMPLEDATA_PATH, unprocessed_data);


    // Get Phasecode
    std::vector<float> phasecode;
    read_phasecode(unprocessed_file, phasecode);

    if(phasecode.empty())
    {
        std::cout << " Error: Cannot decode file! No phasecode. File: " << unprocessed_file.getFileName() << std::endl;
        return 1;
    }


    // Setup For Processing
    int contexts_avail = process_init(0);

    // Create Arrays
    Muir4DArrayF complex_intermediate_1(boost::extents[1][1][1][2]);
    Muir4DArrayF complex_intermediate_2(boost::extents[1][1][1][2]);
    Muir4DArrayF difference4D(boost::extents[1][1][1][2]);

    Muir3DArrayF processed_data_1;
    Muir3DArrayF processed_data_2;

    DecodingConfig config_1;
    DecodingConfig config_2;

    std::vector<std::string> timing_strings_1;
    std::vector<std::string> timing_strings_2;
    Muir2DArrayD             timings_1;
    Muir2DArrayD             timings_2;

    // Config Processing
    unsigned int row = 0;
    Decoding_Stage stage = STAGE_PHASECODE;
    config_1.intermediate_row = row;
    config_2.intermediate_row = row;
    config_1.intermediate_stage = stage;
    config_2.intermediate_stage = stage;

    // OpenCL Method
    std::cout << "Processing using OpenCL Method..." << std::endl;
    process_data_cl(0, unprocessed_data, phasecode, processed_data_1, config_1, timing_strings_1, timings_1, complex_intermediate_1);
    print_dimensions(complex_intermediate_1);
    print_dimensions(processed_data_1);

    // CPU Method
    std::cout << "Processing using CPU Method..." << std::endl;
    process_data_cpu(0, unprocessed_data, phasecode, processed_data_2, config_2, timing_strings_2, timings_2, complex_intermediate_2);
    print_dimensions(complex_intermediate_2);
    print_dimensions(processed_data_2);


    diff_sum(complex_intermediate_1, complex_intermediate_2, difference4D);

    MuirHD5 file_output(std::string("gpu_method.h5"), H5F_ACC_TRUNC);

    // Create group
    file_output.createGroup(RTI_DECODEDDIR_PATH);
    
    std::string intermediate_path = RTI_DECODEDDIR_PATH + std::string("/Intermediate");
    file_output.createGroup(intermediate_path);

    // Prepare and write decoded sample data
    std::string gpu_path = intermediate_path + std::string("/GPU");
    std::string cpu_path = intermediate_path + std::string("/CPU");
    std::string diff_path = intermediate_path + std::string("/Diff");
    file_output.write_4D_float(gpu_path, complex_intermediate_1);
    file_output.write_4D_float(cpu_path, complex_intermediate_2);
    file_output.write_4D_float(diff_path, difference4D);
    
    Muir2DArrayF _sample_range;
    // Get range data
    unprocessed_file.read_2D_float(RTI_RAWSAMPLERANGE_PATH, _sample_range);
    // Prepare and write range data
    file_output.write_2D_float(RTI_DECODEDRANGE_PATH, _sample_range);

    Muir2DArrayD _time;
    // Get radac data
    unprocessed_file.read_2D_double(RTI_RADACTIME_PATH, _time);
    // Prepare and write radac data
    file_output.write_2D_double(RTI_DECODEDRADAC_PATH, _time);

    Muir2DArrayUI _framecount;
    // Get framecount data
    unprocessed_file.read_2D_uint(RTI_RAWFRAMECOUNT_PATH, _framecount);
    // Prepare and write framecount data
    file_output.write_2D_uint(RTI_DECODEDFRAME_PATH, _framecount);
    return 0;  // successfully terminated
}



void print_help ()
{
    std::cout << "usage: muir-validate unprocessed.h5" << std::endl;
    
}
