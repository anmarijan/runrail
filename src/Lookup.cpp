#include <stdio.h>
#include <new>
#include <memory>
#include <iostream>
///////////////////////////////////////////////////////////////////////////////
// for json
/*
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/optional.hpp>
#include "nlohmann/json.hpp"
using namespace boost::property_tree;
*/
#include "nlohmann/json.hpp"
///////////////////////////////////////////////////////////////////////////////
using namespace nlohmann;
//----------------------------------------------------------------------------
#include "Lookup.h"
//----------------------------------------------------------------------------
Lookup::Lookup() {
	id = 0;
	Fsize = 0;
	min_speed = 10;
	max_speed = 500;
};
//-----------------------------------------------------------------------------
//  Initialize
//-----------------------------------------------------------------------------
void Lookup::init(size_t s) {
	Fsize = 0;
	if ( s > 0 ) {
		try {
			index.resize(s);
			value.resize(s);
			Fsize = s;
		} catch (std::bad_alloc& ) {
			index.clear();
			value.clear();
        }
	}
};
//-----------------------------------------------------------------------------
//  Initialize: s=size, data= size x 2 table array
//-----------------------------------------------------------------------------
void Lookup::init(size_t s, const double data[][2]) {
	init(s);
	if( Fsize > 0 ) {
		for(size_t i=0; i < Fsize; i++) {
			index[i] = data[i][0];
			value[i] = data[i][1];
        }
    }
}
void Lookup::set(size_t n, double x, double y) {
	if( n >= 0 && n < Fsize ) {
		index[n] = x;
		value[n] = y;
	}
};

double Lookup::get_index(int i) const {
	return index[i];
}

double Lookup::get_value(int i) const {
	return value[i];
}


void Lookup::index_sort()
{
	double temp;
	for(size_t i=0; i < Fsize-1; i++) {
		for(size_t j=1; j < Fsize-1-i; j++) {
			if( index[j] < index[j-1] ) {
				temp = index[j];
				index[j] = index[j-1];
				index[j-1]=temp;
				temp = value[j];
				value[j] = value[j-1];
				value[j-1]=temp;
			}
		}
	}
}
//-----------------------------------------------------------------------------
 // return interpolated value
 // index[0] should be minimum of valid x
//-----------------------------------------------------------------------------
double Lookup::midval(double x) const {
	double result = 0.0;

    if ( size() == 0 || x < index[0]) result = 0.0;
    else if ( x >= index[size()-1] ) result = value[size()-1];
    else {
        for(size_t i=1; i < size(); i++) {
            double a = value[i-1];
            double b = value[i];
            if( x < index[i] ) {
                if( index[i-1] == index[i] ) result = b;
                else result = (b - a) /(index[i] - index[i-1]) * (x - index[i-1]) + a;
                break;
            }
        }
    }
    return result;
};


double Lookup::lowval(double x) const {
	double result = min_speed;
	if( size() == 0 ) result = 0;
	else if (x < index[0]) result = min_speed;
	else { 
		for(size_t i=size()-1; i >= 0; i--) {
			if( x >= index[i] ) {
				result = value[i];
				break;
			}
		}
	}
	return result;
};

double Lookup::highval(double x) const {
	double result = min_speed;
	if( size() == 0 ) result = 0;
	else {
		for(size_t i=0; i < size(); i++) {
			if(x <= index[i]) {
				result = value[i];
				break;
			}
		}
	}
	return result;
}

//-----------------------------------------------------------------------------
// Read json data (nlohmann)
//-----------------------------------------------------------------------------
bool Lookup::read_jsonfile(const nlohmann::json& jdata) {
	if( jdata.is_array() ) {
		fprintf(stderr,"Format error (array)\n");
		return false;
	}
	try {
		id = jdata.at("id");
		label_x = jdata.at("labels")[0];
		label_y = jdata.at("labels")[1];
		unit_x = jdata.at("units")[0];
		unit_y = jdata.at("units")[1];
		json j = jdata.at("data");
		std::size_t s = j.size();
		init(s);
		int i=0;
		for(json::iterator it = j.begin(); it != j.end(); ++it) {
			if( (*it).is_array() ) {		
				index[i] = (*it)[0];
				value[i] = (*it)[1];
			} else {
				fprintf(stderr,"Format error (not array)\n");
			}
			i++;
		}
	} catch(json::exception& e) {
		fprintf(stderr,"%s\n", e.what());
		return false;
	}
	return true;
}

/*
bool Lookup::read_jsonfile(const char* fname) {
	ptree pt;
	ptree pt_data;
	try {
		read_json(fname, pt);
		printf("Loaded\n");
		pt_data = pt.get_child("data");
		int s = pt_data.size();
		init(s);
		int i=0;
		for (ptree::const_iterator row = pt_data.begin(); row != pt_data.end(); ++row){
			ptree::const_iterator it = row->second.begin();
			index[i] = it->second.get_value<double>();
			++it;
			value[i] = it->second.get_value<double>();
            i++;
        }
    } catch(const ptree_error& e) {
        fprintf(stderr,"[Error] %s\n", e.what() );
        return false;
    }
    return true;
};
*/
void Lookup::test_print() {
	printf("id: %d\n", id);
	printf("Lables: %s %s\n", label_x.c_str(), label_y.c_str());
	printf("Units : %s %s\n", unit_x.c_str(), unit_y.c_str());
	for(size_t i=0; i < Fsize; i++) {
		printf("%f\t%f\n", index[i], value[i]);
	}
}