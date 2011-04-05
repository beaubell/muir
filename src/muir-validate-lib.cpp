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
    
    Muir3DArrayF::size_type dim1_size = array_dims[0];
    Muir3DArrayF::size_type dim2_size = array_dims[1];
    Muir3DArrayF::size_type dim3_size = array_dims[2];
    
    // Resize output
    output.resize(boost::extents[dim1_size][dim2_size][dim3_size]);
    
    float max_standard = -std::numeric_limits<float>::infinity( ), max_test = -std::numeric_limits<float>::infinity( );
    for (size_t dim1 = 0; dim1 < dim1_size; dim1++)
        for (size_t dim2 = 0; dim2 < dim2_size; dim2++)
            for (size_t dim3 = 0; dim3 < dim3_size; dim3++)
            {
                max_standard = std::max(max_standard, standard[dim1][dim2][dim3]);
                max_test = std::max(max_test, test[dim1][dim2][dim3]);
            }
            
    std::cout << "Maximums found, standard: " << max_standard << std::endl;
    std::cout << "                test    : " << max_test << std::endl;
    std::cout << "          Ratios s/t: " << max_standard/max_test << std::endl;
    std::cout << "          Ratios t/s: " << max_test/max_standard << std::endl;
    
    double accumulator = 0.0 ,sc_accumulator = 0.0;
    for (size_t dim1 = 0; dim1 < dim1_size; dim1++)
        for (size_t dim2 = 0; dim2 < dim2_size; dim2++)
            for (size_t dim3 = 0; dim3 < dim3_size; dim3++)
            {
                double abs_diff = abs(standard[dim1][dim2][dim3] - test[dim1][dim2][dim3]);
                accumulator += abs_diff;
                output[dim1][dim2][dim3] = abs_diff;
                sc_accumulator += abs(standard[dim1][dim2][dim3]/max_standard - test[dim1][dim2][dim3]/max_test);
            }

    std::cout << "       Sum Difference: " << accumulator << std::endl;
    std::cout << "Scaled Sum Difference: " << sc_accumulator << std::endl;

    return accumulator;
}

double diff_sum(const Muir4DArrayF &standard, const Muir4DArrayF &test, Muir4DArrayF &output)
{
    const Muir4DArrayF::size_type *array_dims = standard.shape();
    assert(standard.num_dimensions() == 4);
    
    Muir4DArrayF::size_type dim1_size = array_dims[0];
    Muir4DArrayF::size_type dim2_size = array_dims[1];
    Muir4DArrayF::size_type dim3_size = array_dims[2];
    Muir4DArrayF::size_type dim4_size = array_dims[3];
    
    // Resize output
    output.resize(boost::extents[dim1_size][dim2_size][dim3_size][dim4_size]);
    
    float max_standard = -std::numeric_limits<float>::infinity( ), max_test = -std::numeric_limits<float>::infinity( );
    for (size_t dim1 = 0; dim1 < dim1_size; dim1++)
        for (size_t dim2 = 0; dim2 < dim2_size; dim2++)
            for (size_t dim3 = 0; dim3 < dim3_size; dim3++)
                for (size_t dim4 = 0; dim4 < dim4_size; dim4++)
                {
                    max_standard = std::max(max_standard, standard[dim1][dim2][dim3][dim4]);
                    max_test = std::max(max_test, test[dim1][dim2][dim3][dim4]);
                }

    std::cout << "Maximums found, standard: " << max_standard << std::endl;
    std::cout << "                test    : " << max_test << std::endl;
    std::cout << "          Ratios s/t: " << max_standard/max_test << std::endl;
    std::cout << "          Ratios t/s: " << max_test/max_standard << std::endl;

    double accumulator = 0.0 ,sc_accumulator = 0.0;
    for (size_t dim1 = 0; dim1 < dim1_size; dim1++)
        for (size_t dim2 = 0; dim2 < dim2_size; dim2++)
            for (size_t dim3 = 0; dim3 < dim3_size; dim3++)
                for (size_t dim4 = 0; dim4 < dim4_size; dim4++)
                {
                    double abs_diff = abs(standard[dim1][dim2][dim3][dim4] - test[dim1][dim2][dim3][dim4]);
                    accumulator += abs_diff;
                    output[dim1][dim2][dim3][dim4] = abs_diff;
                    sc_accumulator += abs(standard[dim1][dim2][dim3][dim4]/max_standard - test[dim1][dim2][dim3][dim4]/max_test);
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