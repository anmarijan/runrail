#ifndef TRAIN_BASE_H
#define TRAIN_BASE_H

#include <string>
#include "nlohmann/json.hpp"
////////////////////////////////////////////////////////////////////////////////
// default values
constexpr double ACCELERATION = 0.83;
constexpr double DECELERATION = 1.0;
constexpr double DEFJERK = 0.6;
constexpr double COASTING = 0.1;
////////////////////////////////////////////////////////////////////////////////
enum class ForceMethod { MOTOR, SPEED_TRACTION, SIMPLE };
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
    double fixed_acc;        // Fixed accelerations (m/s^2)
    double dec, coast;       // decelerations (m/s^2)
    double jerk;             // jerk (m/s^3)
    double fixed_force;      // Fixed force (N) in const-torque (ForceMethod::SIMPLE)
    double torque_max_speed; // Max speed (km/h) of torque area (ForceMethod::SIMPLE)
    double power_max_speed;  // Max speed (km/h) of constant power area (ForceMethod::SIMPLE)
    double fixed_power;      // Fixed power (ForceMethod::SIMPLE)
    double simple_const;     // Constant value for special area (ForceMethod:SIMPLE)
    double res_coefs[10];    // Each train keeps coefficients of resistance model
    double start_resist;     // Starting rolling resistance (N/t)
    double start_resist_sp;  // Maximum speed of the starting rolling resistance (km/h)
    double curve_resist_A;   // Paremeter of curve (R m) resist A/R (N/t)
    double inertia;
	double aux_power;      // power of auxiliary equipment
    ForceMethod force_method;
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
    void set_simple_method();
};

#endif
