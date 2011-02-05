#define __CL_ENABLE_EXCEPTIONS
 
#if defined(__APPLE__) || defined(__MACOSX)
#include <OpenCL/cl.hpp>
#else
#include <CL/cl.hpp>
#endif
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <fstream>

#include "muir-hd5.h"
#include "muir-utility.h"
#include "muir-constants.h"

void load_file (const std::string &path, std::string &file_contents);

int
main(void)
{
    
    cl_int err = CL_SUCCESS;
    try 
    {
      std::vector<cl::Platform> platforms;
      cl::Platform::get(&platforms);
      if (platforms.size() == 0) {
          std::cout << "Platform size 0\n";
          return -1;
      }

      cl_context_properties properties[] = 
         { CL_CONTEXT_PLATFORM, (cl_context_properties)(platforms[0])(), 0};
      cl::Context context(CL_DEVICE_TYPE_GPU, properties); 
 
      std::vector<cl::Device> devices = context.getInfo<CL_CONTEXT_DEVICES>();

      std::string stage1;
      load_file ("stage1-phasecode.cl", stage1); 
      cl::Program::Sources source(1,
          std::make_pair(stage1.c_str(),stage1.size()));
      cl::Program program_ = cl::Program(context, source);
      try{
        program_.build(devices);
      }
      catch(...)
      {
         std::cout << "Build Status: " << program_.getBuildInfo<CL_PROGRAM_BUILD_STATUS>(devices[0]) << std::endl;
        std::cout << "Build Options:\t" << program_.getBuildInfo<CL_PROGRAM_BUILD_OPTIONS>(devices[0]) << std::endl;
        std::cout << "Build Log:\t " << program_.getBuildInfo<CL_PROGRAM_BUILD_LOG>(devices[0]) << std::endl;
         throw;        
      }  
      cl::Kernel kernel(program_, "phasecode", &err);

      // Load Data and Initialize memory
      std::string filename("/scratch/bellamy/20081024.001/d0000648.dt0.h5");
      MuirHD5 file_in(filename, H5F_ACC_RDONLY);

      Muir4DArrayF sample_data;
      Muir4DArrayF prefft_data; 
      std::vector<int> phasecode;

      // Read Phasecode
      if (!read_phasecode(file_in, phasecode))
        std::cout << "File: " << filename << ", doesn't contain a phase code!" << std::endl;

      // Read in experiment data
      std::cout << "Reading file: " << filename << std::endl;     
      file_in.read_4D_float (RTI_RAWSAMPLEDATA_PATH , sample_data);
      std::vector<size_t> ex;
      const size_t* shape = sample_data.shape();
      ex.assign( shape, shape+sample_data.num_dimensions() );
      prefft_data.resize(ex);

      std::cout << "Sample data - # elements:" << sample_data.num_elements() << std::endl;
      std::cout << "PreFFT data - # elements:" << prefft_data.num_elements() << std::endl;
      size_t sample_size = sample_data.num_elements()*sizeof(float);
      size_t phasecode_size = 4;
      float *b = new float(9);
      std::cout << "Sample data - Size      :" << sample_size << std::endl;

      printf("Creating OpenCL arrays\n");
 
      //our arrays
      cl::Buffer cl_buf_sample    = cl::Buffer(context, CL_MEM_READ_ONLY,  sample_size, NULL, &err);
      cl::Buffer cl_buf_phasecode = cl::Buffer(context, CL_MEM_READ_ONLY,  phasecode_size,  NULL, &err);
      cl::Buffer cl_buf_prefft    = cl::Buffer(context, CL_MEM_WRITE_ONLY, sample_size, NULL, &err);

      cl::Event event;
      cl::CommandQueue queue(context, devices[0], 0, &err);


      printf("Pushing data to the GPU\n");
      //push our CPU arrays to the GPU
      err = queue.enqueueWriteBuffer(cl_buf_sample,    CL_TRUE, 0, sample_size, sample_data.data(), NULL, &event);
      err = queue.enqueueWriteBuffer(cl_buf_phasecode, CL_TRUE, 0, phasecode_size,  b, NULL, &event);
      err = queue.enqueueWriteBuffer(cl_buf_prefft,    CL_TRUE, 0, sample_size, prefft_data.data(), NULL, &event);

      err = kernel.setArg(0, cl_buf_sample);
      err = kernel.setArg(1, cl_buf_phasecode);
      err = kernel.setArg(2, cl_buf_prefft);
      err = kernel.setArg(3, 1);
      err = kernel.setArg(4, 10);
      err = kernel.setArg(5, 1100);
      //Wait for the command queue to finish these commands before proceeding
      queue.finish();

      //for now we make the workgroup size the same as the number of elements in our arrays
      //workGroupSize[0] = num;

      printf("in runKernel\n");
      //execute the kernel
      err = queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(1100,5000), cl::NullRange, NULL, &event);
      //printf("clEnqueueNDRangeKernel: %s\n", oclErrorString(err));
      queue.finish();

      //lets check our calculations by reading from the device memory and printing out the results
      err = queue.enqueueReadBuffer(cl_buf_prefft, CL_TRUE, 0, sample_size, prefft_data.data(), NULL, &event);
      //printf("clEnqueueReadBuffer: %s\n", oclErrorString(err));

      for(int i=0; i < 20; i++)
      {
          printf(" input[%d] = %10.8f,%0.8f\n", i, sample_data[0][0][i][0], sample_data[0][0][i][1]);
          printf("output[%d] = %10.8f,%0.8f\n", i, prefft_data[0][0][i][0], prefft_data[0][0][i][1]);
      }

      MuirHD5 file_out("out.hd5", H5F_ACC_TRUNC);
      file_out.write_4D_float ("prefft" , prefft_data);

      event.wait();
    }
    catch (cl::Error err) {
       std::cerr 
          << "ERROR: "
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
        char * memblock;

        size = file.tellg();
        //memblock = new char [size];
        file_contents.resize(size);

        file.seekg (0, std::ios::beg);
        file.read (const_cast<char *>(file_contents.c_str()), size);
        file.close();
    }
}

