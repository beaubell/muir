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

#define BOOST_DISABLE_ASSERTS 1
#define NDEBUG 1
#include "boost/multi_array.hpp"

#include "H5Cpp.h"
#include <string>
#include <vector>

#include "muir-hd5.h"

class MuirData
{
   public:
    MuirData(const std::string &filename_in, int option = 0);
    virtual ~MuirData();

    void print_onesamplecolumn(const std::size_t run, const std::size_t column);
    void print_stats();
    void save_2dplot(const std::string &output_file);
    void save_fftw_2dplot(const std::string &output_file);

    void process_fftw();
    void save_decoded_data(const std::string &output_file);
    void read_decoded_data(const std::string &input_file);

   private:
	// No copying
	MuirData(const MuirData &in);
	MuirData& operator= (const MuirData &right);
	
	std::string _filename;
    float       _pulsewidth;
    float       _txbaud;

    void        print_onesamplecolumn(float (&sample)[1100][2], float (&range)[1100]);
    std::vector<int> _phasecode;

    typedef boost::multi_array<float , 4> SampleDataArray;
    SampleDataArray _sample_data;

    typedef boost::multi_array<float , 3> DecodedDataArray;
    DecodedDataArray _decoded_data;

	Muir2DArrayF  _sample_range;
	Muir2DArrayUI _framecount;
    Muir2DArrayD  _time;



};

#endif // #ifndef MUIR_DATA_H
