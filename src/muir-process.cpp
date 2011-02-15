#include "muir-process.h"
#include "muir-process-cl.h"
//#include "muir-process-cuda.h"
//#include "muir-process-cpu.h"


int process_init(void* opengl_ctx)
{
    // Attempt to initialize OpenCL context
    int err = process_init_cl(opengl_ctx);

    // Attempt to initialize Cuda context
    //int err = process_cuda_init(opengl_ctx);

    // Attempt to initialize CPU decoding context
    //int err = process_cpu_init(opengl_ctx);

    return err;
}

int process_data(const Muir4DArrayF& sample_data, const std::vector<float>& phasecode, Muir3DArrayF& decoded_data)
{
    // Use OpenCL Decoding
    int err = process_data_cl(sample_data, phasecode, decoded_data);

    return err;
}