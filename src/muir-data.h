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

extern const std::string PULSEWIDTH_PATH;
extern const std::string BAUDLENGTH_PATH;
extern const std::string EXPERIMENTFILE_PATH;
extern const std::string RADACTIME_PATH;
extern const std::string SAMPLEDATA_PATH;
extern const std::string SAMPLERANGE_PATH;


class MuirData
{
   public:
    MuirData(const std::string &filename_in);
    ~MuirData();

    void print_onesamplecolumn(const std::size_t run, const std::size_t column);
    void print_stats();
    void save_2dplot(const std::string &output_file);

   private:
    std::string _filename;
    H5::H5File  _h5file;
    float       _pulsewidth;
    float       _txbaud;

    float       read_scalar_float(const H5std_string &dataset_name);
    std::string read_string(const H5std_string &dataset_name);

    void        read_phasecode(void);
    void        read_times(void);
    void        read_sampledata(void);
    void        read_samplerange(void);
	void        read_framecount(void);

    void        print_onesamplecolumn(float (&sample)[1100][2], float (&range)[1100]);
    std::vector<int> _phasecode;

    typedef float (*SampleDataArray)[10][500][1100][2];
    SampleDataArray _sample_data;
    typedef float (*SampleRangeArray)[1][1100];
    SampleRangeArray _sample_range;
	typedef unsigned int (*FrameCountArray)[10][500];
    FrameCountArray _framecount;

    double _time[10][2];



};