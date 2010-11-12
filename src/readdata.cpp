// 
//
//
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/convenience.hpp>

#ifndef H5_NO_NAMESPACE
#ifndef H5_NO_STD
    using std::cout;
    using std::endl;
#endif  // H5_NO_STD
#endif

#include "muir-data.h"

#include "H5Cpp.h"

#ifndef H5_NO_NAMESPACE
    using namespace H5;
#endif

namespace fs = boost::filesystem;

struct Flags
{
    bool option_plot;
    bool option_decode;
    bool option_decode_load;
    bool option_decode_plot;
    
    Flags()
    :option_plot(false),
     option_decode(false),
     option_decode_load(false),
     option_decode_plot(false) {}
};

// Prototypes
void print_help (void);
void process_expfiles(const std::vector<fs::path> &files, const Flags& flags);
void process_decfiles(const std::vector<fs::path> &files, const Flags& flags);

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
               /// FIXME
               //std::cout << dirI->path(). << std::endl;
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


void process_expfiles(const std::vector<fs::path> &files, const Flags& flags)
{
    for (int i = 0; i < static_cast<int>(files.size()); i++)
    {
        std::string expfile =  files[i].string();

       // Strips two levels of .blah.h5
        std::string base =  fs::basename(fs::basename(files[i]));

       // OK.. This is in place to mitigate a race condition between libHDF5 and openmp
        sleep(rand() % 10);

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


void process_decfiles(const std::vector<fs::path> &files, const Flags& flags)
{
    for (int i = 0; i < static_cast<int>(files.size()); i++)
    {
        std::string decfile =  files[i].string();

       // Strips two levels of .blah.h5
        std::string base =  fs::basename(fs::basename(files[i]));

       // OK.. This is in place to mitigate a race condition between libHDF5 and openmp
        sleep(rand() % 10);

       // Loading file
        std::cout << "Loading Decoded Data: " << decfile << std::endl;
        MuirData data(decfile, 1);

        if (flags.option_decode_plot)
        {
            std::string plotfile = base + std::string("-decoded.png");
            std::cout << "Generating decoding plot: " << plotfile << std::endl;
            //data.save_fftw_2dplot(plotfile);
        }
    }
}


void print_help ()
{
    std::cout << "usage: readdata [--plot] [--decode-load | --decode [--decode-plot]] hdf5files " << std::endl;
    std::cout << "  --plot       : Generate a PNG file from data." << std::endl;
    std::cout << "  --decode     : Decode data and save to a HDF5 file." << std::endl;
    std::cout << "  --decode-load: Load decoded data from HDF5 file." << std::endl;
    std::cout << "  --decode-plot: Generate a PNG file from decoded data." << std::endl;
}
