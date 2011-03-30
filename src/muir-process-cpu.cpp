
#include "muir-process-cpu.h"
#include "muir-process.h"
#include "muir-global.h"
#include "muir-timer.h"

#include <fftw3.h>

#include <iostream>
#include <complex>
#include <cstring>
#include <algorithm>


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
static const std::string SectionName("CPU/OpenMP");
static const std::string ProcessVersion("0.4");
static const std::string ProcessString("CPU Decoding (single precision) Process");

void apply_phasecode(const unsigned int range_offset,
                     const Muir4DArrayF &in_buffer,
                     const std::vector<float>& phasecode,
                     Muir4DArrayF &out_buffer,
                     unsigned int colomn_chunk_size,
                     unsigned int column_num);

void find_peak(const unsigned int range_offset,
               const Muir4DArrayF &in_buffer,
               Muir3DArrayF &out_buffer,
               unsigned int colomn_chunk_size,
               unsigned int column_num);



// Initialize CPU devices for decoding
int process_init_cpu()
{
    // Always return one cpu device (the decoding process is multithreaded with OpenMP)
    return 1;
}

// CPU Decoding Routine
int process_data_cpu(int id,
                     const Muir4DArrayF& sample_data,
                     const std::vector<float>& phasecode,
                     Muir3DArrayF& decoded_data,
                     DecodingConfig &config,
                     std::vector<std::string>& timing_strings,
                     Muir2DArrayD& timings,
                     Muir4DArrayF& complex_intermediate
                    )
{

    /// Setup Accumulators For Statistics
    MUIR::Timer main_time;
    accumulator_set< double, features< tag::min, tag::mean, tag::max > > acc_setup;
    accumulator_set< double, features< tag::min, tag::mean, tag::max > > acc_copyto;
    accumulator_set< double, features< tag::min, tag::mean, tag::max > > acc_fftw;
    accumulator_set< double, features< tag::min, tag::mean, tag::max > > acc_copyfrom;
    accumulator_set< double, features< tag::count, tag::min, tag::mean, tag::max > > acc_row;

    /// Get Data Dimensions
    const Muir4DArrayF::size_type *array_dims = sample_data.shape();
    assert(_sample_data.num_dimensions() == 4);

    Muir4DArrayF::size_type max_sets = array_dims[0];
    Muir4DArrayF::size_type max_cols = array_dims[1];
    Muir4DArrayF::size_type num_rangebins = array_dims[2];

    /// Initialize timing structure
    timing_strings.clear();
    timing_strings.push_back("Setup Time");     // 0
    timing_strings.push_back("Phasecode Time"); // 1 
    timing_strings.push_back("FFT Time");       // 2
    timing_strings.push_back("Peakfind Time");  // 3
    timing_strings.push_back("Cleanup Time");   // 4
    timing_strings.push_back("Row Total Time"); // 5
    timings.resize(boost::extents[timing_strings.size()][num_rangebins]);

    /// Configure FFT Size
    unsigned int fft_size = config.fft_size;  // Also used for normalization

    /// Configure References
    unsigned int start_row = config.intermediate_row;
    unsigned int end_row = num_rangebins;
    
    const Muir4DArrayF& sample_data_ref = sample_data;
    //const Muir4DArrayF& post_integration_ref = sample_data;  // Since we don't Implement time integration yet.

    if (!(config.intermediate_stage == STAGE_ALL))
    {
        /// Setup for Intermediate Data Gathering
        start_row = config.intermediate_row;
        end_row = config.intermediate_row + 1;

        /// Initialize intermediate data structure;
        if (config.intermediate_stage == STAGE_TIMEINTEGRATION)
        {
            complex_intermediate.resize(boost::extents[max_sets][max_cols][num_rangebins][2]);
            //post_integration_ref = complex_intermediate;
        }

    }
    else
    {
        /// Initialize decoded data boost multi_array;
        decoded_data.resize(boost::extents[max_sets][max_cols][num_rangebins]);
    }

    /// Time Integration
    //time_integration(sample_data_ref, post_integration_ref, 0);
    if (config.intermediate_stage == STAGE_TIMEINTEGRATION)
        return 0;


    // Calculate each row
    #pragma omp parallel for
    for(unsigned int phasecode_offset = start_row; phasecode_offset < end_row; phasecode_offset++)
    {
        // Row Timing
        MUIR::Timer row_time;
        MUIR::Timer stage_time;

        // Write number of threads in config for first pass
        if (phasecode_offset == start_row)
            config.threads = omp_get_num_threads();

        /// Configure References
        Muir4DArrayF fft_in  = Muir4DArrayF(boost::extents[1][1][1][2]);
        Muir4DArrayF fft_out = Muir4DArrayF(boost::extents[1][1][1][2]);
        Muir4DArrayF& fft_in_ref = fft_in;   // Defaults
        Muir4DArrayF& fft_out_ref = fft_out; // Defaults

        switch(config.intermediate_stage)
        {
            case STAGE_ALL:
                fft_in_ref = fft_in;
                fft_out_ref = fft_out;
                break;
            case STAGE_PHASECODE:
                fft_in_ref = complex_intermediate;
                fft_out_ref = fft_out;
                break;
            case STAGE_POSTFFT:
                fft_in_ref = fft_in;
                fft_out_ref = complex_intermediate;
                break;
            case STAGE_POWER:
                // NOT USED
                throw std::logic_error("process_cpu_data(): STAGE_POWER is not handled in this process.");
                break;
            case STAGE_TIMEINTEGRATION:
                // Uh oh!
                throw std::logic_error("process_cpu_data(): Something bad happenend, we should not be handling time integration here.");
                break;
        }

        // Allocate Memory
        fft_in_ref.resize(boost::extents[max_sets][max_cols][fft_size][2]);
        fft_out_ref.resize(boost::extents[max_sets][max_cols][fft_size][2]);

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
            p = fftwf_plan_many_dft(1, N, max_sets*max_cols, (float (*)[2])fft_in_ref.data(), NULL, 1, fft_size, (float (*)[2])fft_out_ref.data(), NULL, 1, fft_size, FFTW_FORWARD, FFTW_MEASURE | FFTW_DESTROY_INPUT);
        }

        // Timing Startup [0]
        acc_setup(stage_time.elapsed());
        timings[0][phasecode_offset] = stage_time.elapsed();
        stage_time.restart();

        // Apply Phasecode
        apply_phasecode(phasecode_offset, sample_data_ref, phasecode, fft_in_ref, 0 ,0);

        // Timing Phasecode [1]
        acc_copyto(stage_time.elapsed());
        timings[1][phasecode_offset] = stage_time.elapsed();
        stage_time.restart();

        // Execute FFTW
        if (!(config.intermediate_stage == STAGE_PHASECODE))
            fftwf_execute(p);

        // Timing FFT [2]
        acc_fftw(stage_time.elapsed());
        timings[2][phasecode_offset] = stage_time.elapsed();
        stage_time.restart();

        // Execute Peak Finding
        if (!(config.intermediate_stage == STAGE_PHASECODE || config.intermediate_stage == STAGE_POSTFFT))
            find_peak(phasecode_offset, fft_out_ref, decoded_data, 0 ,0);

        // Timing peakfind [3]
        acc_copyfrom(stage_time.elapsed());
        timings[3][phasecode_offset] = stage_time.elapsed();
        stage_time.restart();

        #pragma omp critical (fftw)
        {
            fftwf_destroy_plan(p);
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
    config.platform = SectionName;
    config.device = std::string("Unknown CPU");
    config.process = ProcessString;
    config.process_version = ProcessVersion;
    config.phasecode_muting = 0;
    config.time_integration = 0;

    return 0;
}


void apply_phasecode(const unsigned int range_offset,
                const Muir4DArrayF &in_buffer,
                const std::vector<float>& phasecode,
                Muir4DArrayF &out_buffer,
                unsigned int colomn_chunk_size,
                unsigned int column_num)
{
    // Determine Strides
    const Muir4DArrayF::size_type *in_dims = in_buffer.shape();
    Muir4DArrayF::size_type in_sets = in_dims[0];
    Muir4DArrayF::size_type in_cols = in_dims[1];
    Muir4DArrayF::size_type in_rangebins = in_dims[2];

    const Muir4DArrayF::size_type *out_dims = out_buffer.shape();
    Muir4DArrayF::size_type out_sets = out_dims[0];
    Muir4DArrayF::size_type out_cols = out_dims[1];
    Muir4DArrayF::size_type out_rangebins = out_dims[2];

    size_t phasecode_size = phasecode.size();

    // Check for error conditions
    if(in_sets != out_sets || in_cols != out_cols)
        throw std::logic_error("apply_phasecode(): Critical dimensions of in and out buffers do not match!");

    if(range_offset >= in_rangebins)
        throw std::logic_error("apply_phasecode(): requested range falls outside of range for input buffer");

    // Zero output buffer
    std::fill(out_buffer.data(), out_buffer.data() + out_buffer.num_elements(), 0.0f);

    // Copy data into fftw vector, apply phasecode, and zero out the rest
    for(Muir4DArrayF::size_type out_row = 0; out_row < out_rangebins; out_row++)
    {
        // Check if we are within non-zero output range
        if (!(out_row > phasecode_size || (range_offset + out_row) > in_rangebins))
        {
            float phase_multiplier = phasecode[out_row];

            for(Muir4DArrayF::size_type set = 0; set < out_sets; set++)
                for(Muir4DArrayF::size_type col = 0; col < out_cols; col++)
                {
                    out_buffer[set][col][out_row][0] = in_buffer[set][col][out_row+range_offset][0] * phase_multiplier;
                    out_buffer[set][col][out_row][1] = in_buffer[set][col][out_row+range_offset][1] * phase_multiplier;
                }
        }
    }

}


void find_peak(const unsigned int range_offset,
                     const Muir4DArrayF &in_buffer,
                     Muir3DArrayF &out_buffer,
                     unsigned int colomn_chunk_size,
                     unsigned int column_num)
{
    // Determine Strides
    const Muir4DArrayF::size_type *in_dims = in_buffer.shape();
    Muir4DArrayF::size_type in_sets = in_dims[0];
    Muir4DArrayF::size_type in_cols = in_dims[1];
    Muir4DArrayF::size_type fft_size = in_dims[2];

    const Muir4DArrayF::size_type *out_dims = out_buffer.shape();
    Muir4DArrayF::size_type out_sets = out_dims[0];
    Muir4DArrayF::size_type out_cols = out_dims[1];
    Muir4DArrayF::size_type out_rangebins = out_dims[2];

    // Check for error conditions
    if(in_sets != out_sets || in_cols != out_cols)
        throw std::logic_error("find_peak(): Critical dimensions of in and out buffers do not match!");

    if(range_offset >= out_rangebins)
        throw std::logic_error("find_peak(): requested range falls outside of range for output buffer");

    // Loop through each pulse frame (set and col)
    for(std::size_t set = 0; set < in_sets; set++)
    {
        for(std::size_t col = 0; col < in_cols; col++)
        {
            float max_power = 0.0f;

            // Iterate through the column spectra and find the max value
            for(std::size_t row = 0; row < fft_size; row++)
            {
                float power = powf(in_buffer[set][col][row][0],2) + powf(in_buffer[set][col][row][1],2);

                if (power > max_power)
                {
                    max_power = power;
                }

            }

            // Assign and normalize
            out_buffer[set][col][range_offset] = sqrtf(max_power)/static_cast<float>(fft_size);

        }
    }

}

