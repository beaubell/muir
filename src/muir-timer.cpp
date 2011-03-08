//
// C++ Implementation: muir-timer
//
// Description: A simple high resolution timer using boost
//
//
// Author: Beau V.C. Bellamy <bvbellamy@arsc.edu>
//         Arctic Region Supercomputing Center
//
//
//

#include "muir-timer.h"

#include <boost/date_time/posix_time/posix_time.hpp>

namespace MUIR
{

namespace BST_PT = boost::posix_time;

Timer::Timer()
: _starttime(BST_PT::microsec_clock::local_time())
{
}

double
Timer::elapsed()
{
    BST_PT::ptime stoptime(BST_PT::microsec_clock::local_time());
    BST_PT::time_duration diff (stoptime.time_of_day() - _starttime.time_of_day());
    return (diff.total_milliseconds())/1000.0;
}

void
Timer::restart()
{
    _starttime = BST_PT::microsec_clock::local_time();
}

} // Namespace MUIR
