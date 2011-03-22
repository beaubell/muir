#ifndef MUIR_TIMER_H
#define MUIR_TIMER_H
//
// C++ Declaration: timer
//
// Description: Functions for getting timing information
//
//
// Author: Beau V.C. Bellamy <bvbellamy@arsc.edu>
//         Arctic Region Supercomputing Center
//
//

#include <ctime>

namespace MUIR
{

//namespace BST_PT = boost::posix_time;

class Timer
{

  public:
    Timer();

    double elapsed();
    void   restart();

  private:
    //BST_PT::ptime _starttime;
    timespec _starttime;
    timespec diff(const timespec &start, const timespec &end) const;
    timespec gettime();
};


} // Namespace MUIR

#endif // MUIR_TIMER_H