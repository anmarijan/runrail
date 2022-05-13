#include <iostream>
#include <fstream>
#include <stdexcept>
#include "RunControl.h"
#include "train.h"
#include "nlohmann/json.hpp"
///////////////////////////////////////////////////////////////////////////////
using namespace nlohmann;
///////////////////////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
RunControl::RunControl() {
    mSvgMaxpt = 1;
}
//-----------------------------------------------------------------------------
// Get the line pointer of id = line_id
//-----------------------------------------------------------------------------
std::shared_ptr<RailLine> RunControl::getLine(int line_id) {
    for(const auto& item: lines) {
        if( item->getID() == line_id ) {
            return item;
        }
    }
    return nullptr;
}
//-----------------------------------------------------------------------------
// Read a train data file
//-----------------------------------------------------------------------------
bool RunControl::read_train(const char* fname) {
    std::ifstream fs(fname);
 	if (!fs) {
		fprintf(stderr,"Cannot open file %s\n", fname);
		return false;
	}
	json jroot;
    try {
        fs >> jroot;
        fs.close();
    } catch(...) {
        fprintf(stderr,"Format Error in %s\n", fname);
		return false;
    }
    bool ret = true;
    try {
        json jdata = jroot["train"];
        for(json::iterator it = jdata.begin(); it != jdata.end(); ++it) {
            std::shared_ptr<Train> train = std::make_shared<Train>();
            ret = train->read_json(*it);
            if( ret == false) break;
            trains.push_back(std::move(train));
        }
    } catch(nlohmann::json::exception& e) {
        fprintf(stderr,"Error: %s\n", e.what() );
        return false;
    }
    return true;
}
//-----------------------------------------------------------------------------
// Read a line data file
//-----------------------------------------------------------------------------
bool RunControl::read_line(const char* fname) {
    std::shared_ptr<RailLine> line = std::make_shared<RailLine>();
    int ret = line->read(fname);
    if ( ret <= 0 ) {
        fprintf(stderr,"ERROR: Reading line file (%d)\n", ret);
        return false;
    }
    lines.push_back(line);
    return true;
}
//-----------------------------------------------------------------------------
// Read a speed-traction file
//-----------------------------------------------------------------------------
bool RunControl::read_traction(const char* fname) {
    std::ifstream fs(fname);
	if (!fs) {
		fprintf(stderr,"Cannot open file %s\n", fname);
		return false;
	}
	json jroot;
	try {
		fs >> jroot;
        fs.close();
	}
	catch (...) {
        fprintf(stderr,"Format Error in %s\n", fname);
		return false;
	}
    bool ret = true;
    try {
        json jdata = jroot["speedtraction"];
        for(json::iterator it = jdata.begin(); it != jdata.end(); ++it) {
            std::shared_ptr<SpeedTraction> sptr = std::make_shared<SpeedTraction>();
            ret = sptr->read_jsonfile(*it);
            if( ret == false) break;
            sptr_list.push_back(std::move(sptr));
        }
    } catch(nlohmann::json::exception& e) {
        fprintf(stderr,"Error: %s\n", e.what() );
        return false;
    }
    if ( ret ) {
        errmsg = "";
        // sptr->print();
    }
    else errmsg = "ERROR: Reading Speed-Traction file (" + std::string(fname) + ").\n";
    return ret;
}
//-----------------------------------------------------------------------------
// Read params
//-----------------------------------------------------------------------------
bool RunControl::read_params(const char* fname) {
    std::ifstream fs(fname);
	if (!fs) {
		fprintf(stderr,"Cannot open file %s\n", fname);
		return false;
	}
	json jroot;
	try {
		fs >> jroot;
        fs.close();
	}
    catch (nlohmann::json::exception& e) {
        fprintf(stderr,"Format Error in %s (%s)\n", fname, e.what());
		return false;
	}
    bool ret = true;
    try {
        json jdata = jroot["speedtraction"];
        for(json::iterator it = jdata.begin(); it != jdata.end(); ++it) {
            std::shared_ptr<SpeedTraction> sptr = std::make_shared<SpeedTraction>();
            ret = sptr->read_jsonfile(*it);
            if( ret == false) throw std::runtime_error("not found speedtraction");
            sptr_list.push_back(std::move(sptr));
        }
        jdata = jroot["motor"];
        for(json::iterator it = jdata.begin(); it != jdata.end(); ++it) {
            std::shared_ptr<Motor> motor = std::make_shared<Motor>();
            ret = motor->read_json(*it);
            if( ret == false) throw std::runtime_error("not found motor");
            motors.push_back(std::move(motor));
        }

        jdata = jroot["train"];
        for(json::iterator it = jdata.begin(); it != jdata.end(); ++it) {
            std::shared_ptr<Train> train = std::make_shared<Train>();
            ret = train->read_json(*it);
            if( ret == false) throw std::runtime_error("not found train");
            trains.push_back(std::move(train));
        }
        jdata = jroot["line"];
        for(json::iterator it = jdata.begin(); it != jdata.end(); ++it) {
            std::shared_ptr<RailLine> line = std::make_shared<RailLine>();
            int id = it->at("id");
            line->setID(id);
            std::string line_file_name = it->at("fname");
            int ret = line->read(line_file_name.c_str());
            if ( ret <= 0 ) {
                errmsg = "ERROR " + std::to_string(ret) +": Line file (" + line_file_name + ").\n";
                return false;
            }
            lines.push_back(line);
        }
        if (jdata.find("maxpt") != jdata.end()) {
            mSvgMaxpt = jdata.at("maxpt");
            if (mSvgMaxpt <= 0)  mSvgMaxpt = 0;
        }

        set_train_motor();
        set_train_traction();
    } catch(nlohmann::json::exception& e) {
        fprintf(stderr,"Error: %s\n", e.what() );
        return false;
    }
    catch (std::runtime_error& e) {
        fprintf(stderr, "Error: %s\n", e.what());
        return false;
    }
    if ( ret ) {
        errmsg = "";
        // sptr->print();
    }
    else errmsg = "ERROR: Reading the Control file (" + std::string(fname) + ").\n";
    return ret;
}
//-----------------------------------------------------------------------------
// Identify motor to trains
//-----------------------------------------------------------------------------
void RunControl::set_train_motor() {
    for(const auto& train: trains) {
        int tid = train->motor_index;
        for(const auto& motor: motors) {
            if( tid == motor->id ) {
                train->set_motor(motor);
                break;
            }
        }
    }
}

//-----------------------------------------------------------------------------
// Identify speed traction relationship to tains
//-----------------------------------------------------------------------------
void RunControl::set_train_traction() {
    for(const auto& train: trains) {
        int tid = train->speed_traction_index;
        for(const auto& sptr: sptr_list) {
            if( tid == sptr->id ) {
                train->set_speed_traction(sptr);
                break;
            }
        }
    }
}
//-----------------------------------------------------------------------------
// Identify line to tains
//-----------------------------------------------------------------------------
bool RunControl::set_train_line() {
    for(const auto& train: trains) {
        int tid = train->line_index;
        for(const auto& line: lines) {
            if( tid == line->getID()) {
                if( train->set_line(line) == false) return false;
                break;
            }
        }
    }
    return true;
}
//-----------------------------------------------------------------------------
// Use the first train data and use the line data of line_id
//-----------------------------------------------------------------------------
int RunControl::run1(const char* fname) {
    FILE* fp;
    printf("[0] start.\n");

    set_train_traction();
    if( set_train_line() == false) {
        fprintf(stderr,"Station length must be longer than the train length\n");
        return(-3);
    }
    std::shared_ptr<Train> train = trains.front();
    if( train->get_line() == nullptr ) {
        fprintf(stderr,"Line id of the train is not found.\n");
        return(-2);
    }
    errno_t err = fopen_s(&fp, fname, "wt");
    if ( err != 0 ) {
        fprintf(stderr, "Cannot create file %s\n", fname);
        return (-1);
    }
    printf("[1] Prepare calculation.\n");
    train->prepare_run();
    int counter = 0;
    int information = 0;
    printf("[2] Start Calculation.\n");
    fprintf(fp, "status\ttime\tdistance\tspeed\taccel\tforce\tpower\n");
    train->run_print(fp);
    while(true) {
         //printf("[%5d]\n", counter);
        int result = train->main_run();
		if( result == RunCode::LessPower ) {
            errmsg = "Too low power.";
            train->run_print(fp);
            fclose(fp);
			return (-3);
		}
        if ( counter % 16 == 0 ) information = 1;
        else information = 0;

        if ( information == 1) {
            train->run_print(fp);
        }
        else train->run_print(fp);
        if ( result == RunCode::EndOfLine ) break;
        counter++;
    }
    fclose(fp);
    printf("[3] End Calculation\n");
    present_train = train;
    present_line = train->get_line();
    return (0);
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void RunControl::traction_test(const char* fname) {
    FILE* fp;
    errno_t err = fopen_s(&fp, fname, "wt");
    if (err != 0) {
        printf("Cannot create file %s\n", fname);
        return;
    }
    std::shared_ptr<Train> train = trains.front();
    fprintf(fp, "Speed(km/h)\tTraction(N)\n");
    for (int i = 0; i < 101; i++) {
        double v = (double)i;
        double t = train->get_force(v);
        fprintf(fp, "%d\t%f\n", i, t);
    }
    fclose(fp);
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void RunControl::print_data() {
    for(const auto& x : sptr_list) {
        x->test_print();
    }
    for(const auto&x : lines) {
        x->test_print();
    }
}