
#include "muir-process-cpu.h"
#include "muir-process.h"
#include "muir-global.h"
#include "muir-timer.h"

#include <fftw3.h>

#include <iostream>
#include <complex>


#include <cassert>

// Boost::Accumulators
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/min.hpp>
#include <boost/accumulators/statistics/max.hpp>
#include <boost/accumulators/statistics/count.hpp>
using namespace boost::accumulators;

/// Macros
#ifdef _OPENMP
#include <omp.h>
#else
#define omp_get_thread_num() 0
#define omp_get_num_threads() 1
#endif


/// Constants
static const std::string SectionName("CPU");
static const std::string SectionVersion("0.3");

int process_init_cpu()
{
    // Always return one cpu device (the decoding process is multithreaded with OpenMP)
    return 1;
}

int process_data_cpu(int id,
                     const Muir4DArrayF& sample_data,
                     const std::vector<float>& phasecode,
                     Muir3DArrayF& decoded_data,
                     DecodingConfig &config,
                     std::vector<std::string>& timing_strings,
                     Muir2DArrayD& timings)
{

    // Setup Accumulators For Statistics
    MUIR::Timer main_time;
    //main_time.restart();
    accumulator_set< double, features< tag::min, tag::mean, tag::max > > acc_setup;
    accumulator_set< double, features< tag::min, tag::mean, tag::max > > acc_copyto;
    accumulator_set< double, features< tag::min, tag::mean, tag::max > > acc_fftw;
    accumulator_set< double, features< tag::min, tag::mean, tag::max > > acc_copyfrom;
    accumulator_set< double, features< tag::count, tag::min, tag::mean, tag::max > > acc_row;

    // Get Data Dimensions
    const Muir4DArrayF::size_type *array_dims = sample_data.shape();
    assert(_sample_data.num_dimensions() == 4);

    Muir4DArrayF::size_type max_sets = array_dims[0];
    Muir4DArrayF::size_type max_cols = array_dims[1];
    Muir4DArrayF::size_type num_rangebins = array_dims[2];

    std::size_t phasecode_size = phasecode.size();

    // Initialize decoded data boost multi_array;
    decoded_data.resize(boost::extents[max_sets][max_cols][num_rangebins]);

    // Initialize timing structure
    timing_strings.clear();
    timing_strings.push_back("Setup Time");     // 0
    timing_strings.push_back("Phasecode Time"); // 1 
    timing_strings.push_back("FFT Time");       // 2
    timing_strings.push_back("Peakfind Time");  // 3
    timing_strings.push_back("Cleanup Time");   // 4
    timing_strings.push_back("Row Total Time"); // 5
    timings.resize(boost::extents[timing_strings.size()][num_rangebins]);

    unsigned int fft_size = 1024;  // Also used for normalization

    #pragma omp critical (fftw)
    {
        //fftw_init_threads();
        //fftw_plan_with_nthreads(1);
    }
    
    // Calculate each row
    #pragma omp parallel for
    for(unsigned int phasecode_offset = 0; phasecode_offset < num_rangebins; phasecode_offset++)
    {

        // Write number of threads in config for first pass
        if (phasecode_offset == 0)
            config.threads = omp_get_num_threads();

        // Row Timing
        MUIR::Timer row_time;
        MUIR::Timer stage_time;

        // Setup for row
        fftwf_complex *in, *out;
        fftwf_plan p;

        int N[1] = {fft_size};

        // Display stats from first thread
        int th_id = omp_get_thread_num();
        if ( th_id == 0 && MUIR_Verbose)
            std::cout
                << SectionName << "[" << id << "]"
                << ": Progress:" << static_cast<float>(count(acc_row))/static_cast<float>(num_rangebins)*100.0 << "%"
                << "  (Mean Timings [s]) Row TTL: " << mean(acc_row)
                << ", Setup: " << mean(acc_setup)
                << ", Copy/Phase/Zero: " << mean(acc_copyto)
                << ", FFTW: " << mean(acc_fftw)
                << ", FindPeak: " << mean(acc_copyfrom)
                << ", Rows/Sec: " << static_cast<float>(count(acc_row))/main_time.elapsed()
                << ", Threads: " << omp_get_num_threads()
                << std::endl;

        #pragma omp critical (fftw)
            {
                in  = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * fft_size*max_sets*max_cols);
                out = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * fft_size*max_sets*max_cols);
                p = fftwf_plan_many_dft(1, N, max_sets*max_cols, in, NULL, 1, fft_size, out, NULL, 1, fft_size, FFTW_FORWARD, FFTW_MEASURE | FFTW_DESTROY_INPUT);
            }

            // Timing Startup [0]
            acc_setup(stage_time.elapsed());
            timings[0][phasecode_offset] = stage_time.elapsed();
            stage_time.restart();

            // Copy data into fftw vector, apply phasecode, and zero out the rest
            for(Muir4DArrayF::size_type fft_row = 0; fft_row < fft_size; fft_row++)
            {
                if (fft_row > phasecode_size || (phasecode_offset + fft_row) > num_rangebins)
                {
                    for(std::size_t set = 0; set < max_sets; set++)
                        for(std::size_t col = 0; col < max_cols; col++)
                        {
                            std::size_t index = set*(fft_size*max_cols) + col*(fft_size) + fft_row;
                            in[index][0] = 0;
                            in[index][1] = 0;
                        }
                }
                else
                {
                    float phase_multiplier = phasecode[fft_row];

                    for(Muir4DArrayF::size_type set = 0; set < max_sets; set++)
                        for(Muir4DArrayF::size_type col = 0; col < max_cols; col++)
                        {
                            Muir4DArrayF::size_type index = set*(fft_size*max_cols) + col*(fft_size) + fft_row;
                            in[index][0] = sample_data[set][col][fft_row+phasecode_offset][0] * phase_multiplier;
                            in[index][1] = sample_data[set][col][fft_row+phasecode_offset][1] * phase_multiplier;
                        }
                }

            }

            // Timing Phasecode [1]
            acc_copyto(stage_time.elapsed());
            timings[1][phasecode_offset] = stage_time.elapsed();
            stage_time.restart();

            // Execute FFTW
            fftwf_execute(p);

            // Timing FFT [2]
            acc_fftw(stage_time.elapsed());
            timings[2][phasecode_offset] = stage_time.elapsed();
            stage_time.restart();

            // Output FFTW data
            for(std::size_t set = 0; set < max_sets; set++)
            {
                for(std::size_t col = 0; col < max_cols; col++)
                {
                    //fftw_complex max_value = {0.0,0.0};
                    float        max_power = 0.0;

                    // Iterate through the column spectra and find the max value
                    for(std::size_t row = 0; row < fft_size; row++)
                    {
                        std::size_t index = set*(fft_size*max_cols) + col*(fft_size) + row;

                        // Skip sqrt when taking magnitude, but save it for later.
                        float power = powf(out[index][0],2) + powf(out[index][1],2);
                        if (power > max_power)
                        {
                            //max_value[0] = out[index][0];
                            //max_value[1] = out[index][1];
                            max_power = power;
                        }

                    }

                    // Assign and normalize
                    decoded_data[set][col][phasecode_offset] = sqrt(max_power)/fft_size;
  
            }
        }

        // Timing peakfind [3]
        acc_copyfrom(stage_time.elapsed());
        timings[3][phasecode_offset] = stage_time.elapsed();
        stage_time.restart();

        #pragma omp critical (fftw)
        {
            fftwf_destroy_plan(p);
            fftwf_free(in);
            fftwf_free(out);
        }

        // Timing Cleanup [4]
        timings[4][phasecode_offset] = stage_time.elapsed();

        // Timing Row [5]
        timings[5][phasecode_offset] = row_time.elapsed();
        acc_row(row_time.elapsed());
    }

    #pragma omp critical (fftw)
    fftwf_cleanup_threads();

    std::cout << "Done!" << std::endl;
    std::cout << "Rows completed: " << count(acc_row) << std::endl;
    std::cout << " Row Min  : " << min(acc_row) << std::endl;
    std::cout << " Row Mean : " << mean(acc_row) << std::endl;
    std::cout << " Row Max  : " << max(acc_row) << std::endl;
    std::cout << "Phase Timings... " << std::endl;
    std::cout << " Phase 1 (Setup) Min  : " << min(acc_setup) << std::endl;
    std::cout << " Phase 1 (Setup) Mean : " << mean(acc_setup) << std::endl;
    std::cout << " Phase 1 (Setup) Max  : " << max(acc_setup) << std::endl;
    std::cout << " Phase 2 (Copy/Phase/Zero) Min  : " << min(acc_copyto) << std::endl;
    std::cout << " Phase 2 (Copy/Phase/Zero) Mean : " << mean(acc_copyto) << std::endl;
    std::cout << " Phase 2 (Copy/Phase/Zero) Max  : " << max(acc_copyto) << std::endl;
    std::cout << " Phase 3 (FFTW) Min  : " << min(acc_fftw) << std::endl;
    std::cout << " Phase 3 (FFTW) Mean : " << mean(acc_fftw) << std::endl;
    std::cout << " Phase 3 (FFTW) Max  : " << max(acc_fftw) << std::endl;
    std::cout << " Phase 4 (FindPeak) Min  : " << min(acc_copyfrom) << std::endl;
    std::cout << " Phase 4 (FindPeak) Mean : " << mean(acc_copyfrom) << std::endl;
    std::cout << " Phase 4 (FindPeak) Max  : " << max(acc_copyfrom) << std::endl;

    // Fill out config
    //config.threads = 1; this is done earlier in the OpenMP context.
    config.fft_size = fft_size;
    config.decoding_time = main_time.elapsed();
    config.platform = std::string("CPU");
    config.process = std::string("CPU Decoding (single precision) Process Version: ") + SectionVersion;
    config.phasecode_muting = 0;
    config.time_integration = 0;

    return 0;
}

