#ifndef RUNCONTROL_H
#define RUNCONTROL_H
///////////////////////////////////////////////
#include <string>
#include <memory>
#include <list>
#include "RailLine.h"
#include "train.h"
///////////////////////////////////////////////
class RunControl {
    double mSvgMaxpt;
public:
    std::string errmsg;
    std::list<std::shared_ptr<Train>> trains;
    std::list<std::shared_ptr<RailLine>> lines;
    std::list<std::shared_ptr<SpeedTraction>> sptr_list;
    std::list<std::shared_ptr<Motor>> motors;
public:
    std::shared_ptr<RailLine> present_line;
    std::shared_ptr<Train> present_train;
public:
    RunControl();
    std::shared_ptr<RailLine> getLine(int line_id);
	bool read_line(const char* fname);
    bool read_train(const char* fname);
    bool read_traction(const char* fname);
    bool read_params(const char* fname);
    void set_train_traction();
    void set_train_motor();
    bool set_train_line();
    int run1(const char* fname);
    void traction_test(const char* fname);
    void print_data();
    double svg_maxpt() { return mSvgMaxpt; };
};

#endif
