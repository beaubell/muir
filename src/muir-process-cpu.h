#ifndef MUIR_PROCESS_CPU_H
#define MUIR_PROCESS_CPU_H
//
// C++ Interface: muir-process
//
// Description: Process and Decode MUIR experiment datddth the CPU.
//
//
// Author: Beau V.C. Bellamy <bvbellamy@arsc.edu>
//         Arctic Region Supercomputing Center
//
//
//

#include "muir-types.h"

int process_init_cpu(void);
int process_data_cpu(const Muir4DArrayF& sample_data, const std::vector<float>& phasecode, Muir3DArrayF& decoded_data);

#endif //MUIR_PROCESS_CPU_H
