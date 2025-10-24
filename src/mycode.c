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

    // Step 2: Insert points into the triangulation

    List 
        bad_faces,
        boundary_edges,
        removed_halfedges_1,
        removed_halfedges_2,
        new_halfedges
    ;

    initList(&bad_faces);
    initList(&boundary_edges);
    initList(&removed_halfedges_1);
    initList(&removed_halfedges_2);
    initList(&new_halfedges);

    // Walking face index
    int walking_face = 0;

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

    for (int i = 0; i < mesh->num_vertices; i++) {

        Vertex p = mesh->vertices[hilbert_indices[i].index];

        // Reset temporary storage
        boundary_edges.count = 0;
        removed_halfedges_2.count = 0;
        new_halfedges.count = 0;

        // 1. Find all triangles whose circumcircle contains the point p
        bad_faces.count = 0;
        // Start from the last walking face and get one bad face
        if(getBadFace(mesh, &bad_faces, p, &walking_face) != 0) {
            // Point is outside the triangulation, skip it
            continue;
        }

        // If found, get neighbors of bad faces and to bad_faces list if point is inside circumcircle
        getNeighbours(mesh, &bad_faces, p, walking_face);

        // 2. Find the boundary of the polygonal hole
        for (int j = 0; j < bad_faces.count; j++) {
        int face_index = bad_faces.data[j];
        int he = mesh->faces[face_index].halfedge;
        for (int k = 0; k < 3; k++) {
            HalfEdge* edge = &mesh->halfedges[he];
            HalfEdge* twin_edge = (edge->twin != -1) ? &mesh->halfedges[edge->twin] : NULL;
            int twin_face_index = (twin_edge) ? twin_edge->face : -1;

            // If the twin face is not in bad_faces, this edge is a boundary edge
            if (twin_edge == NULL){
                addToList(&boundary_edges, he);
            }
            else{
                bool is_twin_bad = false;
                for (int m = 0; m < bad_faces.count; m++) {
                    if (twin_face_index == bad_faces.data[m]) {
                        is_twin_bad = true;
                        break;
                    }
                }
                if (!is_twin_bad) {
                    addToList(&boundary_edges, he);
                }
                else{
                    // Mark twin half-edge for removal
                    addToList(&removed_halfedges_2, edge->twin);
                }
            }
            he = edge->next;
        }
    }

        // 4. Re-triangulate the polygonal hole with new faces connecting to point p -> TODO
        // Another way to do this by walking around p and making twins as we go
        // Use removed_halfedges_1 and removed_halfedge_count_1 for this step

        for (int j = 0; j < boundary_edges.count; j++) {
            int he = boundary_edges.data[j];
            HalfEdge* edge = &mesh->halfedges[he];
            HalfEdge* twin_edge = (edge->twin != -1) ? &mesh->halfedges[edge->twin] : NULL;

            // Create new half-edges
            HalfEdge* he1; HalfEdge* he2;
            if (removed_halfedges_1.count > 1){
                he1 = &mesh->halfedges[removed_halfedges_1.data[removed_halfedges_1.count - 1]];
                he2 = &mesh->halfedges[removed_halfedges_1.data[removed_halfedges_1.count - 2]];

                // Set vertices
                he2->vertex = hilbert_indices[i].index;
                he1->vertex = mesh->halfedges[edge->next].vertex;

                edge->next = removed_halfedges_1.data[removed_halfedges_1.count - 1]; // he1
                he1->next = removed_halfedges_1.data[removed_halfedges_1.count - 2]; // he2
                he2->next = he; // Close the triangle

                addToList(&new_halfedges, removed_halfedges_1.data[removed_halfedges_1.count - 1]);
                addToList(&new_halfedges, removed_halfedges_1.data[removed_halfedges_1.count - 2]);

                removed_halfedges_1.count -= 2;
            }
            else if (removed_halfedges_1.count == 1){

                he1 = &mesh->halfedges[removed_halfedges_1.data[removed_halfedges_1.count - 1]];

                if(mesh->num_halfedges >= mesh->max_halfedges) {
                    mesh->max_halfedges += 1000;
                    mesh->halfedges = (HalfEdge*)realloc(mesh->halfedges, mesh->max_halfedges * sizeof(HalfEdge));
                }
                he2 = &mesh->halfedges[mesh->num_halfedges++];

                // Set vertices
                he2->vertex = hilbert_indices[i].index;
                he1->vertex = mesh->halfedges[edge->next].vertex;

                edge->next = removed_halfedges_1.data[removed_halfedges_1.count - 1]; // he1
                he1->next = mesh->num_halfedges - 1; // he2
                he2->next = he; // Close the triangle

                addToList(&new_halfedges, removed_halfedges_1.data[removed_halfedges_1.count - 1]);
                addToList(&new_halfedges, mesh->num_halfedges - 1);

                removed_halfedges_1.count--;
            }
            else{
                if (mesh->num_halfedges >= mesh->max_halfedges) {
                    mesh->max_halfedges *= 2;
                    mesh->halfedges = (HalfEdge*) realloc(mesh->halfedges, mesh->max_halfedges * sizeof(HalfEdge));
                }
                he1 = &mesh->halfedges[mesh->num_halfedges++];
                he2 = &mesh->halfedges[mesh->num_halfedges++];

                // Realloc manipulation may have invalidated pointers, so we need to re-fetch the edge pointer
                edge = &mesh->halfedges[he];

                // Set vertices
                he2->vertex = hilbert_indices[i].index;
                he1->vertex = mesh->halfedges[edge->next].vertex;

                edge->next = mesh->num_halfedges - 2; // he1
                he1->next = mesh->num_halfedges - 1; // he2
                he2->next = he; // Close the triangle

                addToList(&new_halfedges, mesh->num_halfedges - 2);
                addToList(&new_halfedges, mesh->num_halfedges - 1);

            }


            // Create new face
            if (bad_faces.count > 0){
                Face* reused_face = &mesh->faces[bad_faces.data[bad_faces.count - 1]];
                reused_face->halfedge = he; // Point to one of the new half-edges
                edge->face = bad_faces.data[bad_faces.count - 1];
                he1->face = bad_faces.data[bad_faces.count - 1];
                he2->face = bad_faces.data[bad_faces.count - 1];
                bad_faces.count--;
            }
            else{
                if (mesh->num_faces >= mesh->max_faces) {
                    mesh->max_faces += 1000;
                    mesh->faces = (Face*)realloc(mesh->faces, mesh->max_faces * sizeof(Face));
                }
                Face* new_face = &mesh->faces[mesh->num_faces++];
                new_face->halfedge = he; // Point to one of the new half-edges
                edge->face = mesh->num_faces - 1;
                he1->face = mesh->num_faces - 1;
                he2->face = mesh->num_faces - 1;
            }
        }

        // 5. Twins

        for (int j = 0; j < boundary_edges.count; j++) {
            int he = boundary_edges.data[j];
            HalfEdge* he1 = &mesh->halfedges[he];
            HalfEdge* he2 = &mesh->halfedges[he1->next]; // New half-edge pointing to new point
            HalfEdge* he3 = &mesh->halfedges[he2->next]; // New half-edge pointing to original vertex

            for (int k = 0; k < new_halfedges.count; k++) {
                HalfEdge* he_nc = &mesh->halfedges[new_halfedges.data[k]];
                HalfEdge* he_nc_next = &mesh->halfedges[he_nc->next];

                if(he1->vertex == he_nc_next->vertex && he2->vertex == he_nc->vertex){
                    he1->twin = new_halfedges.data[k];
                    he_nc->twin = he;
                }
                if(he2->vertex == he_nc_next->vertex && he3->vertex == he_nc->vertex){
                    he2->twin = new_halfedges.data[k];
                    he_nc->twin = he1->next;
                }
                if(he3->vertex == he_nc_next->vertex && he1->vertex == he_nc->vertex){
                    he3->twin = new_halfedges.data[k];
                    he_nc->twin = he2->next;
                }

            }
        }

        // Removed half-edges cleanup
        int temp = removed_halfedges_1.count;
        removed_halfedges_1.count = removed_halfedges_2.count;
        removed_halfedges_2.count = temp;

        int* temp_ptr = removed_halfedges_1.data;
        removed_halfedges_1.data = removed_halfedges_2.data;
        removed_halfedges_2.data = temp_ptr;

        int temp_size = removed_halfedges_1.max;
        removed_halfedges_1.max = removed_halfedges_2.max;
        removed_halfedges_2.max = temp_size;
    }

    // Removal of infinite points - TODO

#if DEBUG
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
    *(MyMesh*)myMesh = *mesh;
#endif

    freeList(bad_faces);
    freeList(boundary_edges);
    freeList(removed_halfedges_1);
    freeList(removed_halfedges_2);
    freeList(new_halfedges);

    free(hilbert_indices);

    return 0;

}
