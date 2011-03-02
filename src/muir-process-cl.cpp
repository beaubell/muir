#define __CL_ENABLE_EXCEPTIONS
 
//#if defined(__APPLE__) || defined(__MACOSX)
//#include <OpenCL/cl.hpp>
//#else
#include <CL/cl.hpp>
//#endif
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <fstream>

// Boost::Accumulators
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/min.hpp>
#include <boost/accumulators/statistics/max.hpp>
#include <boost/accumulators/statistics/count.hpp>
using namespace boost::accumulators;

#include <boost/bind.hpp>
using boost::bind;

// Boost::Timers
#include <boost/timer.hpp>
using boost::timer;


#include "muir-hd5.h"
#include "muir-utility.h"
#include "muir-constants.h"
#include "muir-global.h"
#include "muir-process.h"

/// Constants
static const std::string SectionName("OpenCL");

/// OpenCL Global State
std::vector<cl::Platform> muir_cl_platforms;
std::vector<cl::Device> muir_cl_devices;
cl::Context muir_cl_context;
cl::Program stage1_program;
cl::Program stage2_program;
cl::Program stage3_program;
cl::Kernel stage1_kernel;
cl::Kernel stage2_kernel;
cl::Kernel stage3_kernel;

void load_file (const std::string &path, std::string &file_contents);
void decode_cl_load_kernels(void);


int process_init_cl(void* opengl_ctx)
{
    cl_int err = CL_SUCCESS;
    
    try
    {
        cl::Platform::get(&muir_cl_platforms);

        std::cout << SectionName << ": # OpenCL of platforms detected: " << muir_cl_platforms.size() << std::endl;

        for(unsigned int i = 0; i < muir_cl_platforms.size(); i++)
        {
            if (MUIR_Verbose)
            {
                std::cout << SectionName << ": Platform[" << i << "] Vendor     : " << muir_cl_platforms[0].getInfo<CL_PLATFORM_VENDOR>() << std::endl;
                std::cout << SectionName << ": Platform[" << i << "] Name       : " << muir_cl_platforms[0].getInfo<CL_PLATFORM_NAME>() << std::endl;
                std::cout << SectionName << ": Platform[" << i << "] Version    : " << muir_cl_platforms[0].getInfo<CL_PLATFORM_VERSION>() << std::endl;
                std::cout << SectionName << ": Platform[" << i << "] Extensions : " << muir_cl_platforms[0].getInfo<CL_PLATFORM_EXTENSIONS>() << std::endl;
            }
        }

        cl_context_properties properties[] = { CL_CONTEXT_PLATFORM, (cl_context_properties)(muir_cl_platforms[0])(), 0};

        muir_cl_context = cl::Context(CL_DEVICE_TYPE_GPU, properties);
        muir_cl_devices = muir_cl_context.getInfo<CL_CONTEXT_DEVICES>();

        std::cout << SectionName << ": Devices detected in context[0]: " << muir_cl_devices.size() << std::endl;
        
        for(unsigned int i = 0; i < muir_cl_devices.size(); i++)
        {
            if (MUIR_Verbose)
            {
                std::cout << SectionName << ": Device[" << i << "] Vendor        : " << muir_cl_devices[0].getInfo<CL_DEVICE_VENDOR>() << std::endl;
                std::cout << SectionName << ": Device[" << i << "] Name          : " << muir_cl_devices[0].getInfo<CL_DEVICE_NAME>() << std::endl;
                std::cout << SectionName << ": Device[" << i << "] Version       : " << muir_cl_devices[0].getInfo<CL_DEVICE_VERSION>() << std::endl;
                std::cout << SectionName << ": Device[" << i << "] Extensions    : " << muir_cl_devices[0].getInfo<CL_DEVICE_EXTENSIONS>() << std::endl;
                std::cout << SectionName << ": Device[" << i << "] Clock Freq    : " << muir_cl_devices[0].getInfo<CL_DEVICE_MAX_CLOCK_FREQUENCY>() << std::endl;
                std::cout << SectionName << ": Device[" << i << "] Compute Units : " << muir_cl_devices[0].getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>() << std::endl;
            }
        }

        std::cout << SectionName << ": " << muir_cl_devices.size() << " device[s] initialized." << std::endl;

        std::cout << SectionName << ": Loading Kernels..." << std::endl;
        decode_cl_load_kernels();
        std::cout << SectionName << ": Kernels loaded." << std::endl;

    }
    catch(...)
    {
        std::cout << SectionName << ": Initialization Failed!" << std::endl;
        return 0;
    }

    // Return the number of OpenCL devices discovered
    return muir_cl_devices.size();
}

void decode_cl_load_kernels(void)
{
    cl_int err = CL_SUCCESS;

    /// Load kernel sources
    std::string stage1_str;
    std::string stage2_str;
    std::string stage3_str;

    try{
        load_file ("stage1-phasecode.cl", stage1_str);
        load_file ("stage2-fft.cl", stage2_str);
        load_file ("stage3-findpeak.cl", stage3_str);
    }
    catch (std::runtime_error error)
    {
        std::cout << SectionName << ": ERROR! - " << error.what() << std::endl;
        std::cout << SectionName << ": Loading kernel sources failed!" << std::endl;
        throw error;
    }

    /// Load and compile stage1 kernel
    cl::Program::Sources stage1_source(1, std::make_pair(stage1_str.c_str(),stage1_str.size()));
    stage1_program = cl::Program(muir_cl_context, stage1_source);
    try{
        stage1_program.build(muir_cl_devices);
    }
    catch(...)
    {
        std::cout << "Build Status: "  << stage1_program.getBuildInfo<CL_PROGRAM_BUILD_STATUS>(muir_cl_devices[0]) << std::endl;
        std::cout <<  "Build Options: " << stage1_program.getBuildInfo<CL_PROGRAM_BUILD_OPTIONS>(muir_cl_devices[0]) << std::endl;
        std::cout <<  "Build Log: "     << stage1_program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(muir_cl_devices[0]) << std::endl;
        throw;
    }
    //stage1_kernel = cl::Kernel(stage1_program, "phasecode", &err);

    /// compile stage2 kernel
    cl::Program::Sources stage2_source(1, std::make_pair(stage2_str.c_str(),stage2_str.size()));
    stage2_program = cl::Program(muir_cl_context, stage2_source);
    try{
        stage2_program.build(muir_cl_devices);
    }
    catch(...)
    {
        std::cout << "Build Status: "  << stage2_program.getBuildInfo<CL_PROGRAM_BUILD_STATUS>(muir_cl_devices[0]) << std::endl;
        std::cout << "Build Options: " << stage2_program.getBuildInfo<CL_PROGRAM_BUILD_OPTIONS>(muir_cl_devices[0]) << std::endl;
        std::cout << "Build Log: "     << stage2_program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(muir_cl_devices[0]) << std::endl;
        throw;
    }
    //stage2_kernel = cl::Kernel(stage2_program, "fft0", &err);

    /// compile stage 3 kernel
    cl::Program::Sources stage3_source(1, std::make_pair(stage3_str.c_str(),stage3_str.size()));
    stage3_program = cl::Program(muir_cl_context, stage3_source);
    try{
        stage3_program.build(muir_cl_devices);
    }
    catch(...)
    {
        std::cout << "Build Status: "  << stage3_program.getBuildInfo<CL_PROGRAM_BUILD_STATUS>(muir_cl_devices[0]) << std::endl;
        std::cout << "Build Options: " << stage3_program.getBuildInfo<CL_PROGRAM_BUILD_OPTIONS>(muir_cl_devices[0]) << std::endl;
        std::cout << "Build Log: "     << stage3_program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(muir_cl_devices[0]) << std::endl;
        throw;
    }
    //stage3_kernel = cl::Kernel(stage3_program, "findpeak", &err);

}



int process_data_cl(int id,
                    const Muir4DArrayF& sample_data,
                    const std::vector<float>& phasecode,
                    Muir3DArrayF& output_data,
                    DecodingConfig &config,
                    std::vector<std::string>& timing_strings,
                    Muir2DArrayD& timings)
{
    boost::timer main_time;
    accumulator_set< double, features< tag::min, tag::mean, tag::max > > acc_setup;
    accumulator_set< double, features< tag::min, tag::mean, tag::max > > acc_copyto;
    accumulator_set< double, features< tag::min, tag::mean, tag::max > > acc_fftw;
    accumulator_set< double, features< tag::min, tag::mean, tag::max > > acc_copyfrom;
    accumulator_set< double, features< tag::count, tag::min, tag::mean, tag::max > > acc_row;

    cl_int err = CL_SUCCESS;
    try 
    {
      boost::timer stage_time;

      // Initialize kernels here since setarg and enque are not threadsafe
      cl::Kernel stage1_kernel(stage1_program, "phasecode", &err);
      cl::Kernel stage2_kernel(stage2_program, "fft0", &err);
      cl::Kernel stage3_kernel(stage3_program, "findpeak", &err);

      // Load Data and Initialize memory
      Muir4DArrayF prefft_data;
      Muir4DArrayF postfft_data;

      // Get Data Dimensions
      const Muir4DArrayF::size_type *array_dims = sample_data.shape();
      
      std::vector<size_t> ex;
      ex.assign( array_dims, array_dims+sample_data.num_dimensions() );
      prefft_data.resize(ex);
      postfft_data.resize(ex);

      Muir4DArrayF::size_type max_sets = array_dims[0];
      Muir4DArrayF::size_type max_cols = array_dims[1];
      Muir4DArrayF::size_type max_range = array_dims[2];

      // Initialize decoded data boost multi_array;
      output_data.resize(boost::extents[max_sets][max_cols][max_range]);

      // Initialize timing structure
      timing_strings.clear();
      timing_strings.push_back("Setup Time");     // 0
      timing_strings.push_back("Phasecode Time"); // 1
      timing_strings.push_back("FFT Time");       // 2
      timing_strings.push_back("Peakfind Time");  // 3
      timing_strings.push_back("Cleanup Time");   // 4
      timing_strings.push_back("Row Total Time"); // 5
      timings.resize(boost::extents[timing_strings.size()][max_range]);
      
      size_t sample_size    = sample_data.num_elements()*sizeof(float);
      size_t phasecode_size = phasecode.size() * sizeof(float);
      size_t output_size    = output_data.num_elements()*sizeof(float);
      
      if (MUIR_Verbose)
      {
        std::cout << SectionName << ": GPU[" << id << "], Sample data - # elements:" << sample_data.num_elements() << std::endl;
        std::cout << SectionName << ": GPU[" << id << "], Output data - # elements:" << prefft_data.num_elements() << std::endl;
        std::cout << SectionName << ": GPU[" << id << "], Sample data - Size      :" << sample_size << std::endl;
        std::cout << SectionName << ": GPU[" << id << "], Phasecode   - Size      :" << phasecode_size << std::endl;
        std::cout << SectionName << ": GPU[" << id << "], Output      - Size      :" << output_size << std::endl;

        for (unsigned int i = 0; i < phasecode.size(); i++)
            std::cout << ((phasecode[i]>0.0f)?1:0);

        std::cout << std::endl;
      }

      std::cout << SectionName << ": GPU[" << id << "] Creating OpenCL arrays" << std::endl;
 
      //our arrays
      cl::Buffer cl_buf_sample    = cl::Buffer(muir_cl_context, CL_MEM_READ_ONLY,  sample_size, NULL, &err);
      cl::Buffer cl_buf_phasecode = cl::Buffer(muir_cl_context, CL_MEM_READ_ONLY,  phasecode_size,  NULL, &err);
      cl::Buffer cl_buf_prefft    = cl::Buffer(muir_cl_context, CL_MEM_READ_WRITE, sample_size, NULL, &err);
      cl::Buffer cl_buf_postfft   = cl::Buffer(muir_cl_context, CL_MEM_READ_WRITE, sample_size, NULL, &err);
      cl::Buffer cl_buf_output    = cl::Buffer(muir_cl_context, CL_MEM_WRITE_ONLY, output_size, NULL, &err);


      cl::Event event;
      cl::CommandQueue queue(muir_cl_context, muir_cl_devices[id], CL_QUEUE_PROFILING_ENABLE, &err);

      std::cout << SectionName << ": GPU[" << id << "] Pushing data to the GPU" << std::endl;

      //push our CPU arrays to the GPU
      err = queue.enqueueWriteBuffer(cl_buf_sample,    CL_TRUE, 0, sample_size,     sample_data.data(), NULL, &event);
      err = queue.enqueueWriteBuffer(cl_buf_phasecode, CL_TRUE, 0, phasecode_size,  &phasecode[0],       NULL, &event);
      err = queue.enqueueWriteBuffer(cl_buf_prefft,    CL_TRUE, 0, sample_size,     prefft_data.data(), NULL, &event);
      err = queue.enqueueWriteBuffer(cl_buf_postfft,   CL_TRUE, 0, sample_size,     postfft_data.data(), NULL, &event);
      err = queue.enqueueWriteBuffer(cl_buf_output,    CL_TRUE, 0, output_size,     output_data.data(), NULL, &event);


      std::cout << SectionName << ": GPU[" << id << "] Load Experiment Data Time: " << stage_time.elapsed() << std::endl;
      stage_time.restart();
      
      float FFT_NSize = 1024.0f;
      float normalize = 1/FFT_NSize;
      int  total_frames = max_sets*max_cols;

      cl::Event stage1_event;
      cl::Event stage2_event;
      cl::Event stage3_event;
      std::vector<cl::Event> waitevents;

      std::vector<cl::Event> stage1_event_list;
      std::vector<cl::Event> stage2_event_list;
      std::vector<cl::Event> stage3_event_list;

      std::cout << SectionName << ": GPU[" << id << "] Processing..." << std::endl;
      for(unsigned int i = 0; i < max_range; i++)
      {
          
          //Setup Stage 1 (Phasecode) Kernel
          err = stage1_kernel.setArg(0, cl_buf_sample);
          err = stage1_kernel.setArg(1, cl_buf_phasecode);
          err = stage1_kernel.setArg(2, cl_buf_prefft);
          err = stage1_kernel.setArg(3, i);
          err = stage1_kernel.setArg(4, (unsigned int)phasecode.size());
          err = stage1_kernel.setArg(5, (unsigned int)max_range);
          //Wait for the command queue to finish these commands before proceeding
          //queue.finish();

          // Setup waiting for stage 3
          waitevents.clear();
          waitevents.push_back(stage3_event);
          
          //Execute Stage 1 (Phasecode) Kernel  (dont wait for events on the first run!)
          err = queue.enqueueNDRangeKernel(stage1_kernel, cl::NullRange, cl::NDRange(phasecode.size(),total_frames), cl::NullRange, (i == 0)?NULL:&waitevents, &stage1_event);
          //queue.finish();

          //Setup Stage 2 (FFT) Kernel
          // __kernel void fft0(__global float2 *in, __global float2 *out, int dir, int S, uint)
          err = stage2_kernel.setArg(0, cl_buf_prefft);
          err = stage2_kernel.setArg(1, cl_buf_postfft);
          err = stage2_kernel.setArg(2, -1);
          err = stage2_kernel.setArg(3, (int)total_frames);
          err = stage2_kernel.setArg(4, (int)max_range);

          // Setup waiting for stage 1
          waitevents.clear();
          waitevents.push_back(stage1_event);
          
          //Execute Stage 2 (FFT) Kernel
          err = queue.enqueueNDRangeKernel(stage2_kernel, cl::NullRange, cl::NDRange(64*total_frames), cl::NDRange(64), &waitevents, &stage2_event);
          //queue.finish();

          //Setup Stage 3 (FindPeak) Kernel
          //
          err = stage3_kernel.setArg(0, cl_buf_postfft);
          err = stage3_kernel.setArg(1, cl_buf_output);
          err = stage3_kernel.setArg(2, i);
          err = stage3_kernel.setArg(3, (int)max_range);
          err = stage3_kernel.setArg(4, (float)normalize);

          // Setup waiting for stage 2
          waitevents.clear();
          waitevents.push_back(stage2_event);
          
          //Execute Stage 3 (FindPeak) Kernel
          err = queue.enqueueNDRangeKernel(stage3_kernel, cl::NullRange, cl::NDRange(total_frames), cl::NullRange, &waitevents, &stage3_event);
          //queue.finish();

          // Get stage events for timing/profiling information
          stage1_event_list.push_back(stage1_event);
          stage2_event_list.push_back(stage2_event);
          stage3_event_list.push_back(stage3_event);
     }

     queue.finish();

     // Get timing information
     cl_ulong start, end;
     for (unsigned int i = 0; i < stage1_event_list.size(); i++)
     {
         timings[0][i] = 0.0; // Startup

         stage1_event_list[i].getProfilingInfo(CL_PROFILING_COMMAND_END, &end);
         stage1_event_list[i].getProfilingInfo(CL_PROFILING_COMMAND_START, &start);
         float stage1exec = (end - start) * 1.0e-9f;
         timings[1][i] = stage1exec; // Phasecode

         stage2_event_list[i].getProfilingInfo(CL_PROFILING_COMMAND_END, &end);
         stage2_event_list[i].getProfilingInfo(CL_PROFILING_COMMAND_START, &start);
         float stage2exec = ((end - start) * 1.0e-9f) ;
         timings[2][i] = stage2exec; // FFT

         stage3_event_list[i].getProfilingInfo(CL_PROFILING_COMMAND_END, &end);
         stage3_event_list[i].getProfilingInfo(CL_PROFILING_COMMAND_START, &start);
         float stage3exec = ((end - start) * 1.0e-9f) ;
         timings[3][i] = stage3exec; // Peakfind

         timings[4][i] = 0.0; // Cleanup
         timings[5][i] = stage1exec + stage2exec + stage3exec; // Row TTL
     }
     
     
     std::cout.precision(10);
     std::cout << SectionName << ": GPU[" << id << "] OpenCL Stage 1+2+3 Time: " << stage_time.elapsed() << std::endl;
     stage_time.restart();

     std::cout << SectionName << ": GPU[" << id << "] Downloading data from GPU..." << std::endl;
      //lets check our calculations by reading from the device memory and printing out the results
      err = queue.enqueueReadBuffer(cl_buf_output, CL_TRUE, 0, output_size, output_data.data(), NULL, &event);
      //err = queue.enqueueReadBuffer(cl_buf_postfft, CL_TRUE, 0, sample_size, postfft_data.data(), NULL, &event);
      //err = queue.enqueueReadBuffer(cl_buf_prefft, CL_TRUE, 0, sample_size, prefft_data.data(), NULL, &event);

      queue.finish();
      event.wait();

    }
    catch (cl::Error err) {
       std::cerr 
          << SectionName
          << ": GPU["
          << id
          << "] ERROR: "
          << err.what()
          << "("
          << err.err()
          << ")"
          << std::endl;
    }

    return EXIT_SUCCESS;
}

void load_file (const std::string &path, std::string &file_contents)
{
    // Load file
    //std::string strpath = path.string();
    std::ifstream file;
    file.open(path.c_str(), std::ios::in | std::ios::ate | std::ios::binary);

    if (file.is_open())
    {
        std::ifstream::pos_type size;

        size = file.tellg();

        file_contents.resize(size);

        file.seekg (0, std::ios::beg);
        file.read (const_cast<char *>(file_contents.c_str()), size);
        file.close();
    }
    else
    {
        throw std::runtime_error("Unable to open file: " + path);
    }
}

