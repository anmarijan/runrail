#include <cassert>
#include "Substation.h"

SubstationList::~SubstationList() {
    if(fp) fclose(fp);
}
void SubstationList::add(Substation& s) {
    data.push_back(s);
}

Substation& SubstationList::get(unsigned int i) {
    return data[i];
}
// the member "power" of each Substation should be initialized somewhere else.
void SubstationList::set_power(int tm, double x, double power_kW) {
    unsigned int nSubstations = data.size();
    if( nSubstations == 0 ) {
        return;
    } else if ( nSubstations == 1) {
        data[0].power += power_kW;
    } else {
        bool check = false;
        for(unsigned int i=1; i < nSubstations; i++) {
            if ( data[i-1].distance <= x && x < data[i].distance) {
                double length = data[i].distance - data[i-1].distance;
                assert(length > 0);
                double rate =  (x-data[i-1].distance)/length;
                double rval = power_kW * rate;
                double lval = power_kW - rval;
                data[i-1].power += lval;
                data[i].power += rval;
                check = true;
                break;
            }
        }
        if ( check == false ) {
            if( x < data[0].distance ) data[0].power += power_kW;
            else if ( x >= data[nSubstations-1].distance) data[nSubstations-1].power += power_kW;
        }
    }
}

void SubstationList::reset() {
    for(auto& x : data) {
        x.power = 0;
    }
}

bool SubstationList::log_start(const char* fname) {
    if( fp != NULL ) fclose(fp);
    errno_t err = fopen_s(&fp,fname,"wt");
    if( err == -1 ) return false;
    return true;
}

void SubstationList::log_end() {
    if( fp != NULL ) fclose(fp);
}