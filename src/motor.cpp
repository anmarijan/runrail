#include "common.h"
#include "motor.h"
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------
// 1 Motor
//-----------------------------------------------------------------------------
Motor::Motor() {
	max_power = 182;       // Max power (kW)
    Volt = 1100;           // Voltage (1100V)
	n_pole = 4;            // Numner of Poles
	power_coef1 = 0.95;     //Power efficiency rate
    power_coef2 = 0.90;
    fb1 = 50;              // Input frequency at Point-1 (maximum)
    fb2 = 110;             // Input frequency at Point-2
    fs1 = 2.0;             // Slip frequency at Point-1
    fs2 = 6.4;             // Slip frequency at Point-2
// train
	full_speed = 150;      // maximum speed
	diameter = 820;        //Diameter of wheels (mm)
	gear = 7.07;           //Reduction ratio
}
//-----------------------------------------------------------------------------
// Torque between Point-1 and Point-2
//-----------------------------------------------------------------------------
double Motor::torque(double fi, double fs) {
    double c = coef_t * M_1_PI * 0.5;
    double s = fs/fi;
    return c * pow(Volt/fi,2) * (fs/(coef_a+fs*fs)) * (0.5*n_pole/(1-s));
}
//-----------------------------------------------------------------------------
// Motor: F (N) : Calculate fi1 that achives the traction power = F.
// f (Hz) = sp2f x speed (km/h)
// max_power (kW)
// diameter (mm)
// coef_a = pow(a), a = r/(2*pi*L)
// I = s*E/{(2*pi*L)*sqrt(pow(a)+pow(s*f))}
// W = I*I*r/s = coef_t * V^2 * (s/(a^2+fs^2))
//-----------------------------------------------------------------------------
void Motor::init(double F) {
    double WD = diameter /1000;
    double s1 = fs1/fb1;
    // I(fb1,fs1)=I(fb2,fs2)
    coef_a = (fs2*fs2-fs1*fs1)/((fs2/fb2)/s1) - fs1*fs1;
    coef_t = ((coef_a+fs1*fs1)/(fs1/fb1)) * (max_power * power_coef1 * 1000) / (Volt*Volt);
    double T0 = torque(fb1, fs1);
    F0 = T0 * gear / (WD/2);
    if( F0 > F ) {
        // Slip rate is fixed
        fi1 = (F0/F) * fb1;
    } else {
        fi1 = fb1;
    }
    // Slip rate is assumed to be the same
    double fs0 = s1 * fi1;
    double c = (M_PI * WD )/gear;
    velo1 = 3.6 * c * (fi1-fs1) / (n_pole/2); // km/h
    if( fs0 >= fs2 ) velo2 = velo1;
    else {
        velo2 = 3.6 * c * (fb2-fs2) /(n_pole/2);
    }
    sp2f = (1000/3.6) * n_pole * gear * M_1_PI / diameter;
    Force = F;
}
//-----------------------------------------------------------------------------
// Motor: Traction power
// Need Slip frequency at Point-2
//-----------------------------------------------------------------------------
double Motor::tract(double sp) const {
    double x;
    if( sp < velo1) {
        x = F0;
    } else if ( sp < velo2 && velo1 < velo2) {
        double c = power_coef1 + ((power_coef2-power_coef1)/(velo2-velo1))*(sp-velo1);
        x = F0 * (velo1/sp) * (power_coef1/c);
    } else {
        double F2 = F0 * (velo1/velo2) * (power_coef1/power_coef2);
        double f2 = sp2f * velo2 + fs2;
        double f  = sp2f * sp + fs2;
        // power efficiency for sp >= velo2 is assumed to be constant
        x = F2 * pow(f2/f,2);
    }
    return x;
}
//-----------------------------------------------------------------------------
// Motor:
//-----------------------------------------------------------------------------
double Motor::power_ton(double sp) const {
    double F = tract(sp);
    double e;
    if( sp < velo1) e = power_coef1;
    else if( sp < velo2) e =  power_coef1 + ((power_coef2-power_coef1)/(velo2-velo1))*(sp-velo1);
    else e = power_coef2;
    return F * GRAV_ACC * sp /( e * 3600);
}
//-----------------------------------------------------------------------------
// Motor:
//-----------------------------------------------------------------------------
void Motor::print() {
    printf("Traction force (N) = %f\n", F0);
    printf("Frequency-1 = %f\n", fb1);
    printf("Frequency-2 = %f\n", fb2);
    printf("Slip Frequency-1 = %f\n", fs1);
    printf("Slip Frequency-2 = %f\n", fs2);
    printf("Requested traction force (N) = %f\n", Force);
    printf("Frequency-1 (adjusted) = %f\n", fi1);
    printf("Speed-1 = %f\n", velo1);
    printf("Speed-2 = %f\n", velo2);
    printf("coef_a = %f\n", coef_a);
    printf("coef_t = %f\n", coef_t);
	printf("speed to frequency = %f\n", sp2f);
}
//-----------------------------------------------------------------------------
// Motor
//-----------------------------------------------------------------------------
bool Motor::read_json(const nlohmann::json& jdata) {
	if( jdata.is_array() ) {
		fprintf(stderr,"Format error (array)\n");
		return false;
	}
	try {
        id = jdata.at("id");
		max_power = jdata.at("power");
		Volt = jdata.at("volt");
		n_pole = jdata.at("pole");
		power_coef1 = jdata.at("efficiency1");
		power_coef2 = jdata.at("efficiency2");
		fb1 = jdata.at("freq1");
        fb2 = jdata.at("freq2");
        fs1 = jdata.at("fslip1");
        fs2 = jdata.at("fslip2");
        full_speed = jdata.at("maxspeed");
        diameter = jdata.at("diameter");
        gear = jdata.at("gear");
	} catch(nlohmann::json::exception& e) {
		fprintf(stderr,"%s\n", e.what());
		return false;
	}
	return true;

}
