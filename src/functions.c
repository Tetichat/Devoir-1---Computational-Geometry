#include "functions.h"
#include "predicates.h"

int createInitialTriangles(MyMesh* mesh) {

    // Initialize predicates.h
    exactinit();

    mesh->num_halfedges = 6; // 2 triangles * 3 half-edges each
    mesh->num_faces = 2; // 2 initial triangles

    // Find the bounding box of the points
    double min_x = __DBL_MAX__, min_y = __DBL_MAX__;
    double max_x = -__DBL_MAX__, max_y = -__DBL_MAX__;
    for (int i = 0; i < mesh->num_vertices; i++) {
        if (mesh->vertices[i].x < min_x) min_x = mesh->vertices[i].x;
        if (mesh->vertices[i].x > max_x) max_x = mesh->vertices[i].x;
        if (mesh->vertices[i].y < min_y) min_y = mesh->vertices[i].y;
        if (mesh->vertices[i].y > max_y) max_y = mesh->vertices[i].y;
    }

    // Create the 4 extremity points
    double L = mesh->L;
    mesh->vertices[mesh->num_vertices].x = min_x - L; mesh->vertices[mesh->num_vertices].y = min_y - L; // Bottom-left
    mesh->vertices[mesh->num_vertices + 1].x = max_x + L; mesh->vertices[mesh->num_vertices + 1].y = min_y - L; // Bottom-right
    mesh->vertices[mesh->num_vertices + 2].x = max_x + L; mesh->vertices[mesh->num_vertices + 2].y = max_y + L; // Top-right
    mesh->vertices[mesh->num_vertices + 3].x = min_x - L; mesh->vertices[mesh->num_vertices + 3].y = max_y + L; // Top-left

    // Triangle 1: (num_points, num_points+1, num_points+2)
    mesh->halfedges[0].vertex = mesh->num_vertices;
    mesh->halfedges[1].vertex = mesh->num_vertices + 1;
    mesh->halfedges[2].vertex = mesh->num_vertices + 2;

    mesh->halfedges[0].next = 1;
    mesh->halfedges[1].next = 2;
    mesh->halfedges[2].next = 0;

    // Triangle 2: (num_points, num_points+2, num_points+3)
    mesh->halfedges[3].vertex = mesh->num_vertices;
    mesh->halfedges[4].vertex = mesh->num_vertices + 2;
    mesh->halfedges[5].vertex = mesh->num_vertices + 3;
    mesh->halfedges[3].next = 4; mesh->halfedges[4].next = 5; mesh->halfedges[5].next = 3;

    // Set twin relationships
    mesh->halfedges[2].twin = 3; mesh->halfedges[3].twin = 2; // Shared edge
    mesh->halfedges[0].twin = -1; mesh->halfedges[1].twin = -1; // Boundary
    mesh->halfedges[4].twin = -1; mesh->halfedges[5].twin = -1; // Boundary

    mesh->faces[0].halfedge = 0;
    mesh->faces[1].halfedge = 3;

    // Link half-edges to faces
    mesh->halfedges[0].face = 0; mesh->halfedges[1].face = 0; mesh->halfedges[2].face = 0;
    mesh->halfedges[3].face = 1; mesh->halfedges[4].face = 1; mesh->halfedges[5].face = 1;

    return 0; // Success
}


int printMesh(FILE* out, MyMesh *mesh) {
    int num_faces = mesh->num_faces;
    fprintf(out, "%d\n", num_faces);
    for (int i = 0; i < num_faces; i++) {
        int he1 = mesh->faces[i].halfedge;
        int he2 = mesh->halfedges[he1].next;
        int he3 = mesh->halfedges[he2].next;
        fprintf(out, "%d %d %d \n", mesh->halfedges[he1].vertex, mesh->halfedges[he2].vertex, mesh->halfedges[he3].vertex);
    }
    return 0;
}

double isInsideCircle(Vertex d, Vertex a, Vertex b, Vertex c) {
    double pos[8] = {
        a.x, a.y,
        b.x, b.y,
        c.x, c.y,
        d.x, d.y
    };
    return incircle(&pos[0], &pos[2], &pos[4], &pos[6]);
}

void initList(List *l) {
    int max = 20;
    int count = 0;
    int *data = (int*)malloc(max * sizeof(int));
    
    *l = (List){
        .data = data,
        .count = count,
        .max = max
    };
}

void addToList(List *l, int value) {
    if (l->count >= l->max) {
        // Reallocate with increased size
        l->max *= 2;
        l->data = (int*)realloc(l->data, l->max * sizeof(int));
    }

    l->data[l->count++] = value;
}

int getList(List *l, int idx) {
    return l->data[idx];
}

void emptyList(List *l) {
    l->count = 0;
}

void freeList(List l) {
    free(l.data);
}

MyMesh* createMesh(char* input_file) {

    MyMesh* mesh = (MyMesh*)malloc(sizeof(MyMesh));
    if (!mesh) {
        perror("Failed to allocate memory for mesh");
        return NULL;
    }

    FILE* infile = fopen(input_file, "r");
    if (!infile) {
        perror("Failed to open input file");
        return NULL;
    }
    int error = fscanf(infile, "%d", &mesh->num_vertices);
    if (error != 1) {
        perror("Failed to read number of vertices");
        fclose(infile);
        free(mesh);
        return NULL;
    }
    mesh->max_vertices = mesh->num_vertices + 4; // Extra space for extremities
    mesh->vertices = (Vertex*)malloc(mesh->max_vertices * sizeof(Vertex));
    for (int i = 0; i < mesh->num_vertices; i++) {
        error = fscanf(infile, "%lf %lf", &mesh->vertices[i].x, &mesh->vertices[i].y);
        if (error != 2) {
            perror("Failed to read vertex coordinates");
            fclose(infile);
            free(mesh->vertices);
            free(mesh);
            return NULL;
        }
    }
    fclose(infile);

    // Half-edge data structure initialization
    mesh->max_halfedges = mesh->num_vertices * 7; // Rough estimate
    mesh->max_faces = mesh->num_vertices * 2; // Rough estimate
    mesh->halfedges = (HalfEdge*)malloc(mesh->max_halfedges * sizeof(HalfEdge));
    mesh->faces = (Face*)malloc(mesh->max_faces * sizeof(Face));
    if (!mesh->halfedges || !mesh->faces) {
        perror("Failed to allocate memory for half-edges or faces");
        free(mesh->vertices);
        free(mesh);
        return NULL;
    }

    mesh->num_halfedges = 0;
    mesh->num_faces = 0;

    return mesh;
}


void freeMesh(MyMesh* mesh) {

    free(mesh->vertices);
    free(mesh->halfedges);
    free(mesh->faces);
    free(mesh);
}

void swap(double* a, double* b){
    double temp = *a;
    *a = *b;
    *b = temp;
}

static int global_depth; // Global variable to hold depth for comparison function
void HilbertSort(double x, double y, int depth, int** bits) {

    double x0 = 0.0, y0 = 0.0;
    double xBlue = 0.0, yBlue = 1.0;
    double xRed = 1.0, yRed = 0.0;

    double coordRed, coordBlue;
    global_depth = depth;

    for (int i = 0; i < depth; i++) {

        coordRed = (x-x0)*xRed + (y-y0)*yRed;
        coordBlue = (x-x0)*xBlue + (y-y0)*yBlue;
        xRed/=2; yRed/=2;
        xBlue/=2; yBlue/=2;
        if (coordBlue >= 0 && coordRed >= 0) {
            (*bits)[i] = 2;
            x0 += (xRed + xBlue); y0 += (yRed + yBlue);
        }
        else if (coordBlue <= 0 && coordRed >= 0){
            (*bits)[i] = 3;
            x0 -= (xBlue-xRed); y0 -= (yBlue-yRed);
            swap(&xRed, &xBlue);
            swap(&yRed, &yBlue);
            xRed = -xRed; xBlue = -xBlue;
            yRed = -yRed; yBlue = -yBlue;
        }
        else if (coordBlue <= 0 && coordRed <= 0) {
            (*bits)[i] = 0;
            x0 -= (xRed + xBlue); y0 -= (yRed + yBlue);
            swap(&xRed, &xBlue);
            swap(&yRed, &yBlue);
        }
        else if (coordBlue >= 0 && coordRed <= 0) {
            (*bits)[i] = 1;
            x0 += (xBlue - xRed); y0 += (yBlue - yRed);
        }
    }
}
int compareHilbert(const void* a, const void* b) {
    const HilbertPoint* pointA = (const HilbertPoint*)a;
    const HilbertPoint* pointB = (const HilbertPoint*)b;

    for (int i = 0;i<global_depth; i++) {
        if (pointA->bits[i] < pointB->bits[i]) return -1;
        if (pointA->bits[i] > pointB->bits[i]) return 1;
        // If bits are equal, continue to next bit
    }
    return 0; // They are equal (should not reach here in practice)
}

int getBadFace(MyMesh* mesh, List* bad_faces, Vertex p, int *actual_face) {

    bool found = false;
    int max_attempts = mesh->num_faces; // Prevent infinite loops
    int attempts = 0;
    while(!found && attempts < max_attempts){
        int he = mesh->faces[*actual_face].halfedge;
        HalfEdge* a = &mesh->halfedges[he];
        HalfEdge* b = &mesh->halfedges[a->next];
        HalfEdge* c = &mesh->halfedges[b->next];

        double det = isInsideCircle(p, mesh->vertices[a->vertex], mesh->vertices[b->vertex], mesh->vertices[c->vertex]);
        if (det > 0) {
            found = true;
            addToList(bad_faces, *actual_face);
        }
        else{
            // Move to closest adjacent face to point p
            // Move to the adjacent face in the direction of point p
            double ax = mesh->vertices[a->vertex].x, ay = mesh->vertices[a->vertex].y;
            double bx = mesh->vertices[b->vertex].x, by = mesh->vertices[b->vertex].y;
            double cx = mesh->vertices[c->vertex].x, cy = mesh->vertices[c->vertex].y;

            double cross1 = (bx - ax) * (p.y - ay) - (by - ay) * (p.x - ax);
            double cross2 = (cx - bx) * (p.y - by) - (cy - by) * (p.x - bx);
            double cross3 = (ax - cx) * (p.y - cy) - (ay - cy) * (p.x - cx);

            int next_face = -1;
            if (cross1 < 0) {
                HalfEdge* twin_edge = (a->twin != -1) ? &mesh->halfedges[a->twin] : NULL;
                if (twin_edge) next_face = twin_edge->face;
            } else if (cross2 < 0) {
                HalfEdge* twin_edge = (b->twin != -1) ? &mesh->halfedges[b->twin] : NULL;
                if (twin_edge) next_face = twin_edge->face;
            } else if (cross3 < 0) {
                HalfEdge* twin_edge = (c->twin != -1) ? &mesh->halfedges[c->twin] : NULL;
                if (twin_edge) next_face = twin_edge->face;
            } else {
                // Point is inside the triangle or on boundary
                break;
            }
            if (next_face == -1) break;
            *actual_face = next_face;

        }
        attempts++;
    }
    return found ? 0 : -1; // Return 0 if found, -1 if not
}

void getNeighbours(MyMesh* mesh, List* bad_faces, Vertex p, int actual_face) {

    // Check all half-edges of the actual_face
    int he = mesh->faces[actual_face].halfedge;
    for (int i = 0; i < 3; i++) {
        HalfEdge* edge = &mesh->halfedges[he];
        HalfEdge* twin_edge = (edge->twin != -1) ? &mesh->halfedges[edge->twin] : NULL;
        int twin_face_index = (twin_edge) ? twin_edge->face : -1;

        if (twin_edge != NULL) {
            // Check if this twin face is already in bad_faces
            bool is_twin_bad = false;
            for (int m = 0; m < bad_faces->count; m++) {
                if (twin_face_index == bad_faces->data[m]) {
                    is_twin_bad = true;
                    break;
                }
            }
            if (!is_twin_bad) {
                // Check if point p is inside the circumcircle of the twin face
                int he1 = mesh->faces[twin_face_index].halfedge;
                int he2 = mesh->halfedges[he1].next;
                int he3 = mesh->halfedges[he2].next;

                double det = isInsideCircle(p, mesh->vertices[mesh->halfedges[he1].vertex],
                                           mesh->vertices[mesh->halfedges[he2].vertex],
                                           mesh->vertices[mesh->halfedges[he3].vertex]);
                if (det > 0) {
                    addToList(bad_faces, twin_face_index);
                    // Recursively check neighbors of this twin face
                    getNeighbours(mesh, bad_faces, p, twin_face_index);
                }
            }
        }
        he = edge->next;
    }

}

void findBoundary(MyMesh* mesh, List* bad_faces, List* boundary_edges, List* removed_halfedges) {

    for (int j = 0; j < bad_faces->count; j++) {
        int face_index = bad_faces->data[j];
        int he = mesh->faces[face_index].halfedge;
        for (int k = 0; k < 3; k++) {
            HalfEdge* edge = &mesh->halfedges[he];
            HalfEdge* twin_edge = (edge->twin != -1) ? &mesh->halfedges[edge->twin] : NULL;
            int twin_face_index = (twin_edge) ? twin_edge->face : -1;

            // If the twin face is not in bad_faces, this edge is a boundary edge
            if (twin_edge == NULL){
                addToList(boundary_edges, he);
            }
            else{
                bool is_twin_bad = false;
                for (int m = 0; m < bad_faces->count; m++) {
                    if (twin_face_index == bad_faces->data[m]) {
                        is_twin_bad = true;
                        break;
                    }
                }
                if (!is_twin_bad) {
                    addToList(boundary_edges, he);
                }
                else{
                    // Mark twin half-edge for removal
                    addToList(removed_halfedges, edge->twin);
                }
            }
            he = edge->next;
        }
    }
}

void insertPoint(
    MyMesh *mesh, 
    HilbertPoint* hilbert_indices,
    List *bad_faces, 
    List *boundary_edges, 
    List *removed_halfedges_1, 
    List *removed_halfedges_2, 
    List *new_halfedges, 
    int vertex_idx
) {
    Vertex p = mesh->vertices[hilbert_indices[vertex_idx].index];
    int walking_face = 0;
        
    // Reset temporary storage
    emptyList(bad_faces);
    emptyList(boundary_edges);
    emptyList(removed_halfedges_2);
    emptyList(new_halfedges);

    // 1. Find all triangles whose circumcircle contains the point p
    // Start from the last walking face and get one bad face
    if(getBadFace(mesh, bad_faces, p, &walking_face) != 0) {
        // Point is outside the triangulation, skip it
        return;
    }

    // If found, get neighbors of bad faces and to bad_faces list if point is inside circumcircle
    getNeighbours(mesh, bad_faces, p, walking_face);

    // 2. Find the boundary of the polygonal hole
    for (int j = 0; j < bad_faces->count; j++) {
        int face_index = bad_faces->data[j];
        int he = mesh->faces[face_index].halfedge;

        for (int k = 0; k < 3; k++) {
            HalfEdge* edge = &mesh->halfedges[he];
            HalfEdge* twin_edge = (edge->twin != -1) ? &mesh->halfedges[edge->twin] : NULL;
            int twin_face_index = (twin_edge) ? twin_edge->face : -1;

            // If the twin face is not in bad_faces, this edge is a boundary edge
            if (twin_edge == NULL){
                addToList(boundary_edges, he);
            }
            else{
                bool is_twin_bad = false;
                for (int m = 0; m < bad_faces->count; m++) {
                    if (twin_face_index == bad_faces->data[m]) {
                        is_twin_bad = true;
                        break;
                    }
                }
                if (!is_twin_bad) {
                    addToList(boundary_edges, he);
                }
                else{
                    // Mark twin half-edge for removal
                    addToList(removed_halfedges_2, edge->twin);
                }
            }
            he = edge->next;
        }
    }

    // 4. Re-triangulate the polygonal hole with new faces connecting to point p -> TODO
    // Another way to do this by walking around p and making twins as we go
    // Use removed_halfedges_1 and removed_halfedge_count_1 for this step

    for (int j = 0; j < boundary_edges->count; j++) {
        int he = boundary_edges->data[j];
        HalfEdge* edge = &mesh->halfedges[he];
        HalfEdge* twin_edge = (edge->twin != -1) ? &mesh->halfedges[edge->twin] : NULL;

        // Create new half-edges
        int he1_idx, he2_idx;
        HalfEdge* he1; HalfEdge* he2;
        int count_rm_he_1 = removed_halfedges_1->count;

        if (count_rm_he_1 == 0) {
            if (mesh->num_halfedges + 2 > mesh->max_halfedges) {
                mesh->max_halfedges *= 2;
                mesh->halfedges = (HalfEdge*) realloc(mesh->halfedges, mesh->max_halfedges * sizeof(HalfEdge));
            }

            edge = &mesh->halfedges[he];
            he1  = &mesh->halfedges[he1_idx = mesh->num_halfedges++];
            he2  = &mesh->halfedges[he2_idx = mesh->num_halfedges++];
        }
        else if (count_rm_he_1 == 1) {
            if(mesh->num_halfedges + 1 > mesh->max_halfedges) {
                mesh->max_halfedges *= 2;
                mesh->halfedges = (HalfEdge*)realloc(mesh->halfedges, mesh->max_halfedges * sizeof(HalfEdge));
            }
            
            he1_idx = getList(removed_halfedges_1, count_rm_he_1 - 1);
            he1 = &mesh->halfedges[he1_idx];

            he2 = &mesh->halfedges[he2_idx = mesh->num_halfedges++];

            removed_halfedges_1->count -= 1;
        }
        else {
            he1_idx = getList(removed_halfedges_1, count_rm_he_1 - 1);
            he2_idx = getList(removed_halfedges_1, count_rm_he_1 - 2);

            he1 = &mesh->halfedges[he1_idx];
            he2 = &mesh->halfedges[he2_idx];

            removed_halfedges_1->count -= 2;
        }

        // Set vertices
        he2->vertex = hilbert_indices[vertex_idx].index;
        he1->vertex = mesh->halfedges[edge->next].vertex;

        edge->next = he1_idx; // he1
        he1->next  = he2_idx; // he2
        he2->next  = he; // Close the triangle

        addToList(new_halfedges, he1_idx);
        addToList(new_halfedges, he2_idx);

        // Create new face
        if (bad_faces->count > 0){
            Face* reused_face = &mesh->faces[bad_faces->data[bad_faces->count - 1]];
            reused_face->halfedge = he; // Point to one of the new half-edges
            edge->face = bad_faces->data[bad_faces->count - 1];
            he1->face = bad_faces->data[bad_faces->count - 1];
            he2->face = bad_faces->data[bad_faces->count - 1];
            bad_faces->count--;
        }
        else{
            if (mesh->num_faces + 1 > mesh->max_faces) {
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
    for (int j = 0; j < boundary_edges->count; j++) {
        int he = boundary_edges->data[j];
        HalfEdge* he1 = &mesh->halfedges[he];
        HalfEdge* he2 = &mesh->halfedges[he1->next]; // New half-edge pointing to new point
        HalfEdge* he3 = &mesh->halfedges[he2->next]; // New half-edge pointing to original vertex

        for (int k = 0; k < new_halfedges->count; k++) {
            HalfEdge* he_nc = &mesh->halfedges[new_halfedges->data[k]];
            HalfEdge* he_nc_next = &mesh->halfedges[he_nc->next];

            if(he1->vertex == he_nc_next->vertex && he2->vertex == he_nc->vertex){
                he1->twin = new_halfedges->data[k];
                he_nc->twin = he;
            }
            if(he2->vertex == he_nc_next->vertex && he3->vertex == he_nc->vertex){
                he2->twin = new_halfedges->data[k];
                he_nc->twin = he1->next;
            }
            if(he3->vertex == he_nc_next->vertex && he1->vertex == he_nc->vertex){
                he3->twin = new_halfedges->data[k];
                he_nc->twin = he2->next;
            }

        }
    }

    // Removed half-edges cleanup
    int temp = removed_halfedges_1->count;
    removed_halfedges_1->count = removed_halfedges_2->count;
    removed_halfedges_2->count = temp;

    int* temp_ptr = removed_halfedges_1->data;
    removed_halfedges_1->data = removed_halfedges_2->data;
    removed_halfedges_2->data = temp_ptr;

    int temp_size = removed_halfedges_1->max;
    removed_halfedges_1->max = removed_halfedges_2->max;
    removed_halfedges_2->max = temp_size;
}

int testDelaunay(MyMesh* mesh) {
    for (int i = 0; i < mesh->num_faces; i++) {
        int he1 = mesh->faces[i].halfedge;
        int he2 = mesh->halfedges[he1].next;
        int he3 = mesh->halfedges[he2].next;

        Vertex a = mesh->vertices[mesh->halfedges[he1].vertex];
        Vertex b = mesh->vertices[mesh->halfedges[he2].vertex];
        Vertex c = mesh->vertices[mesh->halfedges[he3].vertex];

        // Check all other vertices
        for (int j = 0; j < mesh->num_vertices; j++) {
            if (j == mesh->halfedges[he1].vertex || j == mesh->halfedges[he2].vertex || j == mesh->halfedges[he3].vertex) {
                continue; // Skip vertices of the triangle
            }
            Vertex p = mesh->vertices[j];
            double det = isInsideCircle(p, a, b, c);
            if (det > 0) {
                printf("Delaunay violation: Point (%.2f, %.2f) is inside circumcircle of triangle with vertices (%.2f, %.2f), (%.2f, %.2f), (%.2f, %.2f)\n",
                       p.x, p.y, a.x, a.y, b.x, b.y, c.x, c.y);
                return 1;

            }
        }
    }
    return 0; // No violations found
}
