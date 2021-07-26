#include <stdio.h>
#include <stdlib.h>
#include "SVGConv.h"

int main(int argc, char* argv[]) {
    SVGConvert svgc;
    if ( argc < 3) {
        printf("Usage: SVGConv input track");
        exit(1);
    }
    if( svgc.load(argv[1]) == false ) {
        fprintf(stderr,"Cannnot read file %s\n", argv[1]);
        exit(1);
    }
    if( svgc.read_rail(argv[2]) == false ) {
        fprintf(stderr,"Cannnot read file %s\n", argv[2]);
        exit(1);
    }
    svgc.set_limit();
    svgc.svg_save("test3.svg");
    // svgc.test_print();
}