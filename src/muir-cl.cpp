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

// Boost::Timers
#include <boost/timer.hpp>
using boost::timer;


#include "muir-hd5.h"
#include "muir-utility.h"
#include "muir-constants.h"

void load_file (const std::string &path, std::string &file_contents);

int
main(int argc, const char* argv[])
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

      /// Load and compile stage1 kernel
      //boost::timer row_time;
      boost::timer stage_time;

      std::string stage1_str;
      load_file ("stage1-phasecode.cl", stage1_str); 
      cl::Program::Sources stage1_source(1,
          std::make_pair(stage1_str.c_str(),stage1_str.size()));
      cl::Program stage1_program = cl::Program(context, stage1_source);
      try{
        stage1_program.build(devices);
      }
      catch(...)
      {
         std::cout << "Build Status: "  << stage1_program.getBuildInfo<CL_PROGRAM_BUILD_STATUS>(devices[0]) << std::endl;
        std::cout <<  "Build Options: " << stage1_program.getBuildInfo<CL_PROGRAM_BUILD_OPTIONS>(devices[0]) << std::endl;
        std::cout <<  "Build Log: "     << stage1_program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(devices[0]) << std::endl;
         throw;        
      }  
      cl::Kernel stage1_kernel(stage1_program, "phasecode", &err);

      /// Load and compile stage2 kernel
      std::string stage2_str;
      load_file ("stage2-fft.cl", stage2_str);
      cl::Program::Sources stage2_source(1,
          std::make_pair(stage2_str.c_str(),stage2_str.size()));
      cl::Program stage2_program = cl::Program(context, stage2_source);
      try{
        stage2_program.build(devices);
      }
      catch(...)
      {
         std::cout << "Build Status: "  << stage2_program.getBuildInfo<CL_PROGRAM_BUILD_STATUS>(devices[0]) << std::endl;
         std::cout << "Build Options: " << stage2_program.getBuildInfo<CL_PROGRAM_BUILD_OPTIONS>(devices[0]) << std::endl;
         std::cout << "Build Log: "     << stage2_program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(devices[0]) << std::endl;
         throw;
      } 
      cl::Kernel stage2_kernel(stage2_program, "fft0", &err);

      /// Load and compile stage 3 kernel
      std::string stage3_str;
      load_file ("stage3-findpeak.cl", stage3_str);
      cl::Program::Sources stage3_source(1,
          std::make_pair(stage3_str.c_str(),stage3_str.size()));
      cl::Program stage3_program = cl::Program(context, stage3_source);
      try{
        stage3_program.build(devices);
      }
      catch(...)
      {
         std::cout << "Build Status: "  << stage3_program.getBuildInfo<CL_PROGRAM_BUILD_STATUS>(devices[0]) << std::endl;
         std::cout << "Build Options: " << stage3_program.getBuildInfo<CL_PROGRAM_BUILD_OPTIONS>(devices[0]) << std::endl;
         std::cout << "Build Log: "     << stage3_program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(devices[0]) << std::endl;
         throw;
      } 
      cl::Kernel stage3_kernel(stage3_program, "findpeak", &err);


      std::cout << "Load and Compile Kernel Time: " << stage_time.elapsed() << std::endl;
      stage_time.restart(); 

      // Load Data and Initialize memory
      std::string filename(argv[1]);
      MuirHD5 file_in(filename, H5F_ACC_RDONLY);

      Muir4DArrayF sample_data;
      Muir4DArrayF prefft_data;
      Muir4DArrayF postfft_data;
      Muir3DArrayF output_data;
      std::vector<float> phasecode;

      // Read Phasecode
      if (!read_phasecode(file_in, phasecode))
        std::cout << "File: " << filename << ", doesn't contain a phase code!" << std::endl;

      // Read in experiment data
      std::cout << "Reading file: " << filename << std::endl;     
      file_in.read_4D_float (RTI_RAWSAMPLEDATA_PATH , sample_data);

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
      output_data.resize(boost::extents[10][500][1100]);
 

      std::cout << "Sample data - # elements:" << sample_data.num_elements() << std::endl;
      std::cout << "Output data - # elements:" << prefft_data.num_elements() << std::endl;
      size_t sample_size    = sample_data.num_elements()*sizeof(float);
      size_t phasecode_size = phasecode.size() * sizeof(float);
      size_t output_size    = output_data.num_elements()*sizeof(float);

      std::cout << "Sample data - Size      :" << sample_size << std::endl;
      std::cout << "Phasecode   - Size      :" << phasecode_size << std::endl;
      std::cout << "Output      - Size      :" << output_size << std::endl;

      for (unsigned int i = 0; i < phasecode.size(); i++)
        std::cout << ((phasecode[i]>0.0f)?1:0);

      std::cout << std::endl;

      printf("Creating OpenCL arrays\n");
 
      //our arrays
      cl::Buffer cl_buf_sample    = cl::Buffer(context, CL_MEM_READ_ONLY,  sample_size, NULL, &err);
      cl::Buffer cl_buf_phasecode = cl::Buffer(context, CL_MEM_READ_ONLY,  phasecode_size,  NULL, &err);
      cl::Buffer cl_buf_prefft    = cl::Buffer(context, CL_MEM_READ_WRITE, sample_size, NULL, &err);
      cl::Buffer cl_buf_postfft   = cl::Buffer(context, CL_MEM_READ_WRITE, sample_size, NULL, &err);
      cl::Buffer cl_buf_output    = cl::Buffer(context, CL_MEM_WRITE_ONLY, output_size, NULL, &err);


      cl::Event event;
      cl::CommandQueue queue(context, devices[0], 0, &err);

      printf("Pushing data to the GPU\n");
      //push our CPU arrays to the GPU
      err = queue.enqueueWriteBuffer(cl_buf_sample,    CL_TRUE, 0, sample_size,     sample_data.data(), NULL, &event);
      err = queue.enqueueWriteBuffer(cl_buf_phasecode, CL_TRUE, 0, phasecode_size,  &phasecode[0],       NULL, &event);
      err = queue.enqueueWriteBuffer(cl_buf_prefft,    CL_TRUE, 0, sample_size,     prefft_data.data(), NULL, &event);
      err = queue.enqueueWriteBuffer(cl_buf_postfft,   CL_TRUE, 0, sample_size,     postfft_data.data(), NULL, &event);
      err = queue.enqueueWriteBuffer(cl_buf_output,    CL_TRUE, 0, output_size,     output_data.data(), NULL, &event);


      std::cout << "Load Experiment Data Time: " << stage_time.elapsed() << std::endl;
      stage_time.restart();

      std::cout << "Processing..." << std::endl;
      for(unsigned int i = 0; i < 1100; i++)
      {
          //Setup Stage 1 (Phasecode) Kernel
          err = stage1_kernel.setArg(0, cl_buf_sample);
          err = stage1_kernel.setArg(1, cl_buf_phasecode);
          err = stage1_kernel.setArg(2, cl_buf_prefft);
          err = stage1_kernel.setArg(3, i);
          err = stage1_kernel.setArg(4, (unsigned int)phasecode.size());
          err = stage1_kernel.setArg(5, 1100);
          //Wait for the command queue to finish these commands before proceeding
          //queue.finish();

          //Execute Stage 1 (Phasecode) Kernel
          err = queue.enqueueNDRangeKernel(stage1_kernel, cl::NullRange, cl::NDRange(phasecode.size(),5000), cl::NullRange, NULL, &event);
          //queue.finish();

          //Setup Stage 2 (FFT) Kernel
          // __kernel void fft0(__global float2 *in, __global float2 *out, int dir, int S)
          err = stage2_kernel.setArg(0, cl_buf_prefft);
          err = stage2_kernel.setArg(1, cl_buf_postfft);
          err = stage2_kernel.setArg(2, -1);
          err = stage2_kernel.setArg(3, 5000);

          //Execute Stage 2 (FFT) Kernel
          err = queue.enqueueNDRangeKernel(stage2_kernel, cl::NullRange, cl::NDRange(320000), cl::NDRange(64), NULL, &event);
          //queue.finish();

          //Setup Stage 3 (FindPeak) Kernel
          //
          err = stage3_kernel.setArg(0, cl_buf_postfft);
          err = stage3_kernel.setArg(1, cl_buf_output);
          err = stage3_kernel.setArg(2, i);
          err = stage3_kernel.setArg(3, 1100);
          err = stage3_kernel.setArg(4, 1.0f/1024.0f);

          //Execute Stage 3 (FindPeak) Kernel
          err = queue.enqueueNDRangeKernel(stage3_kernel, cl::NullRange, cl::NDRange(5000), cl::NullRange, NULL, &event);
          //queue.finish();
         
     }

     queue.finish();

     std::cout.precision(10);
     std::cout << "OpenCL Stage 1+2+3 Time: " << stage_time.elapsed() << std::endl;
     stage_time.restart();

     std::cout << "Downloading data from GPU..." << std::endl;
      //lets check our calculations by reading from the device memory and printing out the results
      err = queue.enqueueReadBuffer(cl_buf_output, CL_TRUE, 0, output_size, output_data.data(), NULL, &event);
      //err = queue.enqueueReadBuffer(cl_buf_postfft, CL_TRUE, 0, sample_size, postfft_data.data(), NULL, &event);
      //err = queue.enqueueReadBuffer(cl_buf_prefft, CL_TRUE, 0, sample_size, prefft_data.data(), NULL, &event);

      queue.finish();

      //for(int i=0; i < 20; i++)
      //{
      //    printf(" input[%d] = %10.8f,%0.8f\n", i, sample_data[0][0][i][0], sample_data[0][0][i][1]);
      //    printf("output[%d] = %10.8f,%0.8f\n", i, prefft_data[0][0][i][0], prefft_data[0][0][i][1]);
      //}

      std::cout << "Writing out data to file..." << std::endl;
      MuirHD5 file_out("out.hd5", H5F_ACC_TRUNC);
      
      // Create group
      file_out.createGroup(RTI_DECODEDDIR_PATH);

      // Prepare and write decoded sample data
      file_out.write_3D_float(RTI_DECODEDDATA_PATH, output_data);
      //file_out.write_4D_float(RTI_DECODEDDATA_PATH, postfft_data);

      Muir2DArrayF  _sample_range;
      Muir2DArrayUI _framecount;
      Muir2DArrayD  _time;

      // Copy range data
      file_in.read_2D_float (RTI_RAWSAMPLERANGE_PATH, _sample_range);
      file_out.write_2D_float(RTI_DECODEDRANGE_PATH, _sample_range);

      // Prepare and write radac data
      file_in.read_2D_double(RTI_RADACTIME_PATH     , _time);
      file_out.write_2D_double(RTI_DECODEDRADAC_PATH, _time);

      // Prepare and write framecount data
      file_in.read_2D_uint  (RTI_RAWFRAMECOUNT_PATH , _framecount);
      file_out.write_2D_uint(RTI_DECODEDFRAME_PATH, _framecount);

      file_in.close();
      file_out.close();



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

        size = file.tellg();

        file_contents.resize(size);

        file.seekg (0, std::ios::beg);
        file.read (const_cast<char *>(file_contents.c_str()), size);
        file.close();
    }
}

