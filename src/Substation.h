/*
Substations
*/
#ifndef SUBSTATION_H
#define SUBSTATION_H

#include <stdio.h>
#include <vector>
#include <string>
/**
 * \brief A class representing a substation
 */
class Substation {
public:
    int id;
    std::string name;
    double distance;    // Location in m from the beginning
public:
    Substation();
    double power;  //The present power (kW)
    double max_power;  // Max power so far
    double getMaxPower() const { return max_power; };
};

/**
 * \brief List (vector) of substation
 */
class SubstationList {
    FILE* fp;
    std::vector<Substation> data;
public:
    SubstationList(){fp=NULL;};
    ~SubstationList();
    bool log_start(const char* fname);
    void log_end();
    void add(Substation& s);
    Substation& get(unsigned int i);
    void set_power(int tm, double x, double power_kW);
    void reset();
    void print_max_power();
    bool read(const char* fname);
    void write(const char* fname, int s_tm, int e_tm, bool flag);
};

#endif