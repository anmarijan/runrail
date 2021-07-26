#ifndef COMMON_H
#define COMMON_H

constexpr double GRAV_ACC = 9.80665;
constexpr double M_PI = 3.14159265358979323846;
constexpr double M_1_PI	= 0.31830988618379067154;

struct Control {
    static double station_time;  // Default stopping time at stations (s)
};

#endif
