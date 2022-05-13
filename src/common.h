#ifndef COMMON_H
#define COMMON_H

#define _USE_MATH_DEFINES
#include <cmath>
constexpr double GRAV_ACC = 9.80665;

struct Control {
    static double station_time;  // Default stopping time at stations (s)
};

#endif
