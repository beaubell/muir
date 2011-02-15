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

int process_init(void* opengl_ctx);
int process_data(const Muir4DArrayF& sample_data, const std::vector<float>& phasecode, Muir3DArrayF& decoded_data);

#endif //MUIR_PROCESS_H
