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

#include <boost/date_time/posix_time/posix_time.hpp>

namespace MUIR
{

namespace BST_PT = boost::posix_time;

class Timer
{

  public:
    Timer();

    double elapsed();
    void   restart();

  private:
    BST_PT::ptime _starttime;

};


} // Namespace MUIR

#endif // MUIR_TIMER_H