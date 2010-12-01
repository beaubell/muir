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

#include "muir-data.h"
#include "muir-hd5.h"
#include "muir-utility.h"

namespace fs = boost::filesystem;
namespace BST_PT = boost::posix_time;
namespace BST_DT = boost::date_time;
using namespace boost::gregorian;

struct Flags
{
    bool option_plot;
    bool option_decode;
    bool option_decode_load;
    bool option_decode_plot;
    bool option_range;
    BST_PT::time_period range;

    Flags()
    :option_plot(false),
     option_decode(false),
     option_decode_load(false),
     option_decode_plot(false),
     option_range(false),
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
       if (!strcmp(argv[argi],"--plot"))
       {
           flags.option_plot = true;
           continue;
       }
       if (!strcmp(argv[argi],"--plot-powermin"))
       {
           argi++;
           float power = strtof(argv[argi], argv[argi+1]);
       }
       if (!strcmp(argv[argi],"--plot-powermax"))
       {
           argi++;
           float power = strtof(argv[argi], argv[argi+1]);
       }
       if (!strcmp(argv[argi],"--decode"))
       {
           flags.option_decode = true;
           continue;
       }
       if (!strcmp(argv[argi],"--decode-load"))
       {
           flags.option_decode_load = true;
           continue;
       }
       if (!strcmp(argv[argi],"--decode-plot"))
       {
           flags.option_decode_plot = true;
           continue;
       }
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

   if (flags.option_decode_load)
   {
       process_decfiles(files, flags);
   }
   else
   {
       process_expfiles(files, flags);
   }
   return 0;  // successfully terminated
}


void process_expfiles(std::vector<fs::path> files, const Flags& flags)
{
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

       // Save plot
        if (flags.option_plot)
        {
            std::string plotfile = base + std::string(".png");
            std::cout << "Generating plot: " << plotfile << std::endl;
            data.save_2dplot(plotfile);
        }

        if (flags.option_decode)
        {
            std::cout << "Decoding: " << expfile << std::endl;
            data.process_fftw();;

            std::string datafile = base + std::string("-decoded.h5");
            std::cout << "Saving decoded data: " << datafile << std::endl;
            data.save_decoded_data(datafile);
        }

        if (flags.option_decode_plot)
        {
            std::string plotfile = base + std::string("-decoded.png");
            std::cout << "Generating decoding plot: " << plotfile << std::endl;
            data.save_fftw_2dplot(plotfile);
        }
    }
}


void process_decfiles(std::vector<fs::path> files, const Flags& flags)
{
    if (flags.option_range)
    {
        cull_files_range(files, flags);
    }

    for (int i = 0; i < static_cast<int>(files.size()); i++)
    {
        std::string decfile =  files[i].string();

       // Strips two levels of .blah.h5
        std::string base =  fs::basename(files[i]);

       // Loading file
        std::cout << "Loading Decoded Data: " << decfile << std::endl;
        MuirData data(decfile, 1);

        if (flags.option_decode_plot)
        {
            std::string plotfile = base + std::string(".png");
            std::cout << "Generating decoding plot: " << plotfile << std::endl;
            data.save_fftw_2dplot(plotfile);
        }
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
    std::cout << "usage: readdata [--range yyyymmddThhmmss yyyymmddThhmmss] [--plot] " << std::endl;
    std::cout << "                [--decode-load | --decode [--decode-plot]] hdf5files " << std::endl;
    std::cout << "  --plot        : Generate a PNG file from data." << std::endl;
    std::cout << "  --decode      : Decode data and save to a HDF5 file." << std::endl;
    std::cout << "  --decode-load : Load decoded data from HDF5 file." << std::endl;
    std::cout << "  --decode-plot : Generate a PNG file from decoded data." << std::endl;
    std::cout << "  --range       : Only process files that fall within a specified ISO date range in GMT." << std::endl;
}
