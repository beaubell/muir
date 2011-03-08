#include "muir-process.h"
#include "muir-process-cl.h"
//#include "muir-process-cuda.h"
#include "muir-process-cpu.h"

int opencl_initialized = 0;
int cuda_initialized = 0;
int cpu_initialized = 1;

int num_devices = 0;

int process_init(unsigned int method, void* opengl_ctx)
{
    // If no methods set, try them all.
    if(method == 0)
        method = 0xFFFFFFFF;

    // Since init was called, zero out cpu.
    cpu_initialized = false;

    // Number of processing devices found.
    num_devices = 0;
    
    // Attempt to initialize OpenCL context
    if(method & MUIR_DECODE_GPU_OPENCL)
    {
        int opencl_devices = process_init_cl(opengl_ctx);
        if (opencl_devices)
        {
            opencl_initialized = opencl_devices;
            num_devices += opencl_devices;

            // Add devices to list
            // TODO
        }
        
    }

    // Attempt to initialize Cuda context
    if(method & MUIR_DECODE_GPU_CUDA)
    {
        //int cuda_devices = process_init_cuda(opengl_ctx);
        //if (cuda_devices)
        //    cuda_initialied = true;
    }

    // Attempt to initialize CPU decoding context
    if(method & MUIR_DECODE_CPU)
    {
        int cpu_devices = process_init_cpu();
        if (cpu_devices)
        {
            cpu_initialized = cpu_devices;
            num_devices += cpu_devices;
        }
    }
    return num_devices;
}

int process_data(int id,
                 const Muir4DArrayF& sample_data,
                 const std::vector<float>& phasecode,
                 Muir3DArrayF& decoded_data,
                 DecodingConfig &config,
                 std::vector<std::string>& timing_strings,
                 Muir2DArrayD& timings)
{
    int err = 0;

    if ((id - opencl_initialized) < 0)
    {
        // Use OpenCL Decoding
        err = process_data_cl(id, sample_data, phasecode, decoded_data, config, timing_strings, timings);
    }
    //if ((cuda_initialized- id) < 0)
    //    // Use CUDA Decoding
    //    err = process_data_cuda(id, sample_data, phasecode, decoded_data);
    else if (cpu_initialized)
    {
        // Use CPU Decoding
        err = process_data_cpu(id - (opencl_initialized + cuda_initialized), sample_data, phasecode, decoded_data, config, timing_strings, timings);
    }
    
    return err;
}

int process_get_num_devices()
{
    return num_devices;
}

