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

typedef struct List {
    int *data;
    int count;
    int max;
} List;

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

    double L;
} MyMesh;

typedef struct HilbertPoint {
    int index; // Original index of the point
    int* bits; // Hilbert curve bits
} HilbertPoint;


int createInitialTriangles(MyMesh* mesh);

double isInsideCircle(Vertex d, Vertex a, Vertex b, Vertex c);

void initList(List *l);
void addToList(List* l, int value);
int getList(List *l, int idx);
void emptyList(List *l);
void freeList(List l);
#define ARR_REALLOC(arr, num, max) do { \
    if (num >= max) { \
        max *= 2; \
        arr = realloc(arr, sizeof(*(arr)) * max); \
    } \
} while (0);

MyMesh* createMesh(char* input_file);
int printMesh(FILE* out, MyMesh *mesh);
void freeMesh(MyMesh* mesh);

void swap(double* a, double* b);
void HilbertSort(double x, double y, int depth, int** bits);
int compareHilbert(const void* a, const void* b);

int getBadFace(MyMesh* mesh, List* bad_faces, Vertex p, int *actual_face);
void getNeighbours(MyMesh* mesh, List* bad_faces, Vertex p, int actual_face);

void findBoundary(MyMesh* mesh, List* bad_faces, List* boundary_edges, List* removed_half_edges);

void insertPoint(
    MyMesh *mesh, 
    HilbertPoint* hilbert_indices,
    List *bad_faces, 
    List *boundary_edges, 
    List *removed_halfedges_1, 
    List *removed_halfedges_2, 
    List *new_halfedges, 
    int vertex_idx
);

int testDelaunay(MyMesh* mesh);

int Cdelaunay(char* input_file, char* output_file, void *myMesh);

#endif // __FUNCTIONS_H__