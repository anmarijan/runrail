#include <string>
#include "nlohmann/json.hpp"
#include "common.h"
#include "TrainBase.h"

//-----------------------------------------------------------------------------
// TrainBase class
//-----------------------------------------------------------------------------
TrainBase::TrainBase() {
    id = 0;
    name = "209";
    max_speed = 70;
    fixed_acc   = ACCELERATION; // m/s/s
    dec   = DECELERATION;
    coast = COASTING;
    jerk = DEFJERK;
    length = 200.0;
    WM = 205.0;
    WT = 261.0;
    weight = WM + WT ;
    nCars = 10;
    inertia = 0.1;
    res_type = RollingResistance::JNR_EMU;
    res_coefs[0] = 1.6;
    res_coefs[1] = 0.035;
    res_coefs[2] = 0.0197;
    res_coefs[3] = 0.0078;
    start_resist = 3.0 * GRAV_ACC;  // 3kgf/t
    start_resist_sp = 3.0;     // until 3.0 km/h
    curve_resist_A = 600 * GRAV_ACC;
    //
    spmargin = 1.0;
    reaccel_speed = 4.0/3.6;
    b_fix_speed = false;
    b_reaccel = true;
    //
    n_traction_units = 1;
    speed_traction_index = 0;
    line_index = 1;
    //
    force_method = ForceMethod::SPEED_TRACTION;
    motor_index = 0;
    // SIMPLE METHOD
    torque_max_speed = 36;
    power_max_speed = 0;     // Ignore if power_max_speed = 0
    set_simple_method();
}

bool TrainBase::read_json(const nlohmann::json& jdata) {
	if( jdata.is_array() ) {
		fprintf(stderr,"Format error of Train (array)\n");
		return false;
	}
	try {
        double temp = 0;
		id = jdata.at("id");
        name = jdata.at("name");
		max_speed = jdata.at("maxspeed");
		length = jdata.at("length");
		WM = jdata.at("WM");
		WT = jdata.at("WT");
        nCars = jdata.at("nCars");
        if( jdata.contains("traction") == true ) {
            std::string traction_type = jdata.at("traction");
            if (traction_type == "motor") force_method = ForceMethod::MOTOR;
            else if (traction_type == "simple") force_method = ForceMethod::SIMPLE;
            else force_method = ForceMethod::SPEED_TRACTION;
        }
        if (jdata.contains("torquemaxspeed") == true) temp = jdata.at("torquemaxspeed");
        if (temp > 0) {
            torque_max_speed = temp;
        }
        temp = 0;
        if (jdata.contains("powermaxspeed") == true) temp = jdata.at("powermaxspeed");
        if (temp > 0) {
            power_max_speed = temp;
        }
        if( jdata.contains("motorindex") == true ) motor_index = jdata.at("motorindex");
        if( jdata.contains("sptrindex") == true ) speed_traction_index = jdata.at("sptrindex");
        if( jdata.contains("acceleration") == true) fixed_acc = jdata.at("acceleration");
        if( jdata.contains("deceleration") == true) dec = jdata.at("deceleration");
        if( jdata.contains("coasting") == true) coast = jdata.at("coasting");
        if( jdata.contains("jerk") == true) jerk = jdata.at("jerk");
        if( jdata.contains("nTractions") == true) n_traction_units = jdata.at("nTractions");
        if( jdata.contains("resistance") == true) {
            std::string model_name = jdata["resistance"]["model"];
            std::vector<double> model_data = jdata["resistance"]["params"];
            if( set_rolling_resistance(model_name, model_data) == false) {
                fprintf(stderr,"No. of parameters of the rolling resistance model is not enough.\n");
                return false;
            };
        }
        if( jdata.contains("lineindex") == true) line_index = jdata.at("lineindex");
	} catch(nlohmann::json::exception& e) {
		fprintf(stderr,"%s\n", e.what());
		return false;
	}
    set_simple_method();
	return true;
}

bool TrainBase::set_rolling_resistance(const std::string& model_name, const std::vector<double>& data) {
    if( model_name == "None") res_type = RollingResistance::None;
    else if( model_name == "Quadratic") {
        res_type = RollingResistance::Quadratic;
        if( data.size() < 3) return false;
        for(int i=0; i < 3; i++) res_coefs[i] = data[i];
    }
    else if( model_name == "JNR_EMU") {
        res_type = RollingResistance::JNR_EMU;
        if( data.size() < 4) return false;
        for(int i=0; i < 4; i++) res_coefs[i] = data[i];
    }
    else if( model_name == "JNR_ENU_MT") {
        res_type = RollingResistance::JNR_EMU_MT;
        if( data.size() < 6) return false;
        for(int i=0; i < 6; i++) res_coefs[i] = data[i];
    }
    else {
        fprintf(stderr,"Formula of %s is not implemented.\n", model_name.c_str());
        return false;
    }
    return true;
}

void TrainBase::set_simple_method() {
    fixed_force = (weight * 1000) * (1 + inertia) * fixed_acc;
    fixed_power = fixed_force * (torque_max_speed / 3.6);
    simple_const = fixed_power * (power_max_speed / 3.6);
}