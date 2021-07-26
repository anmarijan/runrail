/**
 * RailLine class represents a one-way railway line.
 * It has at least the start and end stations.
 * It consists of segments each of which represents the section having
 * the same attributes.
 */
#ifndef RailLineH
#define RailLineH
////////////////////////////////////////////////////////////////////////////////
#include <vector>
#include <string>
////////////////////////////////////////////////////////////////////////////////
struct SegmentType {
	static const int Normal;
	static const int Station;  //section in a station
	static const int Point;
};
//-----------------------------------------------------------------------------
// Segment class: a section in RailLine class
// Rail switch is not a point but a segment
//-----------------------------------------------------------------------------
class Segment {
public:
    int id;
    int type;           // 0:normal 1:station 2: switch
    double distance;    // begining point of this segment (m)
    double length;      // should be the same as next->distance - this->distance
    double speed;       // maximum speed of this segment(km/h)
    double gradient;
    double radius;
	bool head_only;     // apply the max speed only to the head of a train
public:
	double tm_stop;     // stopping time at station if type = 2 (s)
    double max_speed;
public:
    Segment();
//    segment(double d, double s):
//        distance(d), speed(s){};
};
//-----------------------------------------------------------------------------
// Function to read segement data
//-----------------------------------------------------------------------------
int loadsegdata(const char* fname, std::vector<Segment>* segs);
//-----------------------------------------------------------------------------
// Railway line class
//-----------------------------------------------------------------------------
class RailLine {
private:
	int id;
	int FnSegment;
	int FnStation;
public:
	std::vector<Segment> segs;
	std::string name;
public:
	void set_name(const char* str);
	RailLine();
	void init(int n);
	void clear();
	Segment* getSegment(int i);
	void setSegment(int i, const Segment& s);
	unsigned int nSegment() const { return segs.size(); }
	int nStation() const { return FnStation; }
	double length() const;
	void make_reverse(const RailLine& r);
	int getID() const { return id;};
	void setID(int n) {id = n;};
	void reset();   // speed = max_speed
	void test_print();
	int read(const char* fname);
};

#endif
