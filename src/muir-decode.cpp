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

struct Flags
{
    bool option_range;
    BST_PT::time_period range;

    Flags()
    : option_range(false),
      range(BST_PT::ptime(BST_DT::neg_infin),BST_PT::ptime(BST_DT::pos_infin))
    {}
};

// Prototypes
void print_help (void);
void process_expfiles(std::vector<fs::path> files, const Flags& flags);  // No reference, want copies
void process_decfiles(std::vector<fs::path> files, const Flags& flags);  // No reference, want copies
void cull_files_range(std::vector<fs::path> &files, const Flags& flags);

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
    process_init(NULL);

    if (flags.option_range)
    {
        cull_files_range(files, flags);
    }

    for (int i = 0; i < static_cast<int>(files.size()); i++)
    {
        std::string expfile =  files[i].string();

        // Strips two levels of .blah.h5
        std::string base =  fs::basename(fs::basename(files[i]));

        // Loading file
        std::cout << "Loading Experiment Data: " << expfile << std::endl;
        MuirData data(expfile);

        std::cout << "Decoding: " << expfile << std::endl;
        data.decode();

        std::string datafile = base + std::string("-decoded.h5");
        std::cout << "Saving decoded data: " << datafile << std::endl;
        data.save_decoded_data(datafile);

    }
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


void print_help ()
{
    std::cout << "usage: muir-decode [--range yyyymmddThhmmss yyyymmddThhmmss] hdf5files... " << std::endl;
    std::cout << "  --range          : Only process files that fall within a specified ISO date range in GMT." << std::endl;
}
