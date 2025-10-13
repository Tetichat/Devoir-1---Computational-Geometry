import sys
import math
#import gmsh
import numpy as np

class TriangularMesh:

    def __init__(self, vertices, triangles):
        self.vertices = []
        j = 0
        for v in vertices :
            self.vertices.append(Vertex(v[0],v[1],v[2],j,None))
            j = j+1

        self.faces = []
        self.halfedges = []
        edges = {}
        j = 0
        index = 0
        for t in triangles :
            indices = [index, index+1, index + 2]
            index = index + 3
            for i in range (3) :
                self.vertices[t[i]].halfedge = indices[i]
                self.halfedges.append(Halfedge(indices[(i+1)%3], None, indices[(i+2)%3],
                                                t[i], j, indices[i]))
                edges[(t[i], t[(i+1)%3])] = indices [i]
            self.faces.append(Face(j,indices[0]))
            j = j+1
        for e,ind1 in edges.items() :
            if (e[1],e[0]) in edges :
                ind2 = edges[(e[1],e[0])]
                self.halfedges[ind1].opposite = ind2
                self.halfedges[ind2].opposite = ind1

        for v in self.vertices :
            v.halfedge = None
        for f in self.faces :
            f.halfedge = None
        for h in self.halfedges :
            h.opposite = None

class Vertex:

    def __init__(self, x=0, y=0):
        self.x = x
        self.y = y
    
    def __str__(self):
        return f"({self.x}, {self.y})"

class Halfedge:

    def __init__(self, next=None, opposite=None, vertex=None,face=None):
        self.opposite = opposite
        self.next = next
        self.vertex = vertex
        self.face = face

    def __str__(self):
        return f"Halfedge(next={self.next}, opposite={self.opposite}, vertex={self.vertex}, face={self.face})"

class Face:

    def __init__(self, halfedge=None):
        # halfedge going ccw around this facet.
        self.halfedge = halfedge

