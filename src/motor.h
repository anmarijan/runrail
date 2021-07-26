#ifndef MOTOR_H
#define MOTOR_H
////////////////////////////////////////////////////////////////////////////////
#include "nlohmann/json.hpp"
////////////////////////////////////////////////////////////////////////////////
class Motor {
public:
    int id;
	double max_power;      // Max power (kW)
    double Volt;           // Voltage (1100V)
	int n_pole;            //P/2
	double power_coef1;    // Power efficiency rate at Point-1
    double power_coef2;    // Power efficiency rate at Point-2
    double fb1;            // Input frequency at Point-1 (maximum)
    double fb2;            // Frequency at Point-2
    double fs1;            // Slip freqcuency at Point-1
    double fs2;            // Slip frequency at Point-2
    double Force;          // Necessary force
// train
	double full_speed;     // maximum speed
	double diameter;       //Diameter of wheels (mm)
	double gear;           //Reduction ratio
private:
    double F0;             // Max traction (N)
    double fi1;            // Input frequency at Point-1 (after adjustment).
	double velo1;          // (km/h) at Point-1
	double velo2;          // (km/h) at the point between const-power and special area
    double coef_a;
    double coef_t;         // coeffient: Torque = coef_t *
	double sp2f;           // speed (km/h ) -> inverter frequency
public:
    Motor();
    void init(double F);
    double tract(double sp) const;
    double power_ton(double sp) const;   // power(kW) per ton
    double getVelo1() const { return velo1; }
    double getVelo2() const { return velo2; }
    bool read_json(const nlohmann::json& jdata);
    void print();
private:
    double torque(double fi, double fs);
};

#endif
