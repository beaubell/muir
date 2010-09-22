//
// C++ Interface: muir-data
//
// Description: 
//
//
// Author: Beau V.C. Bellamy <bvbellamy@alaska.edu>, (C) 2010
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include <string>
#include "H5Cpp.h"

extern const std::string PULSEWIDTH_PATH;
extern const std::string BAUDLENGTH_PATH;

class MuirData
{
   public:
    MuirData(const std::string &filename_in);
    ~MuirData();

   private:
    std::string _filename;
    H5::H5File      _h5file;
    float _pulsewidth;
    float _txbaud;

    //float read_pulsewidth();
    //float read_txbaud();

    float       read_scalar_float(const H5std_string &dataset_name);
    std::string read_string(const H5std_string &dataset_name);

    std::string read_phasecode();
};
