#ifndef MUIR_PROCESS_CL_H
#define MUIR_PROCESS_CL_H
//
// C++ Interface: muir-process-cl
//
// Description: Process and Decode MUIR experiment data with OpenCL.
//
//
// Author: Beau V.C. Bellamy <bvbellamy@arsc.edu>
//         Arctic Region Supercomputing Center
//
//
//

#include "muir-types.h"
#include "muir-process.h"

int process_init_cl(void* opengl_ctx);
int process_data_cl(int id,
                    const Muir4DArrayF& sample_data,
                    const std::vector<float>& phasecode,
                    Muir3DArrayF& decoded_data,
                    DecodingConfig &config,
                    std::vector<std::string>& timing_strings,
                    Muir2DArrayD& timings);

#endif //MUIR_PROCESS_CL_H
