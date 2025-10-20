#ifndef __FUNCTIONS_H__
#define __FUNCTIONS_H__

#include <stdio.h>
#include <stdlib.h>
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

typedef struct MyMesh {
    Vertex* vertices;
    HalfEdge* halfedges;
    Face* faces;

    int num_vertices;
    int num_halfedges;
    int num_faces;

    int max_vertices;
    int max_halfedges;
    int max_faces;
} MyMesh;

typedef struct HilbertPoint {
    int index; // Original index of the point
    int* bits; // Hilbert curve bits
} HilbertPoint;


int createInitialTriangles(MyMesh* mesh, double L);
int printMesh(FILE* out, Vertex* points, HalfEdge* halfedges, Face* faces, int num_faces);

double isInsideCircle(Vertex d, Vertex a, Vertex b, Vertex c); // TEMPORAIRE -> meca2170-robustPredicates.c

void addToList(int** list,int*count, int* max_size, int value);

MyMesh* createMesh(char* input_file);
void freeMesh(MyMesh* mesh);

void swap(double* a, double* b);
void HilbertSort(double x, double y, int depth, int** bits);
int compareHilbert(const void* a, const void* b);

int getBadFace(MyMesh* mesh, int** bad_faces, int* bad_face_count, int* max_bad_faces, Vertex p, int *actual_face);
void getNeighbours(MyMesh* mesh, int** bad_faces, int* bad_face_count, int* max_bad_faces, Vertex p, int actual_face);

void findBoundary(MyMesh* mesh, int* bad_faces, int bad_face_count, int** boundary_edges, int* boundary_edge_count, int* max_boundary_edges, int** removed_halfedges_2, int* removed_halfedge_count_2, int* max_removed_halfedges_2);

int testDelaunay(MyMesh* mesh);

int Cdelaunay(char* input_file, char* output_file);


#endif // __FUNCTIONS_H__
