#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// A simple function that performs Delaunay triangulation
int Cdelaunay(char* filenames) {

    // "pts.dat output.dat"
    char* input_file = strtok(filenames, " ");
    char* output_file = strtok(NULL, " ");

    
    printf("Input file: %s\n", input_file);
    printf("Output file: %s\n", output_file);

    return 0;

}


