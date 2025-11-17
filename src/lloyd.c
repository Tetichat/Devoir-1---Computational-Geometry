#include <math.h>

#include "lloyd.h"


// Circumcenter of a face of the triangulation
Vertex compute_circumcenter(MyMesh *mesh, int face_id) {
    HalfEdge *he = &mesh->halfedges[mesh->faces[face_id].halfedge];
    
    Vertex v1 = mesh->vertices[he->vertex];
    he = &mesh->halfedges[he->next];
    Vertex v2 = mesh->vertices[he->vertex];
    he = &mesh->halfedges[he->next];
    Vertex v3 = mesh->vertices[he->vertex];

    double x[] = { v1.x, v2.x, v3.x };
    double y[] = { v1.y, v2.y, v3.y }; 

    double 
        d1 = x[1]*x[1] + y[1]*y[1] - x[0]*x[0] - y[0]*y[0],
        d2 = x[2]*x[2] + y[2]*y[2] - x[1]*x[1] - y[1]*y[1]
    ;

    double denom = 2. * ((x[1]-x[0])*(y[2]-y[1]) - (y[1]-y[0])*(x[2]-x[1]));

    double cx = ((y[2]-y[1])*d1 - (y[1]-y[0])*d2) / denom;
    double cy = ((x[1]-x[0])*d2 - (x[2]-x[1])*d1) / denom;

    return (Vertex){ cx, cy };
}

// With the vertex as current centroid of the cell, computes the new one
Vertex compute_voronoi_cell_centroid(MyMesh* mesh, int vertex_index) {
    List incident_faces;
    initList(&incident_faces);
    
    for (int face_id = 0; face_id < mesh->num_faces; face_id++) {
        HalfEdge* he = &mesh->halfedges[mesh->faces[face_id].halfedge];
        for (int j = 0; j < 3; j++) {
            if (he->vertex == vertex_index) {
                addToList(&incident_faces, face_id);
                break;
            }
            he = &mesh->halfedges[he->next];
        }
    }
    
    double centroid_x = 0., centroid_y = 0.;
    double total_area = 0.;
    
    for (int i = 0; i < incident_faces.count; i++) {
        int face_id = incident_faces.data[i];
        
        Vertex circumcenter = compute_circumcenter(mesh, face_id);
        
        double weight = 1.; // Simple equal weighting
        
        centroid_x += circumcenter.x * weight;
        centroid_y += circumcenter.y * weight;
        total_area += weight;
    }
    
    if (total_area > 0) {
        centroid_x /= total_area;
        centroid_y /= total_area;
    }
    
    freeList(incident_faces);
    return (Vertex){ centroid_x, centroid_y };
}

// TODO: We're not forced to rebuild from the beginning, just inserting the new vertex.
void rebuild_triangulation(MyMesh *mesh) {
    // Clear existing triangulation
    mesh->num_faces = 0;
    mesh->num_halfedges = 0;

    createInitialTriangles(mesh);
    
    List bad_faces, boundary_edges, removed_halfedges_1, removed_halfedges_2, new_halfedges;
    initList(&bad_faces);
    initList(&boundary_edges);
    initList(&removed_halfedges_1);
    initList(&removed_halfedges_2);
    initList(&new_halfedges);
    
    int depth = 12;
    HilbertPoint* hilbert_indices = (HilbertPoint*)malloc(mesh->num_vertices * sizeof(HilbertPoint));
    for (int i = 0; i < mesh->num_vertices; i++) {
        hilbert_indices[i].index = i;
        int* bits = (int*) malloc(depth * sizeof(int));
        HilbertSort(mesh->vertices[i].x, mesh->vertices[i].y, depth, &bits);
        hilbert_indices[i].bits = bits;
    }

    qsort(hilbert_indices, mesh->num_vertices, sizeof(HilbertPoint), compareHilbert);
    
    for (int i = 0; i < mesh->num_vertices; i++) {
        free(hilbert_indices[i].bits);
    }

    int walking_face = 0;
    for (int i = 0; i < mesh->num_vertices; i++) {
        insertPoint(mesh, hilbert_indices, &bad_faces, &boundary_edges, 
                   &removed_halfedges_1, &removed_halfedges_2, &new_halfedges, i, &walking_face);
        
        emptyList(&bad_faces);
        emptyList(&boundary_edges);
        emptyList(&new_halfedges);
    }

    int* first_hes = (int*)malloc(4 * sizeof(int));
    first_hes[0] = 0;
    first_hes[1] = 1;
    first_hes[2] = 4;
    first_hes[3] = 5;

    removeInfinitePoints(mesh, first_hes);
    free(first_hes);
    
    freeList(bad_faces);
    freeList(boundary_edges);
    freeList(removed_halfedges_1);
    freeList(removed_halfedges_2);
    freeList(new_halfedges);
}