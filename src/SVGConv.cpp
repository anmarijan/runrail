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
    for(const auto& point: pts) {
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
//---------------------------------------------------------------------------------------
// Convert the coordinates 
//---------------------------------------------------------------------------------------
void SVGPolyline::convert(convfunc func) {
    double rx, ry;
    for (auto& element : pts) {
        func(element.x, element.y, rx, ry);
        element.x = rx;
        element.y = ry;
    }
}
void SVGLine::print_svg(FILE* fp) {
    fprintf(fp, "<line x1=\"%g\" y1=\"%g\" x2=\"%g\" y2=\"%g\" />\n", x1, y1, x2, y2);
}
void SVGPolyline::print_svg(FILE* fp) {
    double rx, ry;

    fprintf(fp,"<polyline fill=\"none\" points=\"");
    std::list<SVGPoint>::const_iterator it = pts.cbegin();
    rx = it->x;
    ry = it->y;
    fprintf(fp,"%g,%g", rx, ry);
    ++it;
    while( it != pts.cend() ) {
        rx = it->x;
        ry = it->y;
        fprintf(fp," %g,%g", rx, ry);
        ++it;
    }
    fprintf(fp, "\" />\n");
}
void SVGCurve::print_svg(FILE* fp) {
    fprintf(fp, "<path d=\"M%g,%g C%g,%g %g,%g %g,%g\">\n",
        pt[0].x, pt[0].y, pt[1].x, pt[1].y, pt[2].x, pt[2].y, pt[3].x, pt[3].y);

}
void SVGText::print_svg(FILE* fp) {
    fprintf(fp,"<text x=\"%g\" y=\"%g\">%s</text>\n", x, y, str.c_str());
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
    simple_dist = 2;
    xlim_min = ylim_min = 0;
    xlim_max = 0;
    ylim_max = 10;
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
            track.push_back(bg_point(item.distance, pre_speed));
        }
        track.push_back(bg_point(item.distance, item.speed));
        dist = item.distance + item.length;
        pre_speed = item.speed;
        if( base_axis_y < pre_speed) base_axis_y = pre_speed;
        start++;
    }
    track.push_back(bg_point(dist, pre_speed));
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
            track.push_back(bg_point(item.distance, pre_speed));
        }
        track.push_back(bg_point(item.distance, item.speed));
        dist = item.distance + item.length;
        pre_speed = item.speed;
        if( base_axis_y < pre_speed) base_axis_y = pre_speed;
        start++;
    }
    track.push_back(bg_point(dist, pre_speed));
    return true;
}
//---------------------------------------------------------------------------------------
// The maximum numbre of points = 1000
//---------------------------------------------------------------------------------------
bool SVGConvert::load(const char* fname) {
    using namespace boost::assign;
    
    std::string str;
    std::list<ResultData> results;
    // Load the data into the memory
    std::ifstream fi(fname);
    if (!fi) return false;
    // Header file
    if (std::getline(fi, str)) {
        while (std::getline(fi, str)) {
            if (str.empty()) return false;
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
    }
    else {
        return false;
    }
    fi.close();
    /* Calculate xlim_max and xlim_may based on base_axis_x and base_axis_y */
    set_limit();
    /* Construct svg_items */
    int pre_status = -1;
    double pre_distance = 0;
    double pre_speed = 0;
    std::size_t count = 1;
    boost::geometry::model::linestring<bg_point> data;
    data += bg_point(0.0, 0.0);
    for(list<ResultData>::const_iterator it = results.cbegin(); it != results.cend(); ++it) {
        if( (pre_status != -1 && pre_status != it->status) || count == results.size()) {
            if( pre_status == 3 ) {
                // in station, no move
            } if(data.size() == 2 ) {
                svg_items += data;
            } else if (data.size() < 1000) {
                svg_items += data; 
            }
            data.clear();
            data.push_back(bg_point(pre_distance, pre_speed));
        }
        if (data.size() >= 1000) {
            svg_items += data;
            data.clear();
        }
        data.push_back(bg_point(it->distance, it->speed));
        pre_distance = it->distance;
        pre_speed = it->speed;
        pre_status = it->status;
        count++;
    }
    return true;
}
//-----------------------------------------------------------------------------
// Convert distance(km)-speed(km/h) to screen x-y points
//-----------------------------------------------------------------------------
void SVGConvert::convert_func(double x, double y, double& rx, double& ry) {
    double glwidth = xlim_max - xlim_min;
    double glheight = ylim_max - ylim_min;
    double scale_x = svg_axis_x / glwidth;
    double scale_y = svg_axis_y / glheight;
    rx = (x-xlim_min)*scale_x + xmargin;
    ry = svg_axis_y - (y-ylim_min)*scale_y + ymargin;
}
//-----------------------------------------------------------------------------
// Set the maximam distance for the simplify method
//-----------------------------------------------------------------------------
void SVGConvert::set_simplify(double a) {
    if (a >= 0) simple_dist = a;
    else simple_dist = 0;
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
// Convert and simplify the raw data before print
//-----------------------------------------------------------------------------
void SVGConvert::print_lines(FILE* fp, const bg_linestring& item) {
    //for (const auto& item : svg_items) {
        bg_linestring converted;
        bg_linestring output;
        for (const auto& p : item) {
            double rx, ry;
            convert_func(p.x(), p.y(), rx, ry);
            converted.push_back(bg_point(rx,ry));
        }
        fprintf(fp, "<polyline fill=\"none\" points=\"");
        boost::geometry::simplify(converted, output, simple_dist);
        int counter = 0;
        for (const auto& cp : output) {
            if (counter == 0) {
                fprintf(fp, "%g,%g", cp.x(), cp.y());
            }
            else fprintf(fp, " %g,%g", cp.x(), cp.y());
            counter++;
        }
        fprintf(fp, "\" />\n");
    //}
}
//-----------------------------------------------------------------------------
// Todo: make subroutines
//-----------------------------------------------------------------------------
void SVGConvert::svg_print(FILE* fp) {
    // Original data is in screen unit
    fprintf(fp, "<?xml version=\"1.0\"?>\n");
    fprintf(fp, "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"1000\" height=\"800\" viewBox=\"0 0 1000 800\">\n");
    // font style
    fprintf(fp, "<style>text{font-family:Arial,sans-serif;}</style>\n");
    // box
    fprintf(fp,"<rect x=\"%g\" y=\"%g\" width=\"%g\" height=\"%g\" fill=\"none\" stroke=\"black\" stroke-width=\"1\" />\n",
        xmargin,ymargin, svg_axis_x, svg_axis_y);
    fprintf(fp,"<g stroke=\"green\" stroke-width=\"1\">\n");
    
    for (const auto& pointer : svg_items) {
        print_lines(fp, pointer);
    }
    fprintf(fp,"</g>\n");
    fprintf(fp,"<g stroke=\"red\" >\n");
    print_lines(fp, track);
    fprintf(fp,"</g>\n");
    /*************************************
    // xtics
    *************************************/
    set_xtics();
    fprintf(fp,"<g stroke=\"black\" stroke-width=\"1\">\n");
    for(auto& item: xtics) {
        item.print_svg(fp);
    }
    fprintf(fp,"</g>\n");
    /*************************************
    // xlabels
    *************************************/
    fprintf(fp,"<g font-family=\"Courier New\" font-size=\"16\">\n");
    for(auto& item: xlabels) {
        item.print_svg(fp);
    }
    fprintf(fp,"</g>\n");
    /*************************************
    // ytics
    *************************************/
    set_ytics();
    fprintf(fp,"<g stroke=\"black\" stroke-width=\"1\">\n");
    for(auto& item: ytics) {
        item.print_svg(fp);
    }
    fprintf(fp, "</g>\n");
    /*************************************
    // ylabels
    *************************************/
    fprintf(fp,"<g font-family=\"Courier New\" font-size=\"16\">\n");
    for(auto& item: ylabels) {
        item.print_svg(fp);
    }
    fprintf(fp,"</g>\n");
    // labels
    double x = xmargin + svg_axis_x/2 - 6.0*12.0;
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

