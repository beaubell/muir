#ifndef MUIR_PROCESS_H
#define MUIR_PROCESS_H
//
// C++ Interface: muir-process
//
// Description: Process and Decode MUIR experiment data.
//
//
// Author: Beau V.C. Bellamy <bvbellamy@arsc.edu>
//         Arctic Region Supercomputing Center
//
//
//

#include "muir-types.h"

#define MUIR_DECODE_CPU 0x01
#define MUIR_DECODE_GPU_OPENCL 0x02
#define MUIR_DECODE_GPU_CUDA   0x04

int process_init(unsigned int method, void* opengl_ctx = NULL);
int process_data(int id, const Muir4DArrayF& sample_data, const std::vector<float>& phasecode, Muir3DArrayF& decoded_data);
int process_get_num_devices();

#endif //MUIR_PROCESS_H
