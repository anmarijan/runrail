/**
 * Make a svg file of the run curve from the result file and the track file
 */
#ifndef SVGCONV_H
#define SVGCONV_H
///////////////////////////////////////////////////////////////////////////////
#include <iostream>
#include <memory>
#include <list>
#include <vector>
#include <functional>
///////////////////////////////////////////////////////////////////////////////
// For simplify
#include <boost/geometry.hpp>
//#include <boost/geometry/algorithms/simplify.hpp>
#include <boost/geometry/geometries/linestring.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/assign.hpp>
///////////////////////////////////////////////////////////////////////////////
using convfunc = std::function< void(double, double, double&, double&) >;
typedef boost::geometry::model::d2::point_xy<double> bg_point;
typedef boost::geometry::model::linestring<bg_point> bg_linestring;

class SVGPoint {
public:
    double x, y;
    SVGPoint(): x(0), y(0) {}
    SVGPoint(double ix, double iy): x(ix), y(iy) {}
};

class SVGObject {
    int id;
public:
    SVGObject(): id(0) {};
    virtual void print_svg(FILE* fp) = 0;
    virtual void test_print() = 0;
};

class SVGLine : public SVGObject {
    double x1, x2, y1, y2;
public:
    SVGLine(){
        x1 = x2 = y1 = y2 = 0;
    };
    SVGLine(double ix1, double iy1, double ix2, double iy2) {
        x1 = ix1; x2 = ix2; y1 = iy1; y2 = iy2;
    };
    void set(double x1, double y1, double x2, double y2) {
        this->x1 = x1; this->x2 = x2; this->y1 = y1; this->y2=y2;
    }
    void print_svg(FILE* fp);
    void test_print();
};

class SVGPolyline : public SVGObject {
    std::list<SVGPoint> pts;
public:
    void add(double x, double y) {
        pts.emplace_back(SVGPoint(x,y));
    }
    void clear() {
        pts.clear();
    }
    void print_svg(FILE* fp);
    void convert(convfunc func);
    void test_print();
};

class SVGCurve : public SVGObject {
    SVGPoint pt[4];
public:
    void add(const SVGPoint& p1,const SVGPoint& p2,const SVGPoint& p3,const SVGPoint& p4) {
        pt[0] = p1; pt[1] = p2; pt[2] = p3; pt[3]=p4;
    }
    void print_svg(FILE* fp);
    void test_print();
};

class SVGText : public SVGObject {
    std::string str;
    double x,y;
public:
    SVGText() {x=0; y=0;};
    SVGText(double ix, double iy, std::string& istr) {
        x = ix; y = iy; str = istr;
    };
    void print_svg(FILE* fp);
    void test_print();
};
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
class ResultData {
public:
    int status;
    double total_time, distance, speed, accel,force,power;
public:
    friend std::istream& operator >> (std::istream& in, ResultData& d) {
        in >> d.status >> d.total_time >> d.distance >> d.speed >> d.accel >> d.force >> d.power;
        return in;
    }
};
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
class SVGConvert {
    double base_axis_x, base_axis_y; // max value of distance and speed
    double xlim_min, xlim_max, ylim_min, ylim_max;
    double svg_axis_x, svg_axis_y;
    double xmargin, ymargin;
    double xtic_unit;  // 500m interval
    std::list<bg_linestring> svg_items;
    bg_linestring track;
    std::list<SVGLine> xtics;
    std::list<SVGText> xlabels;
    std::list<SVGLine> ytics;
    std::list<SVGText> ylabels;
    double simple_dist;   // used for the simplify function    
public:
    SVGConvert();
    bool load(const char* fname);
    bool read_rail(const char* fname);
    bool read_rail(std::shared_ptr<RailLine>& line);
    void convert_func(double x, double y, double& rx, double& ry);
    void set_limit();
    void set_simplify(double a);
    void svg_print(FILE * fp);
    bool svg_save(const char* fname);
protected:
    void print_lines(FILE* fp, const bg_linestring& ls);
    void set_xtic_unit();
    void set_xtics();
    void set_ytics();
};

#endif
