#ifndef MUIR_VALIDATE_LIB_H
#define MUIR_VALIDATE_LIB_H
//
// C++ Interface: muir-validate-lib
//
// Description: Common functions for validating MUIR processing routines.
//
//
// Author: Beau V.C. Bellamy <bvbellamy@arsc.edu>
//         Arctic Region Supercomputing Center
//
//
//

#include "muir-hd5.h"

unsigned int check_timing(const MuirHD5 &file);
double diff_sum(const Muir3DArrayF &standard, const Muir3DArrayF &test, Muir3DArrayF &output);
double diff_sum(const Muir4DArrayF &standard, const Muir4DArrayF &test, Muir4DArrayF &output);

#endif //MUIR_VALIDATE_LIB_H
