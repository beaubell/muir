#ifndef MUIR_PLOT_H
#define MUIR_PLOT_H
//
// C++ Interface: muir-plot
//
// Description: Plot MUIR experiment data into a png file.
//
//
// Author: Beau V.C. Bellamy <bvbellamy@arsc.edu>
//         Arctic Region Supercomputing Center
//
//
//

#include "muir-data.h"

void save_2dplot(const MuirData &data, const std::string &output_file);
void save_fftw_2dplot(const MuirData &data, const std::string &output_file);



#endif //MUIR_PLOT_H