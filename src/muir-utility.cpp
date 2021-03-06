//
// C++ Implementation: muir-utility
//
// Description: Utility functions for manipulating MUIR experiment data.
//
//
// Author: Beau V.C. Bellamy <bvbellamy@arsc.edu>
//         Arctic Region Supercomputing Center
// 
//
//

#include "muir-utility.h"
#include "muir-constants.h"
#include "muir-types.h"

#include <fstream>
#include <cassert>

#include <boost/date_time/posix_time/posix_time.hpp>
namespace BST_PT = boost::posix_time;


// Checks to see if a given time range intersects with that in the file.
bool have_range(const MuirHD5 &file, boost::posix_time::time_period range)
{
    // Read times from HDF5 File.
    Muir2DArrayD time;
    file.read_2D_double(RTI_RADACTIME_PATH, time);

    // Get shape of time array
    const Muir2DArrayUI::size_type *shape = time.shape();

    // Convert time extents to ptime
    BST_PT::ptime t1 = BST_PT::from_time_t(static_cast<int>(time[0][0]/1000000));
    BST_PT::ptime t2 = BST_PT::from_time_t(static_cast<int>(time[shape[0]-1][shape[1]-1]/1000000));

    // Put ptimes into a range
    BST_PT::time_period file_range(t1,t2);

    // Check for intersection
    return range.intersects(file_range);
}


// Reads and parses the phasecode
// Return false if the the file doesn't contain a phasecode.
bool read_phasecode(const MuirHD5 &file_in, std::vector<float> &phasecode)
{
    std::string experimentfile = file_in.read_string(RTI_EXPERIMENTFILE_PATH);

    // Parse experimentfile...
    std::size_t index_min = experimentfile.find(";Code=");
    std::size_t index_max = experimentfile.find("\n\n[Common Parameters]");

    // If our phasecode bounds don't exist, there may be no phasecode.
    if(index_min == std::string::npos || index_max == std::string::npos)
        return false;

    // Extract the section with the phasecode
    std::string phasecode_bulk = experimentfile.substr(index_min,index_max-index_min);
    std::size_t size = phasecode_bulk.length();

    if(size == 0)
        return false;

    // Prepare the phasecode vector.
    phasecode.clear();

    // Get values for phasecode and dump them into the vector.
    for (std::size_t i = 0; i < size; i++)
    {
        if (phasecode_bulk[i] == '+')
        {
            phasecode.push_back(1.0f);
            continue;
        }

        if (phasecode_bulk[i] == '-')
        {
            phasecode.push_back(-1.0f);
            continue;
        }
    }

    return true;
}

// Load a textfile into a string
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
    else
    {
        throw std::runtime_error("Unable to open file: " + path);
    }
}

void print_dimensions(Muir3DArrayF& in)
{
    // Get Data Dimensions
    const Muir3DArrayF::size_type *array_dims = in.shape();
    assert(in.num_dimensions() == 3);

    std::cout << "3D Matrix: " << array_dims[0] << "x" << array_dims[1] << "x" << array_dims[2] << std::endl;
}

void print_dimensions(Muir4DArrayF& in)
{
    // Get Data Dimensions
    const Muir4DArrayF::size_type *array_dims = in.shape();
    assert(in.num_dimensions() == 4);

    std::cout << "4D Matrix: " << array_dims[0] << "x" << array_dims[1] << "x" << array_dims[2] << "x" << array_dims[3] <<std::endl;
}
