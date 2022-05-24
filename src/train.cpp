////////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#define _USE_MATH_DEFINES
#include <cmath>
#include <cassert>
#include <fstream>
#include <sstream>
#include <algorithm> // std::min
#include "common.h"
#include "train.h"
#include "RailLine.h"
////////////////////////////////////////////////////////////////////////////////
double Train::dt = 1.0/16.0;
const int RunCode::Error = -100;
const int RunCode::LessPower   = -1;
const int RunCode::InSegment   = 0;
const int RunCode::NextSegment = 1;
const int RunCode::NextStation = 2;
const int RunCode::EndOfLine   = 3;
const int RunCode::EndOfPoint  = 4;
const int RunCode::Stop        = 5;
////////////////////////////////////////////////////////////////////////////////
SpeedTraction::SpeedTraction() {
    T1 = T2 = T3 = 0;
    V1 = V2 = V3 = 0;
    nUnit = 1; unit_conv_factor = 1;
}
////////////////////////////////////////////////////////////////////////////////
void SpeedTraction::set_data(LookupItem pair[], int n_array) {
    init(n_array);
    for (size_t i = 0; i < size(); i++) {
        set(i, pair[i].index, pair[i].value);
    }
}
////////////////////////////////////////////////////////////////////////////////
void SpeedTraction::set_param(double t1, double t2, double t3, double v1, double v2, double v3) {
    T1 = t1; T2 = t2; T3 = t3; V1 = v1; V2 = v2; V3 = v3;
};
////////////////////////////////////////////////////////////////////////////////
LookupItem SpeedTraction::get_data(size_t i) {
    LookupItem d;
    d.index = 0;
    d.value = 0;
    if (i >= 0 && i < size()) {
        d.index = index[i];
        d.value = value[i];
    }
    return d;
}
////////////////////////////////////////////////////////////////////////////////
bool SpeedTraction::read_jsonfile(const nlohmann::json& jdata) {
    if( Lookup::read_jsonfile(jdata) == false) return false;
    if( unit_y == "kgf" ) unit_conv_factor = GRAV_ACC;
    else if ( unit_y == "tonf" ) unit_conv_factor = GRAV_ACC/1000;
    else unit_conv_factor = 1;  //"N"
    return true;
}
//-----------------------------------------------------------------------------
// Return the traction power (N)
// The total traction power is nUnit times the result
// unit_conv_factor: from kgf, tonf, etc. to N
//-----------------------------------------------------------------------------
double SpeedTraction::traction(double sp) {
    return nUnit * midval(sp) * unit_conv_factor;
}
//-----------------------------------------------------------------------------
// Set the maximum speed to the SegmentList considering breaking
// If the maximum speed of the next segment is lower than that of the present
// segment, this calculates the maximum allowed speed at the begining of the
// current segment.
// [Input]
//    dec deceleration (m/s^2)
//    train_length  the length of the train (m)
// [Input/Output]
//    segs
// [Return]
//   0
//-----------------------------------------------------------------------------
int setsegspeed(SegmentList& segs, double dec, double train_length, double margin)
{
    SegmentList::reverse_iterator it = segs.rbegin();
    (*it).max_speed = (*it).speed - margin;
    double sp1 = (*it).max_speed /3.6;  //segment speed is in km/h
    double d2 = 0.0;  // for station segments
    // Assume that length > train_length
    // at stations, compare two consitions:
    // 1) stop at the center of segment, and 2) entrance speed
    if ( (*it).type ==  SegmentType::Station ) {
        double d = 0.5*sp1*sp1/dec;
        d2 = (*it).length/2 + train_length/2;
        if( d > d2 ) {
            /*
            sp1 = 0.0;
            (*it).max_speed = 0.0;
            */
            sp1 = std::sqrt(2 * dec * d2);
            (*it).max_speed = sp1;
        } else d2 = 0.0;
    }
    double sp2 = sp1;
    ++it;
    while( it != segs.rend() ) {
        (*it).max_speed = (*it).speed - margin;
        sp1 = (*it).max_speed /3.6;
        double length = (*it).length + d2;
        if ( sp1 > sp2 ) {
            double d = 0.5*(sp1*sp1-sp2*sp2)/dec;
            if ( d > length ) {
                sp1 = std::sqrt(2*dec*length + sp2*sp2);
                (*it).max_speed = sp1 * 3.6;  // m/s --> km/h
            }
        }
        d2 = 0.0;
        // start speed must 0 at station segments
        if ( (*it).type == SegmentType::Station ) {
            double d = 0.5*sp1*sp1/dec;
            d2 = (*it).length/2 + train_length/2;
            if( d > d2 ) {
                /*
                sp1 = 0.0;
                (*it).max_speed = 0.0;
                */
                sp1 = std::sqrt(2 * dec * d2);
                (*it).max_speed = sp1;
            } else d2 = 0.0;
        }
        sp2 = sp1;
        ++it;
    }
    return 0;
}
//-----------------------------------------------------------------------------
// Train Constructor
//-----------------------------------------------------------------------------
Train::Train() {
    line = nullptr;
    init();
    force_method = ForceMethod::SPEED_TRACTION;
}
//-----------------------------------------------------------------------------
// Train Constructor with segment information
//-----------------------------------------------------------------------------
Train::Train(const SegmentList& segs) {
    line = nullptr;
    init(segs);
    force_method = ForceMethod::SPEED_TRACTION;
}
//-----------------------------------------------------------------------------
// Initialize the temporary variables
//-----------------------------------------------------------------------------
void Train::init() {
    total_time = 0;
    distance   = 0;
    speed      = 0;
    station_timer = 0.0;
    force = 0;
    power = 0;
}
//-----------------------------------------------------------------------------
// Train: constructor with segment information
//-----------------------------------------------------------------------------
void Train::init(const SegmentList& segs) {
    init();
    seg_it = segs.begin();
};
//-----------------------------------------------------------------------------
// Train: Set the line pointer
// Check if the train length > station segment length
//-----------------------------------------------------------------------------
bool Train::set_line(const std::shared_ptr<RailLine> r) {
    if( r ) {
        line = r;
        /*
        SegmentList::reverse_iterator it = line->segs.rbegin();
        double d = it->distance;
        ++it;
        while(it != line->segs.rend() ) {
            if( it->type == SegmentType::Station && (length > d - it->distance))
                return false;
            d = it->distance;
            ++it;
        }
        */
        seg_it = line->segs.begin();
    } else line.reset();
    return true;
}
//-----------------------------------------------------------------------------
// Train: Get the line pointer
//-----------------------------------------------------------------------------
std::shared_ptr<RailLine> Train::get_line() {
    return line;
}
//-----------------------------------------------------------------------------
// Calculate the rolling resistance of the train
// v: km/h
// Output: N (not kgf)
//-----------------------------------------------------------------------------
double Train::get_rolling_resist(double v) const {
    double r = 0;
    switch(res_type) {
        case RollingResistance::None:
            break;
        case RollingResistance::Quadratic:
            r = res_coefs[0] + res_coefs[1] * v + res_coefs[2] * v*v;
            break;
        case RollingResistance::JNR_EMU:  // kgf -> N
            r = ( (res_coefs[0]+res_coefs[1]*v)* weight + (res_coefs[2]+res_coefs[3]*(nCars-1))*v*v ) * GRAV_ACC;
            break;
        case RollingResistance::JNR_EMU_MT: // kgf -> N
            r = ((res_coefs[0]+res_coefs[1]*v)*WM + (res_coefs[2]+res_coefs[3]*v)*WT + (res_coefs[4]+res_coefs[5]*(nCars-1))*v*v) * GRAV_ACC;
            break;
        default:
            r = 0;
            break;
    }
    return r;
}
//-----------------------------------------------------------------------------
// Set the motor pointer and initialize
//-----------------------------------------------------------------------------
void Train::set_motor(std::shared_ptr<Motor> pt) {
    motor = pt;
    double F = calc_need_force(20.0, 0.84);
    motor->init(F/n_traction_units);
}
//-----------------------------------------------------------------------------
// Resistance of train
// [Input]
//   v   speed (km/h)
//   gradient % (not 1/1000)
//   radius (m)
// [Return]
//  Resistance (Unit = N, NOT N/t)
//-----------------------------------------------------------------------------
double Train::get_regist(double v, double gradient, double radius) const {
    double Rf, Rg, Rc;
    // Start resistance
    if ( v <  start_resist_sp ) {
        double res_start = start_resist * weight;  // (N/t) x t
        double res_end = get_rolling_resist(start_resist_sp);
        if (res_start > res_end)
            Rf = res_start - ((res_start - res_end) / start_resist_sp) * v;
        else Rf = get_rolling_resist(v);
    }
    else {
        Rf = get_rolling_resist(v);  // (N)
    }
    // Gradient resistance (gradient in percent)
    double gr0 = gradient/100;
    Rg = gr0/std::sqrt(1+gr0*gr0) * (weight*1000) * GRAV_ACC;   // Mg sin(a)
    // Curvature resistance
    if ( radius > 0 ) {
        Rc = (curve_resist_A/radius) * weight;  // (N/ton) * ton
    } else Rc = 0;

    return Rf + Rg + Rc;
}
//-----------------------------------------------------------------------------
// Return traction power (N)
// [Input]
//   v   speed (km/h)
//-----------------------------------------------------------------------------
double Train::get_force(double v) const {
    double F = 0.0;
    if (force_method == ForceMethod::MOTOR) F = motor->tract(v) * n_traction_units;
    else if (force_method == ForceMethod::SIMPLE) {
        if (v <= torque_max_speed) F = fixed_force;
        else if (power_max_speed > torque_max_speed && v > power_max_speed) {
            F = simple_const / std::pow(v / 3.6, 2);
        } else F = fixed_power / (v / 3.6);
    }
    else {
        assert(speed_traction);
        F = speed_traction->traction(v);
    }
    return F;
}
//-----------------------------------------------------------------------------
// Acceleration (right of a=f/M)
// (m/s) is used in the program but (km/h) is used in formulas.
// [Input]
//   sp   speed (m/s)
//   gradient  %
//   radius     (rad)
//   no_force
// [Return]
// return value is in m/s^2
//-----------------------------------------------------------------------------
double Train::df(double sp, double gradient, double radius, bool no_force = false) const {
    double v = sp * 3.6;  // input (m/s) -> km/h for well-known formulaes
    double f =  no_force ? 0.0: get_force(v);
    f = f - get_regist(v,gradient,radius);
    double M = weight * 1000 * ( 1 + inertia );  // ton -> kg
    return f/M;
}
//-----------------------------------------------------------------------------
// Return necessary force to achive the acceleration (m/s^2)
// speed (km/h)
//-----------------------------------------------------------------------------
double Train::calc_need_force(double speed, double acceleration) const {
    double inercia = 0.1;
    double M = weight * 1000 * (1 + inercia);
    double r = get_regist(speed, 0, 0);
    return (M * acceleration + r);
}
//-----------------------------------------------------------------------------
// Get the time and distance necessary to increase the speed from v0 to v1
// in case of acceleration (km/h)
//
// [parameters]
// x0: Starting position
// v0: Starting speed
// v1: Target speed
//
// [Return value]
//  1: Reach the maximum speed
//  0: Success
// -1: x0 exceed the length of the segment
// -2: end of the segment before the speed reach to v1 (keep the time and distance)
// -3: No. of loop exceeds the limit
//-----------------------------------------------------------------------------
int Train::solve(const SegmentList& segs, double x0, double v0, double v1, VarSet* var) const {
    int ret = 0;
    SegmentList::const_iterator it = segs.cbegin();
    double total_length = 0;
    while( it != segs.cend() ) {
        total_length = (*it).distance + (*it).length;
        if( (*it).distance <= x0 &&  x0 < total_length ) break;
        ++it;
    }
    if (it == segs.cend() ) return (-1);
    double gradient = (*it).gradient;
    double radius = (*it).radius;
    double maxSpeed = (*it).max_speed/3.6;
    //assert( v0 < *v);
    double L = x0;
    double sp = v0/3.6;
    double acc;
    int counter = 0;
    while( sp < v1/3.6 ) {
        if( sp > maxSpeed ) {
            ret = 1;
            break;
        }
        step(gradient, radius, &sp, &L, &acc);
        // get gradient and curvature if segment changed
        if (L > total_length ) {
            ++it;
            total_length = (*it).distance + (*it).length;
            while( it != segs.cend() && L >  total_length ) {
                ++it;
                total_length = (*it).distance + (*it).length;
            }
            if( it != segs.cend() ) {
                gradient = (*it).gradient;
                radius = (*it).radius;
                maxSpeed = (*it).max_speed/3.6;
            } else {
                ret = -2;
                L = total_length;
                break;
            }
        }
        counter++;
        if (counter > 2000) {
            fprintf(stderr,"Calculation Error : %f %f", sp * 3.6, L);
            L = total_length;
            ret = -3;
            break;
        }
        printf("D=%f V=%f A=%f\n", L, sp, acc);
    }
    var->sp = sp;
    var->tm = dt * counter;
    var->x = L;
    return (ret);
}
//-----------------------------------------------------------------------------
// Step in solving f=ma by 4th order Runge-Kutta
// [Input] gradient and radius
// [Input] no_force: engine off is no_force is false
// [Ref]
// v: speed (m/s)
// x: location (m)
// a: acceleration (m/s^2)
//-----------------------------------------------------------------------------
void Train::step(double gradient, double radius,
        double* v, double* x, double* a, bool no_force) const
{
    double v1 = *v;
    double x1 = *x;
    // df (v1 is in m/s)
    double h1 = df(v1,gradient,radius,no_force) ;
    double k1 = v1 ;
//printf("[v= %f h=%f k=%f]", v1, h1, k1);
    double h2 = df(v1+h1*dt/2,gradient,radius,no_force) ;
    double k2 = (v1+h1*dt/2) ;

    double h3 = df(v1+h2*dt/2,gradient,radius, no_force) ;
    double k3 = (v1+h2*dt/2) ;

    double h4 = df(v1+h3*dt,gradient,radius, no_force) ;
    double k4 = (v1+h3*dt) ;

    *a = h1/6+h2/3+h3/3+h4/6;
    *v = v1 + (h1/6+h2/3+h3/3+h4/6) * dt;
    *x = x1 + (k1/6+k2/3+k3/3+k4/6) * dt;

}
//-----------------------------------------------------------------------------
 // Current version: the center of the train is at the center of the start
 // segment in the beginning.
 // Initilaiez the internal variables before main_run
 // status is acceleration
 //-----------------------------------------------------------------------------
int Train::prepare_run() {
    seg_it = line->segs.begin();
    distance = seg_it->length/2 + length/2;
    while (seg_it != line->segs.end() && distance > seg_it->length) ++seg_it;
    if (seg_it == line->segs.end()) return (-1);
    total_time = 0.0;
    acc_tm = 0.0;
    speed = 0.0;
    accel = 0.0;
    departure = 0.0;
    force = 0.0;
    tm1 = 0.0;
    total_power = 0.0;
    station_timer = 0.0;
    entered = false;
    setsegspeed(line->segs, dec, length, spmargin);
    status = TrainStatus::Traction;
/*
for(const auto& item: line->segs) {
    printf("SP=%g MAX=%g\n", item.speed, item.max_speed);
}
*/
    return(0);
}
//-----------------------------------------------------------------------------
// Get the maximum speed bitween the head and the tail of this train
// x1: location of the tail
// x2: location fo the head
// return is in m/s
// Note: max_speed is the max speed at the beginning of the segment.
// If the segment is a station segment, the max speed of arriving is max_speed
// but that of departing is not the same
//-----------------------------------------------------------------------------
double Train::get_min_speed(double x1, double x2) const {

    if (!line || line->nSegment() == 0) return max_speed;

    double result = max_speed;
    SegmentList& segs = line->segs;
    SegmentList::const_iterator pre = segs.begin();
    if ((pre->head_only == false) ||
        (x1 >= pre->distance && x1 < pre->distance + pre->length) ||
        (x2 > pre->distance && x2 <= pre->distance + pre->length)) {
        if (pre->type == SegmentType::Station) result = pre->speed; // station departure max
        else result = pre->max_speed;
    }
    for (auto it = std::next(pre); it != segs.end(); ++it) {
        if (x2 < it->distance) {
            if (pre->head_only == false)
            {
                if (pre->type == SegmentType::Station) result = std::min(pre->speed, result);
                else result = std::min(pre->max_speed, result);
                break;
            }
        }
        if (x1 < it->distance) {
            if (pre->head_only == false) {
                if (pre->type == SegmentType::Station) result = std::min(pre->speed, result);
                else result = std::min(pre->max_speed, result);
            }
        }
        pre = it;
    }
    return result / 3.6;
}
//-----------------------------------------------------------------------------
// Update the following data from f(t) to f(t + dt)
// - speed, distance, accel
// - acc_tm
// - force
// - total_time, total_power
// - status
// - entered
// Use condition variables insted of using the segment information
// status 0=traction, 1=coaching, 2=breaking, 3=stop
// [Return]
//  RunCode
//-----------------------------------------------------------------------------
int Train::update() {
    int ret = RunCode::InSegment;
    double start_dist = (*seg_it).distance;
    double gradient = (*seg_it).gradient;
    double radius = (*seg_it).radius;
    // It is necessary to consider the speed lmitation
    double limspeed = std::min((*seg_it).max_speed, max_speed) / 3.6;  //m/s
    SegmentList::const_iterator next_it = std::next(seg_it);
    bool bLastSeg = (next_it == line->segs.end()) ? true : false;
    double next_dist = start_dist + (*seg_it).length;  // distance of the next segment
                                                       // the same as: next_dist = (*next_it).distance;
    // next_max_speed (m/s)
    double next_max_speed = bLastSeg ? 0 : std::min((*next_it).max_speed, max_speed) / 3.6 ;
    if (next_max_speed < 0) next_max_speed = 0;
    // If the train is in the station segment before the midpoint of the segment,
    // meaning that the train has not arrived at the station, the next point should be the midpoint
    if (seg_it->type == SegmentType::Station && distance < start_dist + seg_it->length / 2 + length / 2) {
        next_dist = start_dist + seg_it->length / 2 + length / 2;
        next_max_speed = 0;
    }
    else {
        // The speed of the midpoint of the setion segment is zero.
        if (!bLastSeg && next_it->type == SegmentType::Station && next_it->max_speed == 0)
            next_dist += next_it->length * 0.5;
    }
    // Considering the speed limit between the front and tail of the train
    // entered=false if the speed of the previous section is the matter than the head of the train
    if (entered == false) {
        // Calculate the minmum speed limit between the head and tail of the train
        double x = get_min_speed(distance - length, distance);
        if (x <= 0) return RunCode::Error;
        if (distance - length < start_dist) {
            // if the speed limit is larger than that of the head, it is entered.
            if (x >= limspeed) {
                entered = true;
            }
        }
        else entered = true;
        // Change coasting to acceleration if the speed limit become large when the train
        // entered a new segment
        if (entered && (status == TrainStatus::Coasting) && (limspeed < x))
            status = TrainStatus::Traction;
        if (entered == false) limspeed = x;
    }
    // Check if it is necessary to continue breaking
    /*
    if (status == TrainStatus::Breaking) {

        if (distance - 0.5 * (next_max_speed * next_max_speed - speed * speed) / dec < next_dist ) {
            if (b_reaccel && (speed <= limspeed - reaccel_speed)) {
                status = TrainStatus::Traction;
            }
            else status = TrainStatus::Coasting;
        }
    }
    */
    // In traction or coasting
	if( status == TrainStatus::Traction || status == TrainStatus::Coasting || status == TrainStatus::Constant) {
		// Consider braking if the present speed is lager than the next speed limit
        // units: m, s
        double x = distance;
        double v = speed;
        double a;
        // For the safety side, forecast the situation of t+dt
        // train length is ignored
        if(status == TrainStatus::Traction) {
            step(gradient, radius, &v, &x, &a, false);
            if (a <= 0) {
                fprintf(stderr, "X=%g V=%g a=%g g=%g r=%g\n", x, v, a, gradient, radius);
                return RunCode::LessPower;
            }
            if (v > limspeed) status = TrainStatus::Coasting;
        }
        if(status == TrainStatus::Coasting) {
            step(gradient,radius, &v,&x,&a, true);
            if (x >= next_dist ) {
                if(v > next_max_speed) status = TrainStatus::Breaking;
            } else if (v > std::sqrt(next_max_speed * next_max_speed + 2 * dec * (next_dist - x))) {
                status = TrainStatus::Breaking;
            }
            // if gradient is negative, speed can increase.
        }
        else {
            x = distance + speed * dt;
        }
        // x + 0.5*(v*v-next_max_speed*next_max_speed)/dec
        // if the present speed is 0 and it will exceed the distance after the acceleration of dt,
        // it is assumed that the train has arrived.
		if((speed > 0 ) && (v > next_max_speed)
                && (x + 0.5*(v*v-next_max_speed*next_max_speed)/dec >= next_dist)) {
            // Check if the distance exceeded
            status = TrainStatus::Breaking;
		} else if (status == TrainStatus::Traction) { // in case of traction

            // apply speed before update
            force = get_force(speed*3.6);  // N
            power = force * speed / 1000; //  N x m/s = J/s -> kW
            //step(gradient, radius, &speed, &distance, &accel);
            speed = v;
            distance = x;
            accel = a;
			// Possibility to run for a while after the deceleration during traction?
            // Case when the speed becomes 0 during traction (such as steep slope)
            if( speed < 0 ) return RunCode::LessPower;
            // Change constant speed or coasting if the speed exceeds
            if( speed > limspeed ) {
                if ( b_fix_speed ) status = TrainStatus::Constant;
                else status = TrainStatus::Coasting;
            }
		} else if (status == TrainStatus::Coasting ) {
			// In case of re-traction
			if( b_reaccel && (speed <= limspeed - reaccel_speed)) {
                force = get_force(speed*3.6);
                power = force * speed / 1000; // J/s -> kW
                step(gradient, radius, &speed, &distance, &accel);
				status = TrainStatus::Traction;
			} else if( b_fix_speed ) {
                // acceleration is 0 when the train speed is constant
                accel = 0.0;
                force = get_regist(speed,gradient,radius);
                power = force * speed /1000; // J/s -> kW
                distance += speed * dt;
            } else {
                distance = x;
                accel = a;
                if ( v <= 0 ) { // Use the same speed when the train stop during coasting. Next, accelerate
                    accel = (0.0 - speed)/dt;
                    distance += (-0.5) * speed * speed / accel;
                    speed = 0.0;
                    // Ignore the adjustment of traction power due to the adjustment above.
                    force = get_regist(speed,gradient,radius);
                    power = force * speed /1000; // J/s -> kW
                    status = TrainStatus::Traction;
                } else if (v > speed) { // If speed increases due to the slope
                    force = get_regist(speed,gradient,radius); // This must be negative
                    power = force * speed /1000; // J/s -> kW
                    accel = 0;
                } else {
                    // Coasting
                    force = 0.0;
                    power = 0.0;
                    step(gradient, radius, &speed, &distance, &accel, true);
                }
            }
		} else if (status == TrainStatus::Constant ) {
            distance = x;
        }
	}
    // not else, because it is necessary to consider breaking during traction and coasting calculation
    if (status == TrainStatus::Breaking ) { 	// break
        force = 0.0;
        if ( speed <= 0.0 ) {
            status = TrainStatus::Traction;
        }
        else if (next_max_speed > speed) {
            // Breaking because of slope
            assert(next_dist > distance);
            accel = 0.5 * (next_max_speed * next_max_speed - speed * speed) / (next_dist - distance);
            assert(accel > 0);
            force = calc_need_force(speed, accel);
            power = force * speed / 1000; // J/s -> kW
            double d_tm = (next_max_speed - speed) / accel;
            if (d_tm < dt) {
                accel = (next_max_speed - speed) / dt;
            }
            double v = speed + accel * dt;
            double d = speed * dt + 0.5 * accel * dt * dt;
            speed = v;
            distance = d;
        } 
        else {
            // Assume the deceleration is constant in case of breaking
            // accel = dec * (-1);
            assert(next_dist > distance);
            accel = 0.5 * (next_max_speed * next_max_speed - speed * speed) / (next_dist - distance);
            assert(accel <= 0);
            force = calc_need_force(speed, accel);
            power = force * speed / 1000; // J/s -> kW
            double v = speed + accel * dt;
            if ( v < 0 ) {
                v = 0.0;
                accel = speed / dt * (-1);
            }
            // double d = 0.5*(v*v-speed*speed)/accel + distance;
            double d = speed * dt + 0.5 * accel * dt * dt;
            if ( d < 0 ) {
                // Adjustment to make sure the speed is 0 after dt
                accel = speed / dt * (-1);
                d = speed * dt + 0.5 * accel * dt * dt + distance;
            } else {
                d += distance;
            }
            distance = d;
            speed = v;
        }
	}
	// Recored the traction time
	if (status==TrainStatus::Traction) acc_tm += dt ;
	total_time = total_time + dt;
    total_power += power * dt;

	// Entering to the next segment is decided by the distance
	if( distance >= next_dist) {
        // if the next segment is a station and the speed limit of the segment is not considered
        if( !bLastSeg && next_it->type == SegmentType::Station && next_it->max_speed == 0) {
            status = TrainStatus::Stop;
            ret = RunCode::NextStation;
        } else if( seg_it->type == SegmentType::Station && next_max_speed == 0) {
            status = TrainStatus::Stop;
            ret = RunCode::NextStation;
        } else {
            ret =  RunCode::NextSegment;
            if (status == TrainStatus::Breaking) status = TrainStatus::Coasting;
        }
		if( seg_it->head_only) entered = true;
        else entered = false; // need to consider the length of the train to determine the max speed
	}
	return ret;
}
//-----------------------------------------------------------------------------
// main body of the simulation
// [Update]
// - tm1
// - station_timer
// - seg_it
// The segment of a switch is the same as other segemnt
// call update
// [Return values]
// 0: stay in the segment
// 1: arrived at the next segment
// 2: arrived at the station
// 3: end of the line
// 4: pass the switch
// 5: stopping
 //-----------------------------------------------------------------------------
int Train::main_run() {
	int result;
    SegmentList::const_iterator next_it;

	if( station_timer > 0 ) {
		station_timer -= dt;
		if( station_timer <= 0 ) {
			station_timer = 0;
			departure = total_time;
			tm1 = 0;
			status = TrainStatus::Traction;
		} else {
			total_time += dt;
			tm1 += dt;
			status = TrainStatus::Stop ;
            accel = 0.0;
            speed = 0.0;
            force = 0.0;
			result = 5 ;
			return (result) ;
		}
	}
	// Run the train
	result = update();
    if (result == RunCode::Error) {
        printf("Error\n");
        exit(1);
    } else if( result == RunCode::NextSegment ) {// if the train arrived the next segment
        // The length of Point is 0. The head of train can be in the next next segment. 
        while (seg_it != line->segs.end() && distance > seg_it->distance+seg_it->length) ++seg_it;
        if(  seg_it == line->segs.end() ) return RunCode::EndOfLine;
	} else if (result == RunCode::NextStation) {
        if (seg_it == line->segs.end() ) return RunCode::EndOfLine;
        // for debug(1)
        next_it = std::next(seg_it);
        if( next_it == line->segs.end() ) return RunCode::EndOfLine; // The last station
        else if( next_it->type == SegmentType::Station) {
            ++seg_it;
            next_it = std::next(seg_it);
            if( next_it == line->segs.end() ) return RunCode::EndOfLine;
        }
        else if( seg_it->type != SegmentType::Station) {
            printf("[BUG] Train stops in the non-station segment.\n");
            exit(1);
        }
        // for debug(2)
        if( status != TrainStatus::Stop) {
            fprintf(stderr, "[BUG] Train status is not stop when it stops.\n");
            exit(1);
        }
        // Start countin of the stoppint time at the station
        if( seg_it->tm_stop > 0 ) station_timer = seg_it->tm_stop;
        else station_timer = Control::station_time;
        // departure time is second digit
        if( total_time - floor(total_time) > 0) {
            station_timer += (1 - (total_time - floor(total_time)));
        }
    }
	tm1 += dt;
	return result;
}
//-----------------------------------------------------------------------------
// print out the output
//-----------------------------------------------------------------------------
void Train::run_print(FILE* fp) {
/*
    int a = 0;
    if(entered) a = 1;
    printf("%d\t%.0f\t%.2f\t%.1f\t%.3f\t%.0f\t%d\n",static_cast<int>(status),
        total_time, distance, speed*3.6, accel,force,a);
*/
    fprintf(fp, "%d\t%.3f\t%.2f\t%.1f\t%.3f\t%.0f\t%.1f\n",static_cast<int>(status),
        total_time, distance, speed*3.6, accel,force,power);
}
