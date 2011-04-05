//
// C++ Implementation: muir-timer
//
// Description: A simple high resolution timer using posix high resolution timers
//
//
// Author: Beau V.C. Bellamy <bvbellamy@arsc.edu>
//         Arctic Region Supercomputing Center
//
//
//

#include "muir-timer.h"

//#include <boost/date_time/posix_time/posix_time.hpp>
#include <ctime>
#include <stdexcept>
#include <cassert>

namespace MUIR
{

//namespace BST_PT = boost::posix_time;

Timer::Timer()
: _starttime(gettime())
{
}

double
Timer::elapsed()
{
    //BST_PT::ptime stoptime(BST_PT::microsec_clock::local_time());
    //BST_PT::time_duration diff (stoptime.time_of_day() - _starttime.time_of_day());
    //return (diff.total_milliseconds())/1000.0;
    timespec stoptime, difftime;

    stoptime = gettime();

    difftime = diff(_starttime, stoptime);

    double elapsed_time = static_cast<double>(difftime.tv_sec) + static_cast<double>(difftime.tv_nsec)/1.0e9;

    assert(elapsed_time >= 0);

    return difftime.tv_sec + difftime.tv_nsec/1.0e9;
}

void
Timer::restart()
{
    //_starttime = BST_PT::microsec_clock::local_time();
    _starttime = gettime();
}

timespec Timer::diff(const timespec &start, const timespec &end) const
{
    timespec temp;
    if ((end.tv_nsec-start.tv_nsec)<0) {
        temp.tv_sec = end.tv_sec-start.tv_sec-1;
        temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
    } else {
        temp.tv_sec = end.tv_sec-start.tv_sec;
        temp.tv_nsec = end.tv_nsec-start.tv_nsec;
    }
    return temp;
}

timespec Timer::gettime()
{
    timespec current_time;

    int ret = clock_gettime(CLOCK_MONOTONIC, &current_time);
    if (ret != 0)
        throw std::runtime_error("Unable to read high resolution clock!");

    return current_time;
}

} // Namespace MUIR
