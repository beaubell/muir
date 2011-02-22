
#include "muir-process-cpu.h"
#include "muir-global.h"

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

// Boost::Timers
#include <boost/timer.hpp>
using boost::timer;

/// Macros
#ifdef _OPENMP
#include <omp.h>
#else
#define omp_get_thread_num() 0
#define omp_get_num_threads() 1
#endif

#define FFTW_type float


/// Constants
static const std::string SectionName("CPU");

int process_init_cpu()
{
    // Always return one cpu device (the decoding process is multithreaded with OpenMP)
    return 1;
}

int process_data_cpu(int id, const Muir4DArrayF& sample_data, const std::vector<float>& phasecode, Muir3DArrayF& decoded_data)
{

    // Setup Accumulators For Statistics
    boost::timer main_time;
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
    Muir4DArrayF::size_type max_rows = array_dims[2];
    
    std::size_t phasecode_size = phasecode.size();
    
    // Initialize decoded data boost multi_array;
    decoded_data.resize(boost::extents[max_sets][max_cols][max_rows]);
    
    #pragma omp critical (fftw)
    {
        fftw_init_threads();
        fftw_plan_with_nthreads(2);
    }
    
    // Calculate each row
    #pragma omp parallel for
    for(unsigned int phase_code_offset = 0; phase_code_offset < max_rows; phase_code_offset++)
    {
        // Row Timing
        boost::timer row_time;
        boost::timer stage_time;
        
        // Setup for row
        fftw_complex *in, *out;
        fftw_plan p;
        unsigned int fft_size = max_rows;  // Also used for normalization
        int N[1] = {fft_size};
        
        // Display stats from first thread
        int th_id = omp_get_thread_num();
        if ( th_id == 0 && MUIR_Verbose)
            std::cout
                << SectionName << "[" << id << "]"
                << ": Progress:" << static_cast<float>(count(acc_row))/static_cast<float>(max_rows)*100.0 << "%"
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
                in  = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * fft_size*max_sets*max_cols);
                out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * fft_size*max_sets*max_cols);
                p = fftw_plan_many_dft(1, N, max_sets*max_cols, in, NULL, 1, fft_size, out, NULL, 1, fft_size, FFTW_FORWARD, FFTW_MEASURE | FFTW_DESTROY_INPUT);
            }
            
            // Timing
            acc_setup(stage_time.elapsed());
            stage_time.restart();
            
            // Copy data into fftw vector, apply phasecode, and zero out the rest
            for(Muir4DArrayF::size_type row = 0; row < fft_size; row++)
            {
                if((row >= phase_code_offset) && (row < (phasecode_size + phase_code_offset)))
                {
                    float phase_multiplier = phasecode[row-phase_code_offset];
                    
                    for(Muir4DArrayF::size_type set = 0; set < max_sets; set++)
                        for(Muir4DArrayF::size_type col = 0; col < max_cols; col++)
                        {
                            Muir4DArrayF::size_type index = set*(fft_size*max_cols) + col*(fft_size) + row;
                            in[index][0] = sample_data[set][col][row][0] * phase_multiplier;
                            in[index][1] = sample_data[set][col][row][1] * phase_multiplier;
                        }
                }
                else // ZEROS!
            {
                for(std::size_t set = 0; set < max_sets; set++)
                    for(std::size_t col = 0; col < max_cols; col++)
                    {
                        std::size_t index = set*(fft_size*max_cols) + col*(fft_size) + row;
                        in[index][0] = 0;
                        in[index][1] = 0;
                    }
            }
            }
            
            // Timing
            acc_copyto(stage_time.elapsed());
            stage_time.restart();
            
            // Execute FFTW
            fftw_execute(p);
            
            // Timing
            acc_fftw(stage_time.elapsed());
            stage_time.restart();
            
            // Output FFTW data
            for(std::size_t set = 0; set < max_sets; set++)
            {
                for(std::size_t col = 0; col < max_cols; col++)
                {
                    //fftw_complex max_value = {0.0,0.0};
                    double        max_power = 0.0;
                    
                    // Iterate through the column spectra and find the max value
                    for(std::size_t row = 0; row < fft_size; row++)
                    {
                        std::size_t index = set*(fft_size*max_cols) + col*(fft_size) + row;
                        
                        // Skip sqrt when taking magnitude, but save it for later.
                        double power = pow(out[index][0],2) + pow(out[index][1],2);
                        if (power > max_power)
                        {
                            //max_value[0] = out[index][0];
                            //max_value[1] = out[index][1];
                            max_power = power;
                        }
                        
                    }
                    
                    // Assign and normalize
                    decoded_data[set][col][phase_code_offset] = sqrt(max_power)/fft_size;
                    //decoded_data[set][col][phase_code_offset] = max_power/fft_size;
            }
        }

        // Timing
        acc_copyfrom(stage_time.elapsed());
        acc_row(row_time.elapsed());

        #pragma omp critical (fftw)
        {
            fftw_destroy_plan(p);
            fftw_free(in);
            fftw_free(out);
        }
    }

    #pragma omp critical (fftw)
    fftw_cleanup_threads();

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

    return 0;
}
