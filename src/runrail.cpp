#define VERSION "0.2.1"
#ifndef GITVERSION
	#define GITVERSION "a"
#endif
#include <stdio.h>
#include <string>
#include <memory>
#include <boost/program_options.hpp>
//---------------------------------------------------------------------------
#include "RailLine.h"
#include "RunControl.h"
#include "SVGConv.h"
//---------------------------------------------------------------------------
std::string ctrl_fname;
std::string output_fname;
std::string svg_fname;
/////////////////////////////////////////////////////////////////////////////
void usage() {
	printf("\nrunrail version %s-%s\n\n", VERSION, GITVERSION);
	printf("Usage: runrail input output options\n\n");
	printf("  input : control file name\n");
	printf("  output: output file name\n");
}
/////////////////////////////////////////////////////////////////////////////
int main(int argc, char** argv) {
	bool test_flag = false;
	bool svg_flag = false;
	using namespace boost::program_options;
    RunControl ctrl;
	options_description description("Options");
	description.add_options()
		("svg,s", value<std::string>(), "SVG file name")
		("test,t", "Calc speed-traction relationship");

	variables_map vm;
	auto const parsing_result = parse_command_line(argc, argv, description);
	store(parsing_result, vm);
	notify(vm);
	int check_counter = 0;
	for (auto const& str : collect_unrecognized(parsing_result.options, include_positional)) {
		if (check_counter == 0) ctrl_fname = str;
		else if (check_counter == 1) output_fname = str;
		check_counter++;
	}

	if ( argc < 3 ) {
		usage();
		std::cout << description;
		return(0);
	}
	if (vm.count("test")) test_flag = true;
	if (vm.count("svg")) {
		svg_fname = vm["svg"].as<std::string>();
		svg_flag = true;
	}
	if ( ctrl.read_params(ctrl_fname.c_str()) == false) {
		printf("[Failure in reading %s]\n", argv[1]);
		printf("%s", ctrl.errmsg.c_str());
		return (-1);
	}
	if (test_flag) {
		ctrl.traction_test(output_fname.c_str());
	}
	else {
		// ctrl.print_data();
		int ret = ctrl.run1(output_fname.c_str());
		if (ret < 0) {
			printf("%s\n", ctrl.errmsg.c_str());
			printf("Error Code: %d\n", ret);
			exit(1);
		}
		if (svg_flag) {
			SVGConvert svgc;
			svgc.set_simplify(ctrl.svg_maxpt());
			// line->test_print();
			svgc.read_rail(ctrl.present_line);

			if (svgc.load(output_fname.c_str())) {
				svgc.svg_save(svg_fname.c_str());
			}
			else {
				printf("Output file reading error");
			}
		}
	}

	return 0;
}
