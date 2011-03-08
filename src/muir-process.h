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

#include <string>

#define MUIR_DECODE_CPU 0x01
#define MUIR_DECODE_GPU_OPENCL 0x02
#define MUIR_DECODE_GPU_CUDA   0x04

class DecodingConfig
{
  public:
    unsigned int fft_size;
    unsigned int phasecode_muting;
    unsigned int time_integration;
    unsigned int threads;
    std::string  platform;
    std::string  process;
    double decoding_time;

    DecodingConfig(void) :
    fft_size(1024),
    phasecode_muting(0),
    time_integration(0),
    threads(0),
    platform(""),
    process(""),
    decoding_time(0.0)
    {}
};


int process_init(unsigned int method, void* opengl_ctx = NULL);
int process_data(int id,
                 const Muir4DArrayF& sample_data,
                 const std::vector<float>& phasecode,
                 Muir3DArrayF& decoded_data,
                 DecodingConfig &config,
                 std::vector<std::string>& timing_strings,
                 Muir2DArrayD& timings);
int process_get_num_devices();

#endif //MUIR_PROCESS_H
