#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "functions.h"

// A simple function that performs Delaunay triangulation
int Cdelaunay(char* input_file, char* output_file, void *myMesh) {

    // Points array read from input file
    MyMesh* mesh = createMesh(input_file);
    // Delaunay triangulation algorithm

    // Step 1: Create initial triangle
    mesh->L = 1.;
    createInitialTriangles(mesh);
    printf("Read %d points from %s\n", mesh->num_vertices, input_file);
    
    List 
        bad_faces,
        boundary_edges,
        removed_halfedges_1, // Front and back buffers for 
        removed_halfedges_2, // removed half-edges
        new_halfedges
    ;
    
    initList(&bad_faces);
    initList(&boundary_edges);
    initList(&removed_halfedges_1);
    initList(&removed_halfedges_2);
    initList(&new_halfedges);
    
    // Hilbert curve sorting of points
    int depth = 12;
    HilbertPoint* hilbert_indices = (HilbertPoint*)malloc(mesh->num_vertices * sizeof(HilbertPoint));
    for (int i = 0; i < mesh->num_vertices; i++) {
        hilbert_indices[i].index = i;
        int* bits = (int*) malloc(depth * sizeof(int));
        HilbertSort(mesh->vertices[i].x, mesh->vertices[i].y, depth, &bits);
        hilbert_indices[i].bits = bits;
    }
    // Sort points based on Hilbert curve bits
    qsort(hilbert_indices, mesh->num_vertices, sizeof(HilbertPoint), compareHilbert);
    
    for (int i = 0; i < mesh->num_vertices; i++) {
        free(hilbert_indices[i].bits);
    }
    
    // Step 2: Insert points into the triangulation
    int walking_face = 0;
    for (int i = 0; i < mesh->num_vertices; i++) {
        insertPoint(mesh, hilbert_indices, &bad_faces, &boundary_edges, &removed_halfedges_1, &removed_halfedges_2, &new_halfedges, i, &walking_face);
    }

    int* first_hes = (int*)malloc(4 * sizeof(int));
    first_hes[0] = 0;
    first_hes[1] = 1;
    first_hes[2] = 4;
    first_hes[3] = 5;
    
    removeInfinitePoints(mesh, first_hes);
    free(first_hes);

#ifdef DEBUG
    testDelaunay(mesh);
    printf("Tested triangulation with %d faces\n", mesh->num_faces);
#endif
    // Output the triangulation to the output file
    FILE* outfile = fopen(output_file, "w");
    if (!outfile) {
        perror("Failed to open output file");
        return -1;
    }
    printMesh(outfile, mesh);
    fclose(outfile);

    // Free allocated memory
#if PYTHON_BINDING
    freeMesh(mesh);
#else
    *(MyMesh**)myMesh = (MyMesh*)mesh;
#endif

    freeList(bad_faces);
    freeList(boundary_edges);
    freeList(removed_halfedges_1);
    freeList(removed_halfedges_2);
    freeList(new_halfedges);

    free(hilbert_indices);

    return 0;
}