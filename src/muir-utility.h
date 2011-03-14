#ifndef MUIR_UTILITY_H
#define MUIR_UTILITY_H
//
// C++ Declaration: muir-utility
//
// Description: Utility functions for manipulating MUIR experiment data.
//
//
// Author: Beau V.C. Bellamy <bvbellamy@arsc.edu>
//         Arctic Region Supercomputing Center
// 
//
//

#include "muir-hd5.h"
#include <boost/date_time/posix_time/posix_time.hpp>

// Checks to see if a given time range intersects with that in the file.
bool have_range(const MuirHD5 &file, boost::posix_time::time_period range);

// Reads and parses the phasecode from an HDF5 file.
// Return false if the the file doesn't contain a phasecode.
bool read_phasecode(const MuirHD5 &file_in, std::vector<float> &phasecode);

// Load a textfile into a string
void load_file (const std::string &path, std::string &file_contents);

#endif // MUIR_UTILITY
