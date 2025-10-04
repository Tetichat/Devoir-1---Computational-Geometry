import sys
import ctypes
import os
import argparse
import numpy as np
import matplotlib.pyplot as plt
import HalfEdge as he


def InitialTriangulation(points) :
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

    L = 0.2

    vertices = []
    half_edges = []
    faces = []

    vertices.append(he.Vertex(min_x - L, min_y - L))
    vertices.append(he.Vertex(max_x + L, min_y - L))
    vertices.append(he.Vertex(max_x + L, max_y + L))
    vertices.append(he.Vertex(min_x - L, max_y + L))

    half_edges.append(he.Halfedge(1,None,2,0,0))
    half_edges.append(he.Halfedge(2,None,0,1,0))
    half_edges.append(he.Halfedge(0,None,1,2,0))

    half_edges.append(he.Halfedge(4,None,5,0,1))
    half_edges.append(he.Halfedge(5,None,3,2,1))
    half_edges.append(he.Halfedge(3,None,4,3,1))

    half_edges[0].opposite = 3
    half_edges[3].opposite = 0

    faces.append(he.Face(0))
    faces.append(he.Face(3))

    return vertices, half_edges, faces


def PlotMesh(points, vertices, half_edges, faces) :
    fig, ax = plt.subplots()
    ax.scatter(points[:,0], points[:,1], s=1,c='b')
    ax.scatter([v.x for v in vertices], [v.y for v in vertices], s=1,c='g')
    
    for f in faces :
        h = f.halfedge
        for _ in range(3) :
            v1 = half_edges[h].vertex
            v2 = half_edges[half_edges[h].next].vertex
            ax.plot([vertices[v1].x, vertices[v2].x], [vertices[v1].y, vertices[v2].y], 'r-')
            h = half_edges[h].next
        

    ax.set_aspect('equal')
    plt.show()


if __name__ == '__main__':

    # python3 del2d.py -i points.dat -o triangles.dat
    parser = argparse.ArgumentParser(description='Call a C function from Python')
    parser.add_argument('-i', '--input', type=str, required=True, help='Input file')
    parser.add_argument('-o', '--output', type=str, required=True, help='Output file')
    args = parser.parse_args()


    # Load the shared library
    lib = ctypes.CDLL(os.path.abspath("libmycode.so")) # "mycode.dll" on Windows

    # Put points into the HalfEdge data structure
    points = np.loadtxt(args.input,skiprows=1)
    print(f"Loaded {len(points)} points from {args.input}")

    vertices, half_edges, faces = InitialTriangulation(points)

    PlotMesh(points, vertices, half_edges, faces)


    # Bowyer-Watson Algorithm

    for p in points :
        print(f"Insert point ({p[0]},{p[1]})")

        # 1. Find all triangles whose circumcircle contains the point p
        bad_triangles = []

        for i,f in enumerate(faces) :
            h = f.halfedge
            v1 = half_edges[h].vertex
            v2 = half_edges[half_edges[h].next].vertex
            v3 = half_edges[half_edges[half_edges[h].next].next].vertex

            A = np.array([ [vertices[v1].x, vertices[v1].y, vertices[v1].x**2 + vertices[v1].y**2, 1],
                           [vertices[v2].x, vertices[v2].y, vertices[v2].x**2 + vertices[v2].y**2, 1],
                           [vertices[v3].x, vertices[v3].y, vertices[v3].x**2 + vertices[v3].y**2, 1],
                           [p[0], p[1], p[0]**2 + p[1]**2, 1] ])
            det = np.linalg.det(A)
            if det > 0 :
                bad_triangles.append(i)
                print(f"  Triangle {i} is bad")
        
        # 2. Find the boundary of the polygonal hole

        boundary_edges = []

        for bt in bad_triangles :
            h = faces[bt].halfedge
            for _ in range(3) :
                opp = half_edges[h].opposite
                if opp is None or half_edges[opp].facet not in bad_triangles :
                    boundary_edges.append(h)
                h = half_edges[h].next

        # 3. Remove bad triangles
        for bt in sorted(bad_triangles, reverse=True) :
            del faces[bt]
            # Remove associated half-edges
            h = faces[bt].halfedge
            for _ in range(3) :
                half_edges[h] = None
                h = half_edges[h].next
            half_edges = [he for he in half_edges if he is not None]

        # 4. Re-triangulate the polygonal hole
        new_halfedges = []

        for be in boundary_edges :
            v1 = half_edges[be].vertex
            v2 = half_edges[half_edges[be].next].vertex
            v3 = len(vertices)
            vertices.append(he.Vertex(p[0],p[1]))

            he1 = he.Halfedge(None,None,half_edges[be].next,v1)
            he2 = he.Halfedge(None,None,he1.prev,v2)
            he3 = he.Halfedge(None,None,he2.prev,v3)

            he1.next =  len(half_edges) + len(new_halfedges) + 1
            he2.next =  len(half_edges) + len(new_halfedges) + 2
            he3.next =  len(half_edges) + len(new_halfedges) + 0

            new_halfedges.append(he1)
            new_halfedges.append(he2)
            new_halfedges.append(he3)

            faces.append(he.Face(len(half_edges) + len(new_halfedges) - 3))
            half_edges[be].opposite = len(half_edges) + len(new_halfedges) - 3
            new_halfedges[-1].opposite = be
        half_edges.extend(new_halfedges)
        PlotMesh(points, vertices, half_edges, faces)





    


    

    # Call the C function
    # # Tell ctypes the function signature
    # lib.Cdelaunay.argtypes = [ctypes.c_char_p]
    # lib.Cdelaunay.restype = ctypes.c_int

    # # Call the C function
    # x = args.input + " " + args.output
    # x = x.encode('utf-8')  # Convert to bytes
    # result = lib.Cdelaunay(x)
    # print(f"C function returned: {result}")