#include <stdio.h>
#include <stdlib.h>
#include "functions.h"

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Usage: %s <input_file> <output_file>\n", argv[0]);
        return 1;
    }
    
    // Call your Delaunay triangulation function
    int result = Cdelaunay(argv[1], argv[2]);
    
    if (result == 0) {
        printf("Delaunay triangulation completed successfully.\n");
    } else {
        printf("Error in Delaunay triangulation.\n");
    }
    
    return result;
}
