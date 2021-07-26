#include <stdio.h>
#include <cmath>
#include <string>
#include <fstream>
#include <sstream>
#include <memory>
#include <list>
#include <vector>
#include "RailLine.h"
#include "SVGConv.h"
///////////////////////////////////////////////////////////////////////////////
using namespace std;
//-----------------------------------------------------------------------------
//  For debug
//-----------------------------------------------------------------------------
void SVGLine::test_print() {
    printf("[LINE] (%f,%f) (%f,%f)\n", x1, y1, x2, y2);
}
void SVGPolyline::test_print() {
    printf("[POLYLINE]");
    for(const auto point: pts) {
        printf(" (%f,%f)", point.x, point.y);
    }
    printf("\n");
}
void SVGCurve::test_print() {
    printf("[CURVE]");
    for(int i=0; i < 4; i++) printf(" (%g,%g)", pt[i].x,pt[i].y);
    printf("\n");
}
void SVGText::test_print() {
    printf("[TEXT] (%g,%g) %s\n", x, y, str.c_str());
}
void SVGLine::print_svg(FILE* fp, convfunc func) {
    double rx1, ry1, rx2, ry2;
    func(x1,y1,rx1,ry1);
    func(x2,y2,rx2,ry2);
    fprintf(fp,"<line x1=\"%g\" y1=\"%g\" x2=\"%g\" y2=\"%g\" />\n",
        rx1,ry1,rx2,ry2);
}
void SVGPolyline::print_svg(FILE* fp, convfunc func) {
    double rx, ry;

    fprintf(fp,"<polyline fill=\"none\" points=\"");
    std::list<SVGPoint>::const_iterator it = pts.cbegin();
    func(it->x, it->y, rx, ry);
    fprintf(fp,"%g,%g", rx, ry);
    ++it;
    while( it != pts.cend() ) {
        func(it->x, it->y, rx, ry);
        fprintf(fp," %g,%g", rx, ry);
        ++it;
    }
    fprintf(fp, "\" />\n");
}
void SVGCurve::print_svg(FILE* fp, convfunc func) {
    SVGPoint cvpt[4];
    for(int i=0; i < 4; i++) {
        func(pt[i].x, pt[i].y, cvpt[i].x, cvpt[i].y);
    }
    fprintf(fp,"<path d=\"M%g,%g C%g,%g %g,%g %g,%g\">\n",
        cvpt[0].x,cvpt[0].y,cvpt[1].x,cvpt[1].y, cvpt[2].x,cvpt[2].y,cvpt[3].x,cvpt[3].y);
}
void SVGText::print_svg(FILE* fp, convfunc func) {
    double sx, sy;
    func(x,y,sx,sy);
    fprintf(fp,"<text x=\"%g\" y=\"%g\">%s</text>\n", sx, sy, str.c_str());
}

//---------------------------------------------------------------------------------------
// SVGConverter: Constructor
//---------------------------------------------------------------------------------------
SVGConvert::SVGConvert() {
    svg_axis_x = 800; svg_axis_y = 500;
    xmargin = 100; ymargin = 100;
    base_axis_x = 0;
    base_axis_y = 0;
    xtic_unit = 500;
}
//---------------------------------------------------------------------------------------
// Read rail data
//---------------------------------------------------------------------------------------
bool SVGConvert::read_rail(const char* fname) {
    std::vector<Segment> segs;
    int c = loadsegdata(fname, &segs);
    if ( c < 0 ) return false;
    track.clear();
    int start = 0;
    double dist = 0;
    double pre_speed = 0;
    for(const auto& item: segs) {
        if( start > 0) {
            track.add(item.distance, pre_speed);
        }
        track.add(item.distance, item.speed);
        dist = item.distance + item.length;
        pre_speed = item.speed;
        if( base_axis_y < pre_speed) base_axis_y = pre_speed;
        start++;
    }
    track.add(dist, pre_speed);
    return true;
}
//---------------------------------------------------------------------------------------
// Read rail data
//---------------------------------------------------------------------------------------
bool SVGConvert::read_rail(std::shared_ptr<RailLine>& line) {
    std::vector<Segment> segs;
    if ( !line ) return false;
    track.clear();
    int start = 0;
    double dist = 0;
    double pre_speed = 0;
    for(const auto& item: line->segs) {
        if( start > 0) {
            track.add(item.distance, pre_speed);
        }
        track.add(item.distance, item.speed);
        dist = item.distance + item.length;
        pre_speed = item.speed;
        if( base_axis_y < pre_speed) base_axis_y = pre_speed;
        start++;
    }
    track.add(dist, pre_speed);
    return true;
}
//---------------------------------------------------------------------------------------
// The maximum numbre of points = 1000
//---------------------------------------------------------------------------------------
bool SVGConvert::load(const char* fname) {
    std::ifstream fi(fname);
    if (!fi) return false;
    std::string str;
    std::list<ResultData> results;
    // Load the data into the memory

    while( std::getline(fi,str) ) {
        if ( str.empty() ) return false;
        std::istringstream iss(str);
        ResultData d;
        iss >> d;
        if (!iss) {
            return false;
        }
        if (d.distance > base_axis_x) base_axis_x = d.distance;
        if (d.speed > base_axis_y) base_axis_y = d.speed;
        results.push_back(d);
    }
    fi.close();

    int pre_status = -1;
    double pre_distance = 0;
    double pre_speed = 0;
    std::size_t count = 1;
    list<SVGPoint> data;
    SVGPoint pt = SVGPoint(0,0);
    data.push_back(pt);
    for(list<ResultData>::const_iterator it = results.cbegin(); it != results.cend(); ++it) {
        if( (pre_status != -1 && pre_status != it->status) || count == results.size()) {
            if( pre_status == 3 ) {
                // in station, no move
            } if(data.size() == 2 ) {
                double x1 = data.front().x; double y1 = data.front().y;
                double x2 = data.back().x;  double y2 = data.back().y;
                svg_items.emplace_back(make_unique<SVGLine>(x1,y1,x2,y2));
            } else if (data.size() < 1000) {
                unique_ptr<SVGPolyline> pointer = make_unique<SVGPolyline>();
                for(list<SVGPoint>::const_iterator pi=data.cbegin(); pi != data.cend(); ++pi) {
                    double x = pi->x; double y = pi->y;
                    pointer->add(x,y);
                }
                svg_items.push_back(move(pointer));
            }
            data.clear();
            data.emplace_back(SVGPoint(pre_distance, pre_speed));
        }
        data.emplace_back(SVGPoint(it->distance, it->speed));
        pre_distance = it->distance;
        pre_speed = it->speed;
        pre_status = it->status;
        count++;
    }
    return true;
}
void SVGConvert::convert_func(double x, double y, double& rx, double& ry) {
    double glwidth = xlim_max - xlim_min;
    double glheight = ylim_max - ylim_min;
    double scale_x = svg_axis_x / glwidth;
    double scale_y = svg_axis_y / glheight;
    rx = (x-xlim_min)*scale_x + xmargin;
    ry = svg_axis_y - (y-ylim_min)*scale_y + ymargin;
}
//-----------------------------------------------------------------------------
// Dummy function when the same coordiantes are used.
//-----------------------------------------------------------------------------
void SVGConvert::copy_func(double x, double y, double& rx, double& ry) {
    rx = x; ry = y;
}
//-----------------------------------------------------------------------------
// speed = 10km/h interval
//-----------------------------------------------------------------------------
void SVGConvert::set_limit() {
    xlim_min = ylim_min = 0;
    xlim_max = std::ceil(base_axis_x);
    ylim_max = std::floor(base_axis_y/10)*10 + 10;
}
//-----------------------------------------------------------------------------
// unit = m if it is less than 1000m, otherwise unit is in km
//-----------------------------------------------------------------------------
void SVGConvert::set_xtic_unit() {
    double x = (xlim_max - xlim_min)/10;
    double y = 1;
    while( x/y >= 1 ) {
        y *= 10;
    }
    y /= 10;
    double u = std::ceil(x/y) * y;
    xtic_unit = std::ceil(u/(y*5))*(y*5);
}
//-----------------------------------------------------------------------------
// Set x tics (maximum 10 tics)
//-----------------------------------------------------------------------------
void SVGConvert::set_xtics() {
    char buff[12];
    set_xtic_unit();
    int low_index = (int)(xlim_min/xtic_unit)+1;
    int high_index = (int)(xlim_max/xtic_unit)+1;
    for(int i=low_index; i < high_index; i++) {
        double x = xtic_unit * i;
        double sx, sy;
        convert_func(x,ylim_min, sx, sy);
        xtics.emplace_back(SVGLine(sx,sy,sx,sy+10));
        sprintf_s(buff, 12, "%g", i*xtic_unit/1000);
        std::string str = buff;
        xlabels.emplace_back(SVGText(sx-12,sy+30, str));
    }
}
//-----------------------------------------------------------------------------
// Set y tics (maximum 10 tics)
//-----------------------------------------------------------------------------
void SVGConvert::set_ytics() {
    int i=0;
    while ( true ) {
        i++;
        double ty = 10.0 * i;
        if( ty > ylim_max) break;
        double sx, sy;
        convert_func(xlim_min, ty, sx, sy);
        ytics.emplace_back(SVGLine(sx-10,sy,sx,sy));
        std::string str = std::to_string(10*i);
        ylabels.emplace_back(SVGText(sx-46,sy+6,str));
    }
}
//-----------------------------------------------------------------------------
// Todo: make subroutines
//-----------------------------------------------------------------------------
void SVGConvert::svg_print(FILE* fp) {

    // simulation unit -> screeen unit
    convfunc func = [&](double x, double y, double& rx, double& ry)
    {return this->convert_func(x,y,rx,ry);};
    // Original data is in screen unit
    convfunc dummy_func = [&](double x, double y, double& rx, double& ry) {
        return this->copy_func(x,y,rx,ry);
    };
    fprintf(fp, "<?xml version=\"1.0\"?>\n");
    fprintf(fp, "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"1000\" height=\"800\" viewBox=\"0 0 1000 800\">\n");
    // box
    fprintf(fp,"<rect x=\"%g\" y=\"%g\" width=\"%g\" height=\"%g\" fill=\"none\" stroke=\"black\" stroke-width=\"1\" />\n",
        xmargin,ymargin, svg_axis_x, svg_axis_y);
    fprintf(fp,"<g stroke=\"green\" stroke-width=\"1\">\n");
    for(const auto& pointer: svg_items) {
        pointer->print_svg(fp, func);
    }
    fprintf(fp,"</g>\n");
    fprintf(fp,"<g stroke=\"red\" >\n");
    track.print_svg(fp,func);
    fprintf(fp,"</g>\n");
    /*************************************
    // xtics
    *************************************/
    set_xtics();
    // fprintf(fp,"<g stroke=\"black\" stroke-width=\"1\" transform=\"translate(0.5,0.5)\">\n");
    fprintf(fp,"<g stroke=\"black\" stroke-width=\"1\">\n");
    for(auto& item: xtics) {
        item.print_svg(fp, dummy_func);
    }
    fprintf(fp,"</g>\n");
    /*************************************
    // xlabels
    *************************************/
    fprintf(fp,"<g font-family=\"Courier New\" font-size=\"16\">\n");
    for(auto& item: xlabels) {
        item.print_svg(fp, dummy_func);
    }
    fprintf(fp,"</g>\n");
    /*************************************
    // ytics
    *************************************/
   set_ytics();
    // fprintf(fp,"<g stroke=\"black\" stroke-width=\"1\" transform=\"translate(0.5,0.5)\">\n");
    fprintf(fp,"<g stroke=\"black\" stroke-width=\"1\">\n");
    for(auto& item: ytics) {
        item.print_svg(fp,dummy_func);
    }
    fprintf(fp, "</g>\n");
    /*************************************
    // ylabels
    *************************************/
    fprintf(fp,"<g font-family=\"Courier New\" font-size=\"16\">\n");
    for(auto& item: ylabels) {
        item.print_svg(fp, dummy_func);
    }
    fprintf(fp,"</g>\n");
    // labels
    double x = xmargin + svg_axis_x/2 - 6*12;
    double y = ymargin + svg_axis_y + 20 + 50;
    fprintf(fp,"<text x=\"%g\" y=\"%g\" font-family=\"Courier New\" font-size=\"16\">Distance(km)</text>\n", x, y);
    x = 30;
    y = ymargin + svg_axis_y/2;
    fprintf(fp,"<text x=\"%g\" y=\"%g\" font-family=\"Courier New\" font-size=\"16\" transform=\"rotate(-90,%g,%g)\">Speed(km/h)</text>\n", x, y, x, y);
    fprintf(fp,"</svg>\n");
}
//-----------------------------------------------------------------------------
// Save as a svg file
//-----------------------------------------------------------------------------
bool SVGConvert::svg_save(const char* fname) {
    FILE* fp;
    errno_t err = fopen_s(&fp, fname, "wt");
    if ( err != 0 ) {
        fprintf(stderr, "Cannot create file %s\n", fname);
        return false;
    }
    svg_print(fp);
    fclose(fp);
    return true;
}

void SVGConvert::test_print() {
    for(const auto& pointer: svg_items) {
        pointer->test_print();
    }
}
