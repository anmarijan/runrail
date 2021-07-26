#define VERSION "0.2.0"
#ifndef GITVERSION
	#define GITVERSION "a"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <memory>

#include "RailLine.h"
#include "RunControl.h"
#include "SVGConv.h"

int main(int argc, char** argv) {

    RunControl ctrl;
	if ( argc != 4 ) {
		printf("\nrunrail verssion %s-%s\n\n", VERSION, GITVERSION);
		printf("Usage: runrail control result svg\n\n");
		printf("control: Control file name (INPUT)\n");
		printf("result : Result file name (OUTPUT)\n");
		printf("svg    : SVG file name (OUTPUT)\n");
		exit(1);
	}
	if ( ctrl.read_params(argv[1]) == false) {
		fprintf(stderr,"[Failure in reading %s]\n", argv[1]);
		fprintf(stderr,"%s", ctrl.errmsg.c_str());
		return (-1);
	}
	// ctrl.print_data();
	int ret = ctrl.run1(argv[2]);
	if ( ret < 0 ) {
		fprintf(stderr, "%s\n", ctrl.errmsg.c_str() );
		fprintf(stderr, "Error Code: %d\n", ret);
		exit(1);
	}
	SVGConvert svgc;
	// line->test_print();
	svgc.read_rail(ctrl.present_line);

	if( svgc.load(argv[2]) ) {
		svgc.set_limit();
		svgc.svg_save(argv[3]);
	} else {
		fprintf(stderr, "Output file reading error");
	}

	return 0;
}
