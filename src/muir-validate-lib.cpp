//
// C++ Implementation: muir-validate-lib
//
// Description: Common functions for validating MUIR processing routines.
//
//
// Author: Beau V.C. Bellamy <bvbellamy@arsc.edu>
//         Arctic Region Supercomputing Center
//
//
//

#include "muir-validate-lib.h"
#include "muir-constants.h"

#include <cassert>
#include <limits>
#include <string>
#include <iostream>

double diff_sum(const Muir3DArrayF &standard, const Muir3DArrayF &test, Muir3DArrayF &output)
{
    const Muir3DArrayF::size_type *array_dims = standard.shape();
    assert(standard.num_dimensions() == 3);
    
    Muir3DArrayF::size_type max_sets = array_dims[0];
    Muir3DArrayF::size_type max_cols = array_dims[1];
    Muir3DArrayF::size_type max_range = array_dims[2];
    
    // Resize output
    output.resize(boost::extents[max_sets][max_cols][max_range]);
    
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
                double abs_diff = abs(standard[set][col][range] - test[set][col][range]);
                accumulator += abs_diff;
                output[set][col][range] = abs_diff;
                sc_accumulator += abs(standard[set][col][range]/max_standard - test[set][col][range]/max_test);
            }
            
            std::cout << "       Sum Difference: " << accumulator << std::endl;
        std::cout << "Scaled Sum Difference: " << sc_accumulator << std::endl;
        
        return accumulator;
}

unsigned int check_timing(const MuirHD5 &file)
{
    Muir2DArrayD rowtiming;
    
    file.read_2D_double(RTI_DECODEDROWTIMINGDATA_PATH, rowtiming);
    
    const Muir2DArrayD::size_type *array_dims = rowtiming.shape();
    assert(rowtiming.num_dimensions() == 2);
    
    Muir3DArrayF::size_type num_stages = array_dims[0];
    Muir3DArrayF::size_type num_rangebins = array_dims[1];
    
    unsigned int num_negatives = 0;
    
    for (size_t stage = 0; stage < num_stages; stage++)
    {
        for (size_t rangebin = 0; rangebin < num_rangebins; rangebin++)
        {
            if(rowtiming[stage][rangebin] < 0.0)
            {
                num_negatives++;
            }
            
        }
    }
    
    return num_negatives;
}