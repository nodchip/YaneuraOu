#ifndef APERY_TIME_HPP
#define APERY_TIME_HPP

#include <string>

namespace time_util
{
  std::string formatRemainingTime(
    double startClockSec,
    int currentIndex,
    int numberOfData);
};

#endif
