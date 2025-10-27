#ifndef __LLOYD_H__
#define __LLOYD_H__

#include "functions.h"

Vertex compute_circumcenter(MyMesh *mesh, int face_id);
Vertex compute_voronoi_cell_centroid(MyMesh *mesh, int vertex_id);
void rebuild_triangulation(MyMesh *mesh);

#endif // __LLOYD_H__