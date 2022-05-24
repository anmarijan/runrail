#ifndef TRAIN_H
#define TRAIN_H
////////////////////////////////////////////////////////////////////////////////
#include <memory>
#include <vector>
#include <list>
////////////////////////////////////////////////////////////////////////////////
#include "Lookup.h"
#include "Motor.h"
#include "RailLine.h"
#include "TrainBase.h"
////////////////////////////////////////////////////////////////////////////////
using SegmentList = std::vector<Segment>;
////////////////////////////////////////////////////////////////////////////////
enum class TrainStatus {Traction, Coasting, Breaking, Stop, Constant};
//---------------------------------------------------------------------------
struct RunCode {
    static const int Error;
	static const int LessPower;
	static const int InSegment;     // In a segment
	static const int NextSegment;   // Arrived at the next segment
	static const int NextStation;   // Arrived at the next station
	static const int EndOfLine;     // End of the line
	static const int EndOfPoint;    // Passing a swith
	static const int Stop;          // Stopping
};
//-----------------------------------------------------------------------------
// Speed Traction class
//-----------------------------------------------------------------------------
class SpeedTraction : public Lookup {
    double unit_conv_factor;
public:
	double T1, T2, T3;
	double V1, V2, V3;
	int nUnit;
public:
	SpeedTraction();
	void set_data(LookupItem pair[], int n_array);
	void set_param(double t1, double t2, double t3, double v1, double v2, double v3);

	double traction(double sp);
	LookupItem get_data(size_t i);
    bool read_jsonfile(const nlohmann::json& jdata);
};
///////////////////////////////////////////////////////////////////////
// Set the maximum speed considering the next segment
///////////////////////////////////////////////////////////////////////
int setsegspeed(SegmentList& segs, double dec, double train_length, double margin);

//-----------------------------------------------------------------------------
// A set of variables (time, speed, distance)
// Point-1: The corner point of const-torque and const-power curve
//-----------------------------------------------------------------------------
struct VarSet {
    double tm;
    double sp;
    double x;
};
////////////////////////////////////////////////////////////////////////////////
/// Train Class
//  This keeps an iterator of the belonging segment
////////////////////////////////////////////////////////////////////////////////
class Train : public TrainBase {
public:
    static double dt;       //step size of time (in second)
private:
    // Time dependent variables
    double total_power;  // Total Work (acceleration only) from the beginning of the Simulation (J)
    double total_time;   // Total time from the beginning of the Simulation (s)
    double acc_tm;       // Total time of traction (s)
    double distance;     //(m)
    double speed;        //(m/s)
    double accel;        //(m/s^2)
    double departure;    // Departure time at a station after the main_run
    double force;        // (N)
    double power;        // (1000 J/s)
    double cur_current;
    double tm1;          // Total time between stations
    double station_timer;  // remaining time
	bool entered;
    TrainStatus status;
    SegmentList::const_iterator seg_it; // Current segment in which this train exist
private:
    // Pointer to global objects
    std::shared_ptr<SpeedTraction> speed_traction;  // Speed-Traction relationship
    std::shared_ptr<RailLine> line;
    std::shared_ptr<Motor> motor;
public:
    Train();
    Train(const SegmentList& segs);
    void init();
    void init(const SegmentList& segs);
    double get_speed() const { return speed; };
    double get_dist() const { return distance; };
    TrainStatus get_status() const { return status;};
    // Functions for internal variables
    void set_speed_traction(std::shared_ptr<SpeedTraction> pt) {speed_traction = pt;};
    void set_motor(std::shared_ptr<Motor> pt);
    void set_status(TrainStatus new_status) { status = new_status;};
    void set_dt(double d) { dt = d;};
    bool set_line(const std::shared_ptr<RailLine> r);
    std::shared_ptr<RailLine> get_line();
protected:
    double get_rolling_resist(double v) const;
public:
    double get_regist(double v, double gradient, double radius) const;
    double get_force(double v) const;
    double df(double sp, double gradient, double radius, bool no_force) const;
    double get_power() const { return power;};
public:
    int solve(const SegmentList& segs, double x0, double v0, double v_max, VarSet* var) const;
public:
    int prepare_run();
    int main_run();
    void run_print(FILE* fp);
private:
    void step(double gradient, double radius, double* v, double* x, double* a, bool no_force=false) const;
    int update();
    double get_min_speed(double x1, double x2) const;
    double calc_need_force(double speed, double acceleration) const;
};


#endif
