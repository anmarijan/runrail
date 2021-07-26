////////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <sstream>
////////////////////////////////////////////////////////////////////////////////
#include "RailLine.h"
////////////////////////////////////////////////////////////////////////////////
const int SegmentType::Normal  = 0;
const int SegmentType::Station = 1;
const int SegmentType::Point   = 2;
//-----------------------------------------------------------------------------
// initialize the Segment class in constructor
//-----------------------------------------------------------------------------
Segment::Segment() {
	id = 0;
    type = 0;
    distance = 0.0;
    length = 0.0;
    speed = 0.0;
    gradient = 0.0;
    radius = 0.0;
	tm_stop = 0.0;
    max_speed = 0.0;
	head_only = true;
}
//-----------------------------------------------------------------------------
 // Read the Segment file
 // File format
 // id distance(m) type maximum_speed(km/h) gradient(%) curve(m)
 //----------------------------------------------------------------------------
int loadsegdata(const char* fname, std::vector<Segment>* segs) {
    std::ifstream fi(fname);
    if (!fi) return (-1);
    std::string str;
    int count = 0;
    while( std::getline(fi,str) ) {
		// skip comment lines
		if( str.length() > 0 && str[0] == '#') continue;
        int id, type;
        double distance, length, speed, gradient, radius;
        // remove whilte spaces in left
        std::size_t found = str.find_first_not_of(" \t");
        if( found != std::string::npos ) {
            str = str.substr(found);
        }
        if ( str.empty() ) continue;
        std::istringstream iss(str);
        iss >> id >> distance >> length >> type >> speed >> gradient >> radius;
        if (!iss) {
            return (-2);
        }
        Segment seg;
        seg.id = id;
        seg.type = type;
        seg.distance = distance;
		seg.length = length;
        seg.gradient = gradient;
        seg.speed = speed;
        seg.radius = radius;
		// The legnth of train is considered in slope or curve sections only
		// Todo: make a flag to identify the section 
		if( gradient != 0 || radius != 0) seg.head_only = false;
		else seg.head_only = true;
        segs->push_back(seg);
        count++;
    }
    std::vector<Segment>::reverse_iterator it = segs->rbegin() ;
    double d = (*it).distance;
	++it;
    while( it != segs->rend() ) {
        (*it).length = d - (*it).distance;
		if( (*it).length <= 0 ) {
			printf("%g %g %g\n", d , it->distance, it->length);
			return (-3);
		}
        d = (*it).distance;
        ++it;
    }
    return count;
}
//------------------------------------------------------------------------------
///  RailLine: Read line data file
//------------------------------------------------------------------------------
int RailLine::read(const char* fname) {
	segs.clear();
	return loadsegdata(fname, &segs);
}
//------------------------------------------------------------------------------
///  RailLine: Constructor
//------------------------------------------------------------------------------
RailLine::RailLine() {
	id = 0;
	FnSegment = 0;
	FnStation = 0;
};
//------------------------------------------------------------------------------
///  RailLine: get the pointer of Segment i
//------------------------------------------------------------------------------
Segment* RailLine::getSegment(int i) {
	if(i < 0 || i >= (int)segs.size()) {
		return NULL;
	}
	return &segs[i];
};
//------------------------------------------------------------------------------
///  initialize
//------------------------------------------------------------------------------
void RailLine::init(int n) {
	if( n <= 0 ) return;
	segs.resize(n);
	FnSegment = n;
}
//------------------------------------------------------------------------------
///  Clear
//------------------------------------------------------------------------------
void RailLine::clear() {
	segs.clear();
	FnSegment = 0;
	FnStation = 0;
	name.clear();
}
//------------------------------------------------------------------------------
///  set a Segment
//------------------------------------------------------------------------------
void RailLine::setSegment(int i, const Segment& s) {
	if(i< 0 || i >= FnSegment) return;
	segs[i] = s;
}
//------------------------------------------------------------------------------
///  test print
//------------------------------------------------------------------------------
void RailLine::test_print(){
	for (auto x: segs ) {
		printf("%d\t%d\t%.0f\t%.0f\t%.0f\t%.0f\n",
			x.id, x.type, x.distance, x.speed, x.gradient, x.radius);
	}
};
//------------------------------------------------------------------------------
/// set name
//------------------------------------------------------------------------------
void RailLine::set_name(const char* str)
{
	name = str;
}
//------------------------------------------------------------------------------
/// Change the own segments to the revserse line of the input RilLine r
//------------------------------------------------------------------------------
void RailLine::make_reverse(const RailLine& r)
{
	segs.resize(r.nSegment());
	double L = r.segs[r.nSegment()-1].distance;
	for(unsigned int i=0; i < r.nSegment(); i++) {
		int op = r.nSegment()-1-i;
		segs[i].type  = r.segs[op].type;
		//segs[i].notch = r.segs[op].notch;
		//segs[i].set_name(r.segs[op].name);
		segs[i].max_speed = r.segs[op].max_speed;
		segs[i].distance = L - r.segs[op].distance;
		if(op > 0) {
			segs[i].radius = r.segs[op-1].radius;
			segs[i].gradient = -r.segs[op-1].gradient;
		}
		//segs[i].tm_stop = r.segs[op].tm_stop;
		segs[i].speed = segs[i].max_speed;
//		printf("%c\t%lf\t%lf\n", segs[i].type, segs[i].distance, segs[i].max_speed);
	}
	// The number of stations must be the same
	FnStation = r.nStation();
}


void RailLine::reset()
{
	for(unsigned int i = 0; i < segs.size(); i++) {
		segs[i].speed = segs[i].max_speed;
	}
}
//------------------------------------------------------------------------------
//  Return total length
//------------------------------------------------------------------------------
double RailLine::length() const
{
	double result = 0.0;
	for ( auto x: segs ) {
		result += x.length;
	}
	return result;
}
