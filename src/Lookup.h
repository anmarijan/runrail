#ifndef LOOKUP_H
#define LOOKUP_H

#include <string>
#include <vector>
#include "nlohmann/json.hpp"

class LookupItem {
public:
	double index;
	double value;
public:
	LookupItem() { index = value = 0; };
	LookupItem(double x, double y) { index=x; value = y;};
};

class Lookup {
protected:
	std::vector<double> index;
	std::vector<double> value;
	int Fsize;
public:
	int id;
	std::string label_x;
	std::string label_y;
	std::string unit_x;
	std::string unit_y;
	double min_speed;
	double max_speed;

public:
	Lookup();
public:
	void init(int s);
	void init(int s, const double data[][2]);
	void set(int n, double x, double y);
	double get_index(int i) const;
	double get_value(int i) const;
	int size() const {return Fsize;}
	double lowval(double x) const;
	double midval(double x) const;
	double highval(double x) const;
	void index_sort();
	// bool read_jsonfile(const char* fname);
	bool read_jsonfile(const nlohmann::json& jdata);
	void test_print();
};

#endif
