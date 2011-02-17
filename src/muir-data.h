#ifndef MUIR_DATA_H
#define MUIR_DATA_H
//
// C++ Interface: muir-data
//
// Description: Read and store MUIR experiment data.
//
//
// Author: Beau V.C. Bellamy <bvbellamy@arsc.edu>
//         Arctic Region Supercomputing Center
//
//
//

#include "H5Cpp.h"
#include <string>
#include <vector>

#include "muir-hd5.h"

typedef boost::multi_array<float , 4> SampleDataArray;
typedef boost::multi_array<float , 3> DecodedDataArray;

class MuirData
{
   private:
    std::string _filename;
    float       _pulsewidth;
    float       _txbaud;

    void        print_onesamplecolumn(float (&sample)[1100][2], float (&range)[1100]);
    std::vector<float> _phasecode;

    Muir4DArrayF _sample_data;
    Muir3DArrayF _decoded_data;

    Muir2DArrayF  _sample_range;
    Muir2DArrayUI _framecount;
    Muir2DArrayD  _time;

   public:
    MuirData(const std::string &filename_in, int option = 0);
    virtual ~MuirData();

    void print_onesamplecolumn(const std::size_t run, const std::size_t column);
    void print_stats();

    int  decode(int id = 0);
    void save_decoded_data(const std::string &output_file);
    void read_decoded_data(const std::string &input_file);

    // read only accessors
    const Muir4DArrayF&  get_sample_data() const
        { return _sample_data; };
    const Muir3DArrayF&  get_decoded_data() const
        { return _decoded_data; };
    const Muir2DArrayF&  get_sample_range() const
        { return _sample_range; };
    const Muir2DArrayUI& get_framecount() const
        { return _framecount; };
    const Muir2DArrayD&  get_time() const
        { return _time; };
    const std::string& get_filename() const
        { return _filename; };

   private:
    // No copying
    MuirData(const MuirData &in);
    MuirData& operator= (const MuirData &right);

};

#endif // #ifndef MUIR_DATA_H
