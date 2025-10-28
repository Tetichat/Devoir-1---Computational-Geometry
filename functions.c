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
    double pos[8] = {
        a.x, a.y,
        b.x, b.y,
        c.x, c.y,
        d.x, d.y
    };
    return incircle(&pos[0], &pos[2], &pos[4], &pos[6]);
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

int getBadFace(MyMesh* mesh, int** bad_faces, int* bad_face_count, int* max_bad_faces, Vertex p,int* actual_face) {

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
            addToList(bad_faces, bad_face_count, max_bad_faces, *actual_face);
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

void getNeighbours(MyMesh* mesh, int** bad_faces, int* bad_face_count, int* max_bad_faces, Vertex p,int actual_face) {

    // Check all half-edges of the actual_face
    int he = mesh->faces[actual_face].halfedge;
    for (int i = 0; i < 3; i++) {
        HalfEdge* edge = &mesh->halfedges[he];
        HalfEdge* twin_edge = (edge->twin != -1) ? &mesh->halfedges[edge->twin] : NULL;
        int twin_face_index = (twin_edge) ? twin_edge->face : -1;

        if (twin_edge != NULL) {
            // Check if this twin face is already in bad_faces
            bool is_twin_bad = false;
            for (int m = 0; m < *bad_face_count; m++) {
                if (twin_face_index == (*bad_faces)[m]) {
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
                    addToList(bad_faces, bad_face_count, max_bad_faces, twin_face_index);
                    // Recursively check neighbors of this twin face
                    getNeighbours(mesh, bad_faces, bad_face_count, max_bad_faces, p, twin_face_index);
                }
            }
        }
        he = edge->next;
    }

}

void findBoundary(MyMesh* mesh, int* bad_faces, int bad_face_count, int** boundary_edges, int* boundary_edge_count, int* max_boundary_edges, int** removed_halfedges_2, int* removed_halfedge_count_2, int* max_removed_halfedges_2) {

    for (int j = 0; j < bad_face_count; j++) {
        int face_index = bad_faces[j];
        int he = mesh->faces[face_index].halfedge;
        for (int k = 0; k < 3; k++) {
            HalfEdge* edge = &mesh->halfedges[he];
            HalfEdge* twin_edge = (edge->twin != -1) ? &mesh->halfedges[edge->twin] : NULL;
            int twin_face_index = (twin_edge) ? twin_edge->face : -1;

            // If the twin face is not in bad_faces, this edge is a boundary edge
            if (twin_edge == NULL){
                addToList(boundary_edges, boundary_edge_count, max_boundary_edges, he);
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
                    addToList(boundary_edges, boundary_edge_count, max_boundary_edges, he);
                }
                else{
                    // Mark twin half-edge for removal
                    addToList(removed_halfedges_2, removed_halfedge_count_2, max_removed_halfedges_2, edge->twin);
                }
            }
            he = edge->next;
        }
    }
}




int* cleanupMesh(MyMesh* mesh, int* inf_bound, int len_bound){
    int* he_index_map = malloc(mesh->num_halfedges * sizeof(int));
    int* f_index_map = malloc(mesh->num_faces * sizeof(int));

    // --- compactage des faces ---
    int new_fcount = 0;
    for (int i = 0; i < mesh->num_faces; i++) {
        if (mesh->faces[i].mark == 0) {
            if (new_fcount != i)
                mesh->faces[new_fcount] = mesh->faces[i];
            f_index_map[i] = new_fcount++;
        } else {
            f_index_map[i] = -1;
        }
    }

    // --- compactage des halfedges ---
    int new_hecount = 0;
    for (int i = 0; i < mesh->num_halfedges; i++) {
        HalfEdge* he = &mesh->halfedges[i];
        if (he->mark == 0) {
            if (new_hecount != i)
                mesh->halfedges[new_hecount] = *he;
            he_index_map[i] = new_hecount++;
        } else {
            he_index_map[i] = -1;
        }
    }

    // --- correction des indices ---
    for (int i = 0; i < new_hecount; i++) {
        HalfEdge* he = &mesh->halfedges[i];
        he->next = (he->next >= 0 && he->next < mesh->num_halfedges)
                       ? he_index_map[he->next] : -1;
        he->twin = (he->twin >= 0 && he->twin < mesh->num_halfedges)
                       ? he_index_map[he->twin] : -1;
        he->face = (he->face >= 0 && he->face < mesh->num_faces)
                       ? f_index_map[he->face] : -1;
    }

    for (int i = 0; i < new_fcount; i++) {
        Face* f = &mesh->faces[i];
        if (f->halfedge >= 0 && f->halfedge < mesh->num_halfedges)
            f->halfedge = he_index_map[f->halfedge];
        else
            f->halfedge = -1;
    }

    // --- correction de la frontière ---
    for (int i = 0; i < len_bound; i++) {
        inf_bound[i] = he_index_map[inf_bound[i]];
    }

    mesh->num_halfedges = new_hecount;
    mesh->num_faces = new_fcount;
    //mesh->num_vertices -= 4; // ne tenait deja pas compte de ceux la initialement 

    free(he_index_map);
    free(f_index_map);
    return inf_bound;
}



void removeInfinitePoints(MyMesh* mesh, int* first_hes) {
    int num_first_hes = 4;      
    int* prebound = malloc(mesh->num_halfedges * sizeof(int));
    int len_bound = 0;

    //on initialise le maqrqueur mark de tous nos halfedges et faces
    
    for (int i=0; i<mesh->num_faces;i++){
        mesh->faces[i].mark =0 ;
    }
    for (int i = 0; i<mesh->num_halfedges;i++){            
        mesh->halfedges[i].mark = 1;  
    }

    for (int i=0; i<mesh->num_faces;i++){
        int he = mesh->faces[i].halfedge;
        for (int j=0;j<3;j++){
            mesh->halfedges[he].mark =0 ;
            he = mesh->halfedges[he].next;
        }
    }
    // on initialise aussi prebound

    for (int i = 0; i < mesh->num_halfedges; i++) {
        prebound[i] = -1;
    }
    //maintenant que tout est initialisé, on doit marquer ce qu'il faut supprimer.
    
    for (int i = 0; i < num_first_hes; i++) {
        int he = first_hes[i];
        int f = mesh->halfedges[he].face;

        mesh->halfedges[he].mark = 1;
        if (f != -1) {
            mesh->faces[f].mark = 1;
        }
        
        int following = mesh->halfedges[mesh->halfedges[he].next].twin;
        HalfEdge* main_he = &mesh->halfedges[following];

        while (main_he->next != first_hes[(i + 1) % num_first_hes]) {
            
            prebound[mesh->halfedges[mesh->halfedges[main_he->next].next].twin] = len_bound;
            len_bound++; 
            
            //on marque le he frontiere, et les deux coté de l'arrete qu'on est entrain de supprimer
            mesh->halfedges[following].mark = 1;
            mesh->halfedges[main_he->twin].mark = 1;
            mesh->halfedges[mesh->halfedges[main_he->next].next].mark = 1;
            int face_index = main_he->face;
            if (face_index != -1) {
                mesh->faces[face_index].mark = 1;
            }

            following = mesh->halfedges[main_he->next].twin;
            main_he = &mesh->halfedges[following];
        }
        

        mesh->halfedges[following].mark = 1;
        mesh->halfedges[main_he->twin].mark = 1;
            
    }

    int* inf_bound = malloc(len_bound * sizeof(int));
    for(int i = 0; i<mesh->num_halfedges;i++){
        if (prebound[i] != -1){
            inf_bound[prebound[i]] = i;
        }
    }

    int* new_inf_bound = cleanupMesh(mesh, inf_bound, len_bound);
    

    // --- construction de l'enveloppe convexe ---
    int hullsize;
    int* hull = convex_bound_points(mesh->vertices, mesh->num_vertices, &hullsize);
    fill_convex_hull(mesh, hull, hullsize, new_inf_bound, len_bound);
    
    
    // --- free ---
    free(prebound);
    free(new_inf_bound);
    free(hull);

}


void meshtofile(MyMesh* mesh, const char* output_file, 
                int* highlight_halfedges, int num_highlight) 
{
    FILE* outfile = fopen(output_file, "w");
    if (!outfile) {
        perror("Failed to open output file");
        return;
    }

    fprintf(outfile, "# ============================================\n");
    fprintf(outfile, "# Mesh export\n");
    fprintf(outfile, "# Vertices, HalfEdges, Faces\n");
    fprintf(outfile, "# ============================================\n\n");

    // ============================
    // 1️⃣  Vertices section
    // ============================
    fprintf(outfile, "# Vertices (index, x, y)\n");
    fprintf(outfile, "VERTICES %d\n", mesh->num_vertices);
    for (int i = 0; i < mesh->num_vertices; i++) {
        Vertex* v = &mesh->vertices[i];
        fprintf(outfile, "%d %.10f %.10f\n", i, v->x, v->y);
    }
    fprintf(outfile, "\n");

    // ============================
    // 2️⃣  HalfEdges section
    // ============================
    fprintf(outfile, "# HalfEdges (index, vertex, next, twin, face)\n");
    fprintf(outfile, "HALFEDGES %d\n", mesh->num_halfedges);
    for (int i = 0; i < mesh->num_halfedges; i++) {
        HalfEdge* he = &mesh->halfedges[i];
        fprintf(outfile, "%d %d %d %d %d\n",
                i, he->vertex, he->next, he->twin, he->face);
    }
    fprintf(outfile, "\n");

    // ============================
    // 3️⃣  Faces section
    // ============================
    fprintf(outfile, "# Faces (index, halfedge)\n");
    fprintf(outfile, "FACES %d\n", mesh->num_faces);
    for (int i = 0; i < mesh->num_faces; i++) {
        Face* f = &mesh->faces[i];
        fprintf(outfile, "%d %d\n", i, f->halfedge);
    }
    fprintf(outfile, "\n");

    // ============================
    // 4️⃣  Triangles section (for plotting)
    // ============================
    fprintf(outfile, "# Triangles (face index, 3 vertex indices)\n");
    fprintf(outfile, "TRIANGLES %d\n", mesh->num_faces);
    for (int i = 0; i < mesh->num_faces; i++) {
        int he1 = mesh->faces[i].halfedge;
        if (he1 < 0 || he1 >= mesh->num_halfedges) continue;

        int he2 = mesh->halfedges[he1].next;
        int he3 = mesh->halfedges[he2].next;
        if (he2 < 0 || he3 < 0 ||
            he2 >= mesh->num_halfedges || he3 >= mesh->num_halfedges)
            continue;

        int v1 = mesh->halfedges[he1].vertex;
        int v2 = mesh->halfedges[he2].vertex;
        int v3 = mesh->halfedges[he3].vertex;

        fprintf(outfile, "%d %d %d %d\n", i, v1, v2, v3);
    }
    fprintf(outfile, "\n");

    // ============================
    // 5️⃣  Highlighted HalfEdges
    // ============================
    if (highlight_halfedges != NULL && num_highlight > 0) {
        fprintf(outfile, "# Highlighted HalfEdges (indices)\n");
        fprintf(outfile, "HIGHLIGHTED %d\n", num_highlight);
        for (int i = 0; i < num_highlight; i++) {
            fprintf(outfile, "%d\n", highlight_halfedges[i]);
        }
        fprintf(outfile, "\n");
    }

    // ============================
    // Footer
    // ============================
    fprintf(outfile, "# ============================================\n");
    fprintf(outfile, "# End of mesh export\n");
    fprintf(outfile, "# ============================================\n");

    fclose(outfile);
    printf("✅ Mesh successfully written to '%s'\n", output_file);
}

int* convex_bound_points(Vertex* points, int num_vertices, int* hullsize){
    //on cree une copy de points
    
    VertexKey* sorted = malloc(num_vertices * sizeof(VertexKey));
    for (int i = 0; i < num_vertices; i++) {
        sorted[i].index = i;    
        sorted[i].x = points[i].x;
    }
    qsort(sorted, num_vertices, sizeof(VertexKey), compare_vertex_indexed);



    //maintenant, les vertex sont classés selon x.
    


    int* upper = malloc(num_vertices * sizeof(int));
    int up_size = 0;

    for (int i = 0; i < num_vertices; i++) {
        int idx = sorted[i].index;
        while (up_size >= 2) {
            int i1 = upper[up_size - 2];
            int i2 = upper[up_size - 1];

            double a[2] = { points[i1].x, points[i1].y };
            double b[2] = { points[i2].x, points[i2].y };
            double c[2] = { points[idx].x, points[idx].y };

            double det = orient2d(a, b, c);

            // si le dernier point fait un "virage à droite" ou est colinéaire → on l'enlève
            if (det >= 0)
                up_size--;
            else
                break;
        }
        upper[up_size++] = idx;
    }

    int* lower = malloc(num_vertices * sizeof(int));
    int low_size = 0;

    for (int i = num_vertices - 1; i >= 0; i--) {
        int idx = sorted[i].index;
        while (low_size >= 2) {
            int i1 = lower[low_size - 2];
            int i2 = lower[low_size - 1];

            double a[2] = { points[i1].x, points[i1].y };
            double b[2] = { points[i2].x, points[i2].y };
            double c[2] = { points[idx].x, points[idx].y };

            double det = orient2d(a, b, c);
            if (det >= 0)
                low_size--;
            else
                break;
        }
        lower[low_size++] = idx;
    }

    //Concaténer upper + lower (sans doublons) ===
    if (low_size > 1) low_size -= 2;

    int hull_size = up_size + low_size;
    int* hull = malloc(hull_size * sizeof(int));
    int k = 0;
    for (int i = 0; i < up_size; i++) hull[k++] = upper[i];
    for (int i = 1; i < low_size + 1; i++) hull[k++] = lower[i];

    
    // ===Libérations ===
    free(sorted);
    free(upper);
    free(lower);
    //free(hull);
    *hullsize = hull_size;
    return hull;
}


int compare_vertex_indexed(const void* a, const void* b) {
    const VertexKey* va = (const VertexKey*)a;
    const VertexKey* vb = (const VertexKey*)b;
    if (va->x < vb->x) return -1;
    if (va->x > vb->x) return 1;
    return 0;
}


void flip(MyMesh* mesh, int he_idx, int a_idx, int b_idx, int c_idx, int d_idx) {

    HalfEdge* he = &mesh->halfedges[he_idx];

    int uno_idx = he->next;
    HalfEdge* uno = &mesh->halfedges[uno_idx];

    int dos_idx = uno->next;
    HalfEdge* dos = &mesh->halfedges[dos_idx];

    int oppo_idx = he->twin;
    HalfEdge* oppo = &mesh->halfedges[oppo_idx];

    int tres_idx = oppo->next;
    HalfEdge* tres = &mesh->halfedges[tres_idx];

    int cuatro_idx = tres->next;
    HalfEdge* cuatro = &mesh->halfedges[cuatro_idx];

    int f1_idx = he->face;
    Face* f1 = &mesh->faces[f1_idx];

    int f2_idx = oppo->face;
    Face* f2 = &mesh->faces[f2_idx];

    // --- mise à jour du premier triangle ---
    uno->next = he_idx;
    cuatro->next = uno_idx;
    he->next = cuatro_idx;
    uno->face = f1_idx;


    he->vertex = a_idx;       // nouvelle diagonale
    he->face =  f1_idx;
    cuatro->face = f1_idx;
    f1->halfedge = he_idx;

    // --- mise à jour du second triangle ---   

    dos->next = tres_idx;   
    tres->next = oppo_idx;
    oppo->next = dos_idx;
    tres->face = f2_idx;

    oppo->vertex = d_idx;     // nouvelle diagonale
    oppo->face = f2_idx;
    dos->face = f2_idx;
    f2->halfedge = oppo_idx;

}


void fill_convex_hull(MyMesh* mesh, int* hull,int hull_size, int* new_inf_bound, int len_bound){
    int nverts = mesh->num_vertices;
    size_t total_size = len_bound * 2 + nverts * 3;
    int* pool = malloc(total_size * sizeof(int));   

    int* bound_points = pool;
    int* next_points  = bound_points + len_bound;
    int* prev_points  = next_points + nverts;
    int* he_of_point  = prev_points + nverts;
    int* new    = he_of_point + nverts;

    //memset est plus rapide que des boucles C
    memset(next_points, 0xFF, nverts * sizeof(int)); //0xFF c'est axactmenet la meme chose -1, mais ca evite de devoir faire la conversion de -1 vers ça
    memset(prev_points, 0xFF, nverts * sizeof(int));
    memset(he_of_point, 0xFF, nverts * sizeof(int));
    memset(new, 0xFF, len_bound * sizeof(int));

    // construction de bound points a partir de new_inf_bound
    for (int i = 0; i < len_bound; i++) {
        int he_idx = new_inf_bound[i];
        int v = mesh->halfedges[he_idx].vertex;
        bound_points[i] = v;
        he_of_point[v] = he_idx;  //normalement c'est bon
    }

    // construction de next et prev 
    for (int i = 0; i< len_bound; i++){
        int current = bound_points[i];
        int next = bound_points[(i + 1) % len_bound];
        int prev = bound_points[(i - 1 + len_bound) % len_bound];
        next_points[current] = next;
        prev_points[current] = prev;
    }

    //initialisation
    int new_count = 0, delaunay = 0;

    int v1 = hull[0];
    int v2 = next_points[v1];
    int v3 = next_points[v2];
    int v0 = prev_points[v1];

    
    while (v2 != hull[0]){
        
        //initialisation
        //check_mesh_consistency(mesh);
        //printf("v0, v1, v2, v3 = %d, %d, %d, %d\n",v0, v1, v2, v3);


        //forward
        //double det = orient2d(&mesh->vertices[v1].x, &mesh->vertices[v2].x, &mesh->vertices[v3].x);
        //printf("det = %lf\n", det);
        while (orient2d(&mesh->vertices[v1].x, &mesh->vertices[v2].x, &mesh->vertices[v3].x) <0){

            
            //--- on doit creer le triangle (v1,v2,v3), avec les trois halfedge internes associés
            // HALFEDEGE
            int base_he = mesh->num_halfedges;
            
            HalfEdge* he1 = &mesh->halfedges[base_he + 0];
            HalfEdge* he2 = &mesh->halfedges[base_he + 1];
            HalfEdge* he3 = &mesh->halfedges[base_he + 2];
            mesh->num_halfedges +=3 ;

            he1->vertex = v1;
            he2->vertex = v2;
            he3->vertex = v3;

            he1->next = base_he + 2;    // he3
            he2->next = base_he ;       // he1
            he3->next = base_he + 1;    // he2

            he1->twin = -1; //lié a l'exteireur
            he2->twin = he_of_point[v1];
            he3->twin = he_of_point[v2];

            mesh->halfedges[he_of_point[v1]].twin = base_he + 1;
            mesh->halfedges[he_of_point[v2]].twin = base_he + 2;

            // FACE
            int base_f = mesh->num_faces; //s'assurer que c'est le bon
            Face* new_face = &mesh->faces[base_f];
            mesh->num_faces += 1;
            new_face->halfedge = base_he;
            he1->face = base_f;
            he2->face = base_f;
            he3->face = base_f;

            // INCREMENTATION
            
            new[new_count] = base_he;
            new_count++;

            he_of_point[v1] = base_he;

            next_points[v1] = v3;
            prev_points[v3] = v1;

            
            v2 = v3;
            v3 = next_points[v3];

        }

        //backward
        while(orient2d(&mesh->vertices[v0].x, &mesh->vertices[v1].x, &mesh->vertices[v2].x) <0){
            
            //on doit refaire les triangles
            // HALFEDEGS
            int base_he = mesh->num_halfedges;
            HalfEdge* he0 = &mesh->halfedges[base_he + 0];
            HalfEdge* he1 = &mesh->halfedges[base_he + 1];
            HalfEdge* he2 = &mesh->halfedges[base_he + 2];
            mesh->num_halfedges+=3;

            he0->vertex = v0;
            he1->vertex = v1;
            he2->vertex = v2;

            he0->next = base_he + 2;
            he1->next = base_he ;
            he2->next = base_he + 1;

            he0->twin = -1;
            he1->twin = he_of_point[v0];
            he2->twin = he_of_point[v1];
            mesh->halfedges[he_of_point[v0]].twin = base_he + 1;
            mesh->halfedges[he_of_point[v1]].twin = base_he + 2;


            // FACE
            int base_f = mesh->num_faces;
            Face* new_face = &mesh->faces[base_f];
            mesh->num_faces+=1;

            new_face->halfedge = base_he;
            he0->face = base_f;
            he1->face = base_f;
            he2->face = base_f;

            

            // INCREMENTATION

            new[new_count] = base_he;
            new_count++;

            he_of_point[v0] = base_he;

            next_points[v0] = v2;
            prev_points[v2] = v0;

            v1 = v0;
            v0 = prev_points[v0];


        }

        

        //flip
        bool v2_in_hull = false;   //besoin de verifier si v2 est in hull. Si hull etait classé, on pourrait faire ça plus efficacement
        for (int i = 0; i < hull_size; i++) {
            if (v2 == hull[i]) {
                v2_in_hull = true;
                break;
            }
        }


        if (v2_in_hull){
            delaunay = 0;
            while (delaunay == 0){
                delaunay = 1;
                for (int i = 0; i<new_count;i++){
                    int he_idx = new[i];
                    HalfEdge* he = &mesh->halfedges[he_idx];
                    
                    if (he->twin == -1){
                        continue;
                    }
                    int he_prev = mesh->halfedges[he->next].next;
                    int he_next = he->next;
                    int he_opp = he->twin;
                    int he_opp_next = mesh->halfedges[he_opp].next;
                    int he_opp_prev = mesh->halfedges[he_opp_next].next; // ← idem

                    int pa_idx = mesh->halfedges[he_prev].vertex;
                    int pb_idx = he->vertex;
                    int pc_idx = mesh->halfedges[he_next].vertex;
                    int pd_idx = mesh->halfedges[he_opp_prev].vertex;

                    Vertex pa = mesh->vertices[mesh->halfedges[he_prev].vertex];
                    Vertex pb = mesh->vertices[he->vertex];
                    Vertex pc = mesh->vertices[mesh->halfedges[he_next].vertex];
                    Vertex pd = mesh->vertices[mesh->halfedges[he_opp_prev].vertex];

                    if (isInsideCircle(pa, pb, pc, pd) * orient2d(pa.x, pb.x, pc.x) >=  0){
                        flip(mesh, he_idx, pa_idx, pb_idx, pc_idx, pd_idx);
                        delaunay = 0;
                    }
                }
            }
            new_count = 0;
        }


        v0 = v1;
        v1 = v2;
        v2 = v3;
        v3 = next_points[v3];
    }

    //meshtofile(mesh, "mesh_tridebug.txt", listofnew, listofnew_count);
    free(pool);
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
                return 1; // Violation found

            }
        }
    }
    return 0; // No violations found
}
