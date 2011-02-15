#include "muir-process.h"
#include "muir-process-cl.h"
//#include "muir-process-cuda.h"
#include "muir-process-cpu.h"

bool opencl_initialized = false;

int process_init(void* opengl_ctx)
{
    int err = 0;
    
    // Attempt to initialize OpenCL context
    err = process_init_cl(opengl_ctx);
    if (!err)
        opencl_initialized = true;

    // Attempt to initialize Cuda context
    //int err = process_init_cuda(opengl_ctx);

    // Attempt to initialize CPU decoding context
    err = process_init_cpu();

    return err;
}

int process_data(const Muir4DArrayF& sample_data, const std::vector<float>& phasecode, Muir3DArrayF& decoded_data)
{
    int err = 0;
    
    if (opencl_initialized)
        // Use OpenCL Decoding
        err = process_data_cl(sample_data, phasecode, decoded_data);
    else
        // Use CPU Decoding
        err = process_data_cpu(sample_data, phasecode, decoded_data);

    return err;
}