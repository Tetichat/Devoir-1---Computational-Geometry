import sys
import ctypes
import os
import argparse
import numpy as np
import matplotlib.pyplot as plt
import HalfEdge as he
import time


def InitialTriangulation(points,L) :
    """
    Constructs an initial bounding quadrilateral and its half-edge data structure for a set of 2D points.
    This function computes the axis-aligned bounding box of the input points and creates a slightly
    larger quadrilateral around it. The vertices and half-edges of this quadrilateral are initialized
    using a half-edge data structure.
    Args:
        points (numpy.ndarray): An (N, 2) array of 2D points.
    Returns:
        tuple: A tuple (vertices, half_edges) where:
            - vertices (list): List of Vertex objects representing the quadrilateral's corners.
            - half_edges (list): List of Halfedge objects representing the edges of the quadrilateral.
    """
    
    min_x = min(points[:,0])
    max_x = max(points[:,0])
    min_y = min(points[:,1])
    max_y = max(points[:,1])

    vertices = []
    half_edges = []
    faces = []

    vertices.append(he.Vertex(min_x - L, min_y - L))
    vertices.append(he.Vertex(max_x + L, min_y - L))
    vertices.append(he.Vertex(max_x + L, max_y + L))
    vertices.append(he.Vertex(min_x - L, max_y + L))

    half_edges.append(he.Halfedge(1,None,0,0))
    half_edges.append(he.Halfedge(2,None,1,0))
    half_edges.append(he.Halfedge(0,3,2,0))
    half_edges.append(he.Halfedge(4,2,0,1))
    half_edges.append(he.Halfedge(5,None,2,1))
    half_edges.append(he.Halfedge(3,None,3,1))

    faces.append(he.Face(0))
    faces.append(he.Face(3))

    return vertices, half_edges, faces

def WriteTriangles(filename, vertices, half_edges, faces) :
    with open(filename, 'w') as f:
        f.write(f"{len(faces)}\n")
        for face in faces :
            h = face.halfedge
            v0 = half_edges[h].vertex
            v1 = half_edges[half_edges[h].next].vertex
            v2 = half_edges[half_edges[half_edges[h].next].next].vertex
            f.write(f"{v0} {v1} {v2}\n")


def PlotMesh(points, vertices, half_edges, faces) :
    fig, ax = plt.subplots()
    ax.scatter(points[:,0], points[:,1], s=1,c='b')
    ax.scatter([v.x for v in vertices], [v.y for v in vertices], s=1,c='g')

    #double = set()
    for f in faces :
        h = f.halfedge
        for _ in range(3) :
            v1 = half_edges[h].vertex
            v2 = half_edges[half_edges[h].next].vertex
            #if (v2,v1) not in double :
            ax.plot([vertices[v1].x, vertices[v2].x], [vertices[v1].y, vertices[v2].y], 'r-',linewidth=0.5)
                #ax.annotate(f"{h}", ((vertices[v1].x + vertices[v2].x)/2, (vertices[v1].y + vertices[v2].y)/2), color='purple')
                #double.add((v1,v2))
            #else:
                #ax.annotate(f"{h}", ((vertices[v1].x + vertices[v2].x)/2, (vertices[v1].y + vertices[v2].y)/2 -0.1), color='purple')
            h = half_edges[h].next

    ax.set_aspect('equal')
    plt.show()


if __name__ == '__main__':

    time_start = time.time()

    # python3 del2d.py -i points.dat -o triangles.dat
    parser = argparse.ArgumentParser(description='Call a C function from Python')
    parser.add_argument('-i', '--input', type=str, required=True, help='Input file')
    parser.add_argument('-o', '--output', type=str, required=True, help='Output file')
    args = parser.parse_args()

    print(f"Input file: {args.input}"
          f"\nOutput file: {args.output}")


    # Read points from input file
    points = np.loadtxt(args.input,skiprows=1)

    L = 0.01
    vertices, half_edges, faces = InitialTriangulation(points,L)

    PlotMesh(points, vertices, half_edges, faces)

    number_destroyed_edges = 0
    destroyed_edges = []
    

    for i,p in enumerate(points) :

        ## 1. Find all triangles whose circumcircle contains the point p

        bad_faces = []
        for k,f in enumerate(faces) :
            h = f.halfedge
            v0 = half_edges[h].vertex
            v1 = half_edges[half_edges[h].next].vertex
            v2 = half_edges[half_edges[half_edges[h].next].next].vertex
            A = np.array([[vertices[v0].x, vertices[v0].y, vertices[v0].x**2 + vertices[v0].y**2, 1],
                          [vertices[v1].x, vertices[v1].y, vertices[v1].x**2 + vertices[v1].y**2, 1],
                          [vertices[v2].x, vertices[v2].y, vertices[v2].x**2 + vertices[v2].y**2, 1],
                          [p[0], p[1], p[0]**2 + p[1]**2, 1]])
            det = np.linalg.det(A)
            if det > 1e-12 :
                bad_faces.append(k)


        ## 2. Find the boundary of the polygonal hole
        boundary_edges = []
        for j, f in enumerate(bad_faces) :
            h = faces[f].halfedge
            for _ in range(3) :
                twin = half_edges[h].opposite
                if twin == None:
                    boundary_edges.append(h)
                elif half_edges[twin].face not in bad_faces :
                    boundary_edges.append(h)
                else:
                    destroyed_edges.append(h)
                h = half_edges[h].next

        ## 4. Re-triangulate the polygonal hole with new faces connecting to point p
        new_vertex_index = len(vertices)
        vertices.append(he.Vertex(p[0],p[1]))

        newly_created_edges = []
        for j, h in enumerate(boundary_edges) : 
            be = half_edges[h]

            # Edges
            if number_destroyed_edges > 0:
                destroyed_he = destroyed_edges.pop(0)
                new_he2_index = destroyed_he
                new_he2 = half_edges[new_he2_index]
                new_he2.next = h
                new_he2.vertex = new_vertex_index
                new_he2.face = None
                new_he2.opposite = None

                new_he1 = he.Halfedge(new_he2_index, None, half_edges[be.next].vertex, None)
                half_edges.append(new_he1)
                new_he1_index = len(half_edges)-1
                be.next = new_he1_index

                number_destroyed_edges -= 1
            else:
                new_he2 = he.Halfedge(h, None, new_vertex_index, None)
                half_edges.append(new_he2)
                new_he2_index = len(half_edges)-1

                new_he1 = he.Halfedge(new_he2_index, None, half_edges[be.next].vertex, None)
                half_edges.append(new_he1)
                new_he1_index = len(half_edges)-1
                be.next = new_he1_index




            newly_created_edges.append(new_he1_index)
            newly_created_edges.append(new_he2_index)

            # Faces
            if len(bad_faces) > 0:
                new_face_index = bad_faces.pop()
                faces[new_face_index].halfedge = h
                be.face = new_face_index
                half_edges[new_he1_index].face = new_face_index
                half_edges[new_he2_index].face = new_face_index
            else:
                new_face_index = len(faces)
                new_face = he.Face(h)
                faces.append(new_face)
                be.face = new_face_index
                half_edges[new_he1_index].face = new_face_index
                half_edges[new_he2_index].face = new_face_index               

        # encore twins - Find twins for ALL edges, not just boundary edges
        for j,h in enumerate(boundary_edges):
            he1 = half_edges[h]
            he2 = half_edges[he1.next]
            he3 = half_edges[he2.next]

            for h_nc in newly_created_edges:
                he_nc = half_edges[h_nc]
                he_nc_next = half_edges[he_nc.next]

                if he1.vertex == he_nc_next.vertex and he_nc.vertex == he2.vertex :
                    he1.opposite = h_nc
                    he_nc.opposite = h
                if he2.vertex == he_nc_next.vertex and he_nc.vertex == he3.vertex :
                    he2.opposite = h_nc
                    he_nc.opposite = he1.next
                if he3.vertex == he_nc_next.vertex and he_nc.vertex == he1.vertex :
                    he3.opposite = h_nc
                    he_nc.opposite = he2.next
                
        number_destroyed_edges = len(destroyed_edges)

    
    # Verification de la triangulation Delaunay
    # for k,p in enumerate(points):
    #     for f in faces :
    #         h = f.halfedge
    #         v0 = half_edges[h].vertex
    #         v1 = half_edges[half_edges[h].next].vertex
    #         v2 = half_edges[half_edges[half_edges[h].next].next].vertex
    #         if v0 == k or v1 == k or v2 == k :
    #             continue
    #         A = np.array([[vertices[v0].x, vertices[v0].y, vertices[v0].x**2 + vertices[v0].y**2, 1],
    #                       [vertices[v1].x, vertices[v1].y, vertices[v1].x**2 + vertices[v1].y**2, 1],
    #                       [vertices[v2].x, vertices[v2].y, vertices[v2].x**2 + vertices[v2].y**2, 1],
    #                       [p[0], p[1], p[0]**2 + p[1]**2, 1]])
    #         det = np.linalg.det(A)
    #         if det > 1e-12 :
    #             print(f"Point {k} at ({p[0]}, {p[1]}) is inside circumcircle of triangle with vertices "
    #                   f"({vertices[v0].x}, {vertices[v0].y}), "
    #                   f"({vertices[v1].x}, {vertices[v1].y}), "
    #                   f"({vertices[v2].x}, {vertices[v2].y}) - Determinant: {det}")
    #             sys.exit(1)
    # print("Delaunay triangulation verified successfully.")

    time_end = time.time()
    time = time_end - time_start
    print(f"Time to compute triangulation: {time} seconds")
    
    PlotMesh(points, vertices, half_edges, faces) 

    WriteTriangles(args.output, vertices, half_edges, faces)
