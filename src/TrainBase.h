#ifndef TRAIN_BASE_H
#define TRAIN_BASE_H

#include <string>
#include "nlohmann/json.hpp"
////////////////////////////////////////////////////////////////////////////////
// default values
#define ACCELERATION 0.83
#define DECELERATION 1.0
#define DEFJERK 0.6
#define COASTING 0.1
////////////////////////////////////////////////////////////////////////////////
enum class RollingResistance {None, Quadratic, JNR_EMU_MT, JNR_EMU};
//-----------------------------------------------------------------------------
// TrainBase class
//-----------------------------------------------------------------------------
class TrainBase {
public:
    int id;
    std::string name;
    int speed_traction_index; // Index to a speed-Traction relationship
    int motor_index;
    int n_traction_units;     // Traction power is multiplied by this value
    int line_index;           // Index to a line data
public: // Property values of this train
    int nCars;               // No. of cars
    double length;           // Length of this train (m)
    double weight;           // ton
    double WM, WT;           // WT=Weith of trailer cars, WM=motor cars (ton)
    double max_speed;        // Maximum speed of this train (km/h)
    double fixed_acc;        // accelerations (m/s^2)
    double dec, coast;       // decelerations (m/s^2)
    double jerk;             // jerk (m/s^3)
    double res_coefs[10];    // Each train keeps coefficients of resistance model
    double start_resist;     // Starting rolling resistance (N/t)
    double start_resist_sp;  // Maximum speed of the starting rolling resistance (km/h)
    double curve_resist_A;   // Paremeter of curve (R m) resist A/R (N/t)
    double inertia;
	double aux_power;      // power of auxiliary equipment
    bool use_motor;
public:  // Calculation method
    RollingResistance res_type;   // Formula of rolling resistance calculation
    bool b_fix_speed;      // Fixed speed operation if true
	bool b_reaccel;        // Apply reaccel_speed if true
	double reaccel_speed;  // Power on if the speed is lower than this (m/s)
	double spmargin;       // Safety margin to the speed limitation (km/h)


public:
    TrainBase();
    bool read_json(const nlohmann::json& jdata);
private:
    bool set_rolling_resistance(const std::string& model_name, const std::vector<double>& data);
};

#endif
