#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct Vertex {
    double x, y;
} Vertex;

typedef struct HalfEdge {
    int vertex; // Index of the vertex this half-edge points to
    int next; // Index of the next half-edge in the face
    int twin; // Index of the twin half-edge
    int face; // Index of the face this half-edge borders
} HalfEdge;

typedef struct Face {
    int halfedge; // Index of one of the half-edges bordering the face
} Face;


/**
 * @brief Creates the 2 initial triangles for a Delaunay triangulation.
 *
 * @param points Array of input points.
 * @param num_points Number of points in the array.
 * @param L A large value to ensure the initial triangles encompasses all points.
 * @param halfedges Output parameter to store pointers to the created half-edges.
 * @param faces Output parameter to store pointers to the created face(s).
 * @return 0 on success, or a negative value on failure.
 */
int create_initial_triangle(Vertex** points, int num_points, double L, HalfEdge** halfedges, Face** faces) {


    // Find the bounding box of the points
    double min_x = __DBL_MAX__, min_y = __DBL_MAX__;
    double max_x = -__DBL_MAX__, max_y = -__DBL_MAX__;
    for (int i = 0; i < num_points; i++) {
        if ((*points)[i].x < min_x) min_x = (*points)[i].x;
        if ((*points)[i].x > max_x) max_x = (*points)[i].x;
        if ((*points)[i].y < min_y) min_y = (*points)[i].y;
        if ((*points)[i].y > max_y) max_y = (*points)[i].y;
    }
    // Create the 4 extremity points
    (*points)[num_points].x = min_x - L; (*points)[num_points].y = min_y - L; // Bottom-left
    (*points)[num_points + 1].x = max_x + L; (*points)[num_points + 1].y = min_y - L; // Bottom-right
    (*points)[num_points + 2].x = max_x + L; (*points)[num_points + 2].y = max_y + L; // Top-right
    (*points)[num_points + 3].x = min_x - L; (*points)[num_points + 3].y = max_y + L; // Top-left

    // Create half-edges for the two triangles
    HalfEdge* he1 = &(*halfedges)[0];
    HalfEdge* he2 = &(*halfedges)[1];
    HalfEdge* he3 = &(*halfedges)[2];
    HalfEdge* he4 = &(*halfedges)[3];
    HalfEdge* he5 = &(*halfedges)[4];
    HalfEdge* he6 = &(*halfedges)[5];

    // Triangle 1: (num_points, num_points+1, num_points+2)
    he1->vertex = num_points;    
    he2->vertex = num_points + 1; 
    he3->vertex = num_points + 2;

    he1->next = 1;
    he2->next = 2; 
    he3->next = 0;

    // Triangle 2: (num_points, num_points+2, num_points+3)
    he4->vertex = num_points;
    he5->vertex = num_points + 2;
    he6->vertex = num_points + 3;
    he4->next = 4; he5->next = 5; he6->next = 3;

    // Set twin relationships
    he3->twin = 3; he4->twin = 2; // Shared edge
    he1->twin = -1; he2->twin = -1; // Boundary
    he5->twin = -1; he6->twin = -1; // Boundary

    // Create faces
    Face* f1 = &(*faces)[0];
    Face* f2 = &(*faces)[1];

    f1->halfedge = 0;
    f2->halfedge = 3;

    // Link half-edges to faces
    he1->face = 0; he2->face = 0; he3->face = 0;
    he4->face = 1; he5->face = 1; he6->face = 1;

    return 0; // Success
}

int printMesh(FILE* out, Vertex* points, HalfEdge* halfedges, Face* faces, int num_faces) {
    fprintf(out, "%d\n", num_faces);
    for (int i = 0; i < num_faces; i++) {
        int he1 = faces[i].halfedge;
        int he2 = halfedges[he1].next;
        int he3 = halfedges[he2].next;
        fprintf(out, "%d %d %d \n", halfedges[he1].vertex, halfedges[he2].vertex, halfedges[he3].vertex);
    }
    return 0;
}

double isInsideCircle(Vertex d, Vertex a, Vertex b, Vertex c) {
    double ax = a.x - d.x;
    double ay = a.y - d.y;
    double bx = b.x - d.x;
    double by = b.y - d.y;
    double cx = c.x - d.x;
    double cy = c.y - d.y;

    double det = (ax * ax + ay * ay) * (bx * cy - by * cx) -
                 (bx * bx + by * by) * (ax * cy - ay * cx) +
                 (cx * cx + cy * cy) * (ax * by - ay * bx);
    return det;
}   


void addToList(int** list,int*count, int* max_size, int value){
    if (*count < *max_size) {
        (*list)[*count] = value;
        (*count)++;
    }
    else {
        // Reallocate with increased size
        *max_size += 20;
        *list = (int*)realloc(*list, *max_size * sizeof(int));
        (*list)[*count] = value;
        (*count)++;
    }
}


// A simple function that performs Delaunay triangulation
int Cdelaunay(char* input_file, char* output_file) {

    printf("Input file: %s\n", input_file);
    printf("Output file: %s\n", output_file);

    // Points array read from input file
    Vertex* vertices;
    int num_vertices;
    FILE* infile = fopen(input_file, "r");
    if (!infile) {
        perror("Failed to open input file");
        return -1;
    }
    fscanf(infile, "%d", &num_vertices);
    vertices = (Vertex*)malloc((num_vertices + 4) * sizeof(Vertex)); // Extra space for extremities
    for (int i = 0; i < num_vertices; i++) {
        fscanf(infile, "%lf %lf", &vertices[i].x, &vertices[i].y);
    }
    fclose(infile);

    printf("Read %d points from %s\n", num_vertices, input_file);

    // Half-edge data structure initialization
    int max_halfedges = num_vertices * 10; // Rough estimate
    int max_faces = num_vertices * 2; // Rough estimate
    HalfEdge* halfedges = (HalfEdge*)malloc(max_halfedges * sizeof(HalfEdge));
    Face* faces = (Face*)malloc(max_faces * sizeof(Face));

    // Delaunay triangulation algorithm

    // Step 1: Create initial triangle
    double L = 1; 
    int num_halfedges = 6; // 2 triangles * 3 half-edges each
    int num_faces = 2; // 2 initial triangles
    create_initial_triangle(&vertices, num_vertices, L, &halfedges, &faces);

    // Step 2: Insert points into the triangulation

    // TODO: No dynamic allocation for now, just a fixed size for simplicity

    // Bad faces storage
    int max_bad_faces = 20;
    int* bad_faces = (int*)malloc(max_bad_faces * sizeof(int));
    int bad_face_count = 0;
    // Boundary edges storage
    int max_boundary_edges = 20;
    int* boundary_edges = (int*)malloc(max_boundary_edges * sizeof(int));
    int boundary_edge_count = 0;
    // Removed faces storage
    int max_removed_faces = 20;
    int* removed_faces = (int*)malloc(max_removed_faces * sizeof(int));
    int removed_face_count = 0;
    // Removed half-edges storage
    int max_removed_halfedges_1 = 20;
    int* removed_halfedges_1 = (int*)malloc(max_removed_halfedges_1 * sizeof(int)); // Assume a maximum of 20 removed half-edges
    int removed_halfedge_count_1 = 0;

    int max_removed_halfedges_2 = 20;
    int* removed_halfedges_2 = (int*)malloc(max_removed_halfedges_2 * sizeof(int)); // Assume a maximum of 20 removed half-edges
    int removed_halfedge_count_2 = 0;
    // Newly created edges storage
    int max_new_halfedges = 20;
    int* new_halfedges = (int*)malloc(max_new_halfedges * sizeof(int)); // Assume a maximum of 20 new half-edges
    int new_halfedge_count = 0;


    int debug_value = 0;
    for (int i = 0; i < num_vertices; i++) {

        Vertex p = vertices[i];

        // Reset temporary storage
        bad_face_count = 0;
        boundary_edge_count = 0;
        removed_halfedge_count_2 = 0;
        new_halfedge_count = 0;

        // 1. Find all triangles whose circumcircle contains the point p - TODO  USING walking algorithm and sorted points
        for (int j = 0; j < num_faces; j++) {
            int he = faces[j].halfedge;
            HalfEdge* a = &halfedges[he];
            HalfEdge* b = &halfedges[a->next];
            HalfEdge* c = &halfedges[b->next];

             double det = isInsideCircle(p, vertices[a->vertex], vertices[b->vertex], vertices[c->vertex]);
             if (det > 0) {
                 addToList(&bad_faces, &bad_face_count, &max_bad_faces, j);
             }
        }

        // 2. Find the boundary of the polygonal hole
        for (int j = 0; j < bad_face_count; j++) {
            int face_index = bad_faces[j];
            int he = faces[face_index].halfedge;
            for (int k = 0; k < 3; k++) {
                HalfEdge* edge = &halfedges[he];
                HalfEdge* twin_edge = (edge->twin != -1) ? &halfedges[edge->twin] : NULL;
                int twin_face_index = (twin_edge) ? twin_edge->face : -1;

                // If the twin face is not in bad_faces, this edge is a boundary edge
                if (twin_edge == NULL){
                    addToList(&boundary_edges, &boundary_edge_count, &max_boundary_edges, he);
                }
                else{
                    bool is_twin_bad = false;
                    for (int m = 0; m < bad_face_count; m++) {
                        if (twin_face_index == bad_faces[m]) {
                            is_twin_bad = true;
                            break;
                        }
                    }
                    if (!is_twin_bad) {
                        addToList(&boundary_edges, &boundary_edge_count, &max_boundary_edges, he);
                    }
                    else{
                        // Mark twin half-edge for removal
                        addToList(&removed_halfedges_2, &removed_halfedge_count_2, &max_removed_halfedges_2, edge->twin);
                    }
                }
                he = edge->next;
            }
        }

        // 4. Re-triangulate the polygonal hole with new faces connecting to point p

        for (int j = 0; j < boundary_edge_count; j++) {
            int he = boundary_edges[j];
            HalfEdge* edge = &halfedges[he];
            HalfEdge* twin_edge = (edge->twin != -1) ? &halfedges[edge->twin] : NULL;

            // Create new half-edges
            HalfEdge* he1; HalfEdge* he2;
            if ( removed_halfedge_count_1 > 1){
                he1 = &halfedges[removed_halfedges_1[removed_halfedge_count_1 - 1]];
                he2 = &halfedges[removed_halfedges_1[removed_halfedge_count_1 - 2]];

                // Set vertices
                he2->vertex = i;
                he1->vertex = halfedges[edge->next].vertex;

                edge->next = removed_halfedges_1[removed_halfedge_count_1 - 1]; // he1
                he1->next = removed_halfedges_1[removed_halfedge_count_1 - 2]; // he2
                he2->next = he; // Close the triangle

                addToList(&new_halfedges, &new_halfedge_count, &max_new_halfedges, removed_halfedges_1[removed_halfedge_count_1 - 1]);
                addToList(&new_halfedges, &new_halfedge_count, &max_new_halfedges, removed_halfedges_1[removed_halfedge_count_1 - 2]);

                removed_halfedge_count_1 -= 2;
            }
            else if ( removed_halfedge_count_1 == 1){
                he1 = &halfedges[removed_halfedges_1[removed_halfedge_count_1 - 1]];

                if(num_halfedges >= max_halfedges) {
                    max_halfedges += 1000;
                    halfedges = (HalfEdge*)realloc(halfedges, max_halfedges * sizeof(HalfEdge));
                }
                he2 = &halfedges[num_halfedges++];

                // Set vertices
                he2->vertex = i;
                he1->vertex = halfedges[edge->next].vertex;

                edge->next = removed_halfedges_1[removed_halfedge_count_1 - 1]; // he1
                he1->next = num_halfedges - 1; // he2
                he2->next = he; // Close the triangle

                addToList(&new_halfedges, &new_halfedge_count, &max_new_halfedges, removed_halfedges_1[removed_halfedge_count_1 - 1]);
                addToList(&new_halfedges, &new_halfedge_count, &max_new_halfedges, num_halfedges - 1);

                removed_halfedge_count_1--;
            }
            else{
                if (num_halfedges >= max_halfedges) {
                    max_halfedges += 1000;
                    halfedges = (HalfEdge*)realloc(halfedges, max_halfedges * sizeof(HalfEdge));
                }
                he1 = &halfedges[num_halfedges++];
                he2 = &halfedges[num_halfedges++];

                // Realloc manipulation may have invalidated pointers, so we need to re-fetch the edge pointer
                edge = &halfedges[he];

                // Set vertices
                he2->vertex = i;
                he1->vertex = halfedges[edge->next].vertex;

                edge->next = num_halfedges - 2; // he1
                he1->next = num_halfedges - 1; // he2
                he2->next = he; // Close the triangle
                
                addToList(&new_halfedges, &new_halfedge_count, &max_new_halfedges, num_halfedges - 2);
                addToList(&new_halfedges, &new_halfedge_count, &max_new_halfedges, num_halfedges - 1);
            }

        

            // Create new face
            if (bad_face_count > 0){
                Face* reused_face = &faces[bad_faces[bad_face_count - 1]];
                reused_face->halfedge = he; // Point to one of the new half-edges
                edge->face = bad_faces[bad_face_count - 1];
                he1->face = bad_faces[bad_face_count - 1];
                he2->face = bad_faces[bad_face_count - 1];
                bad_face_count--;
            }
            else{
                if (num_faces >= max_faces) {
                    max_faces += 1000;
                    faces = (Face*)realloc(faces, max_faces * sizeof(Face));
                }
                Face* new_face = &faces[num_faces++];
                new_face->halfedge = he; // Point to one of the new half-edges
                edge->face = num_faces - 1;
                he1->face = num_faces - 1;
                he2->face = num_faces - 1;
            }
        }

        // 5. Twins

        for (int j = 0; j < boundary_edge_count; j++) {
            int he = boundary_edges[j];
            HalfEdge* he1 = &halfedges[he];
            HalfEdge* he2 = &halfedges[he1->next]; // New half-edge pointing to new point
            HalfEdge* he3 = &halfedges[he2->next]; // New half-edge pointing to original vertex

            for (int k = 0; k < new_halfedge_count; k++) {
                HalfEdge* he_nc = &halfedges[new_halfedges[k]];
                HalfEdge* he_nc_next = &halfedges[he_nc->next];

                if(he1->vertex == he_nc_next->vertex && he2->vertex == he_nc->vertex){
                    he1->twin = new_halfedges[k];
                    he_nc->twin = he;
                }
                if(he2->vertex == he_nc_next->vertex && he3->vertex == he_nc->vertex){
                    he2->twin = new_halfedges[k];
                    he_nc->twin = he1->next;
                }
                if(he3->vertex == he_nc_next->vertex && he1->vertex == he_nc->vertex){
                    he3->twin = new_halfedges[k];
                    he_nc->twin = he2->next;
                }

            }
        }

        // Removed half-edges cleanup
        int temp = removed_halfedge_count_1;
        removed_halfedge_count_1 = removed_halfedge_count_2;
        removed_halfedge_count_2 = temp;

        int* temp_ptr = removed_halfedges_1;
        removed_halfedges_1 = removed_halfedges_2;
        removed_halfedges_2 = temp_ptr;

        int temp_size = max_removed_halfedges_1;
        max_removed_halfedges_1 = max_removed_halfedges_2;
        max_removed_halfedges_2 = temp_size;
    }

    // Output the triangulation to the output file
    FILE* outfile = fopen(output_file, "w");
    if (!outfile) {
        perror("Failed to open output file");
        free(vertices);
        free(halfedges);
        free(faces);
        return -1;
    }
    printMesh(outfile, vertices, halfedges, faces, num_faces);
    fclose(outfile);


    // Free allocated memory
    free(vertices);
    free(halfedges);
    free(faces);
    free(bad_faces);
    free(boundary_edges);
    free(removed_faces);
    free(removed_halfedges_1);
    free(removed_halfedges_2);
    free(new_halfedges);
    return 0;

}