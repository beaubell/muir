// 
//
//
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/convenience.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/local_time_adjustor.hpp>
#include <boost/date_time/c_local_time_adjustor.hpp>
#include <boost/lexical_cast.hpp>

#include "muir-data.h"
#include "muir-hd5.h"
#include "muir-utility.h"
#include "muir-process.h"

namespace fs = boost::filesystem;
namespace BST_PT = boost::posix_time;
namespace BST_DT = boost::date_time;
using namespace boost::gregorian;
using boost::lexical_cast;

#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
struct Flags
{
    bool option_dec_cpu;
    bool option_dec_cuda;
    bool option_dec_opencl;
    bool option_range;
    BST_PT::time_period range;

    Flags()
    : option_dec_cpu(false),
      option_dec_cuda(false),
      option_dec_opencl(false),
      option_range(false),
      range(BST_PT::ptime(BST_DT::neg_infin),BST_PT::ptime(BST_DT::pos_infin))
    {}
};

// Prototypes
void print_help (void);
void process_expfiles(std::vector<fs::path> files, const Flags& flags);  // No reference, want copies
void process_decfiles(std::vector<fs::path> files, const Flags& flags);  // No reference, want copies
void cull_files_range(std::vector<fs::path> &files, const Flags& flags);
void process_thread(int id, std::vector<fs::path> files, int *position);

int main (const int argc, const char * argv[])
{

    // There must at least be a file specified
    if ( argc < 2 )
    {
        print_help();
        return 1;
    }

    std::vector<fs::path> files;

    Flags flags;

    for (int argi = 1; argi < argc; argi++)
    {
        // Check for commands
        if (!strcmp(argv[argi],"--range"))  // Expects two more arguments
        {
            BST_PT::ptime t1,t2;

           // Get first date
            try
            {
                argi++;
                t1 = BST_PT::from_iso_string(argv[argi]);
            }
            catch(...)
            {
                std::cout << "ERROR! Bad Date: " << argv[argi] << std::endl;
                return 1;
            }

           // Get Second date
            try
            {
                argi++;
                t2 = BST_PT::from_iso_string(argv[argi]);
            }
            catch(...)
            {
                std::cout << "ERROR! Bad Date: " << argv[argi] << std::endl;
                return 1;
            }

           // Date range specified
            flags.option_range = true;
            flags.range = BST_PT::time_period(t1, t2);

            continue;
        }
        if (!strcmp(argv[argi],"--cpu"))
        {
            flags.option_dec_cpu = true;
            continue;
        }
        if (!strcmp(argv[argi],"--gpu-cuda"))
        {
            flags.option_dec_cuda = true;
            continue;
        }
        if (!strcmp(argv[argi],"--gpu-opencl"))
        {
            flags.option_dec_opencl = true;
            continue;
        }
        if (!strcmp(argv[argi],"--help") || !strcmp(argv[argi],"-h"))
        {
            print_help();
            return 0;
        }

        fs::path path1(argv[argi]);

       // If not a command, must be a file
        if (fs::is_directory(path1))
        {

            for (fs::directory_iterator dirI(path1); dirI!=fs::directory_iterator(); ++dirI)
            {
               //std::cout << dirI->string() << std::endl;

                if (!fs::is_directory(*dirI))
                    files.push_back(*dirI);
               /// FIXME Doesn't scan higher directories
            }
        }
        else
        {
           // Just add this file
            files.push_back(path1);
        }
    }

    process_expfiles(files, flags);

    return 0;  // successfully terminated
}


void process_expfiles(std::vector<fs::path> files, const Flags& flags)
{
    // Initialize decoding context
    int methods = flags.option_dec_opencl*MUIR_DECODE_GPU_OPENCL | flags.option_dec_cuda*MUIR_DECODE_GPU_CUDA | flags.option_dec_cpu*MUIR_DECODE_CPU;
    int devices = process_init(methods, NULL);
    if(devices == 0)
    {
        std::cout << "NO DEVICES FOUND!" << std::endl;
        return;
    }
    else
    {
        std::cout << "Processing devices initialized: " << devices << std::endl;
    }
 
    if (flags.option_range)
    {
        cull_files_range(files, flags);
    }

    int position = 0;

    // Create threads
    //for (int i = 0; i < 2)); i++)
    //{
        boost::thread thread1(boost::bind(process_thread, 0, files, &position));
        boost::thread thread2(boost::bind(process_thread, 1, files, &position));
    //}

    thread1.join();
    thread2.join();
}



void cull_files_range(std::vector<fs::path> &files, const Flags& flags)
{
    std::cout << "Scanning for files in range: " << BST_PT::to_simple_string(flags.range) << std::endl;
    for(std::vector<fs::path>::iterator iter = files.begin(); iter != files.end(); iter++)
    {
        try
        {
            MuirHD5 file_in(iter->string(), H5F_ACC_RDONLY);
            if(have_range(file_in, flags.range))
            {
            }
            else
            {
                iter = files.erase(iter);
                iter--;  // Not sure this is a good idea, but seems to work even before the first element.
            }
        }
        catch(...)
        {
            std::cout << "Bad file: " << iter->string() << std::endl;
            iter = files.erase(iter);
            iter--;
        }
    }

}

boost::mutex thread_mutex;
//boost::shared_lock threadlock(m);

void process_thread(int id, std::vector<fs::path> files, int *position)
{
    int i = 0;
    {
        boost::mutex::scoped_lock lock(thread_mutex);
        if (id != 0)
            i = ++(*position);
    }

    while (i < static_cast<int>(files.size()))
    {
        
        std::string expfile =  files[i].string();

        // Strips .h5 from file
        std::string base = fs::basename(files[i]);

        // Loading file
        MuirData *data;
        {
            boost::mutex::scoped_lock lock(thread_mutex);
            std::cout << "Loading Experiment Data: " << expfile << std::endl;
            data = new MuirData(expfile);
        }

        std::cout << "Decoding: " << expfile << std::endl;
        data->decode(id);

        std::string datafile = base + std::string(".decoded.h5");
        {
            boost::mutex::scoped_lock lock(thread_mutex);
            std::cout << "Saving decoded data: " << datafile << std::endl;
            data->save_decoded_data(datafile);
            delete data;
 
            i = ++(*position);
        }

    }
}



void print_help ()
{
    std::cout << "usage: muir-decode [--range yyyymmddThhmmss yyyymmddThhmmss] hdf5files... " << std::endl;
    std::cout << "  --range          : Only process files that fall within a specified ISO date range in GMT." << std::endl;
    std::cout << "  --gpu-cuda       : Force GPU CUDA decoding method." << std::endl;
    std::cout << "  --gpu-opencl     : Froce GPU OpenCL decoding method." << std::endl;
    std::cout << "  --cpu            : Force CPU decoding method. (May be combined with one other gpu method)" << std::endl; 
}
