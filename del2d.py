import sys
import ctypes
import os
import argparse
import numpy as np
import matplotlib.pyplot as plt
import HalfEdge as he
import time


if __name__ == '__main__':

    time_start = time.time()

    # python3 del2d.py -i points.dat -o triangles.dat
    parser = argparse.ArgumentParser(description='Call a C function from Python')
    parser.add_argument('-i', '--input', type=str, required=True, help='Input file')
    parser.add_argument('-o', '--output', type=str, required=True, help='Output file')
    args = parser.parse_args()

    # Load the shared library
    lib = ctypes.CDLL(os.path.abspath("libmycode.so")) # "mycode.dll" on Windows

    # Call the C function
    # Tell ctypes the function signature
    lib.Cdelaunay.argtypes = [
        input := ctypes.c_char_p,
        output := ctypes.c_char_p
    ]
    lib.Cdelaunay.restype = ctypes.c_int

    # Call the C function
    result = lib.Cdelaunay(args.input.encode('utf-8'),args.output.encode('utf-8'))

    time_end = time.time()
    time = time_end - time_start
    print(f"Time to compute triangulation: {time} seconds")

    # Read points from input file
    points = np.loadtxt(args.input,skiprows=1)

    # Read triangles from output file
    triangles = np.loadtxt(args.output,skiprows=1,dtype=int)

    xmin = np.min(points[:,0])
    xmax = np.max(points[:,0])
    ymin = np.min(points[:,1])
    ymax = np.max(points[:,1])
    L = .05

    # Plot the points and triangles
    fig, ax = plt.subplots()
    ax.scatter(points[:,0], points[:,1], s=1,c='b')
    for tri in triangles :
        for j in range(3) :
            v1 = tri[j]
            v2 = tri[(j+1)%3]
            x = [-1,-1]
            y = [-1,-1]

            if v1 == len(points):
                x[0] = xmin - L
                y[0] = ymin - L
            elif v1 == len(points)+1:
                x[0] = xmax + L
                y[0] = ymin - L
            elif v1 == len(points)+2:
                x[0] = xmax + L
                y[0] = ymax + L
            elif v1 == len(points)+3:
                x[0] = xmin - L
                y[0] = ymax + L
            
            else:
                x[0] = points[v1,0]
                y[0] = points[v1,1]
            
            if v2 == len(points):
                x[1] = xmin - L
                y[1] = ymin - L
            elif v2 == len(points)+1:
                x[1] = xmax + L
                y[1] = ymin - L
            elif v2 == len(points)+2:
                x[1] = xmax + L
                y[1] = ymax + L
            elif v2 == len(points)+3:
                x[1] = xmin - L
                y[1] = ymax + L
            else:
                x[1] = points[v2,0]
                y[1] = points[v2,1]
            
            ax.plot(x, y, 'r-',linewidth=0.5)


    ax.set_aspect('equal')
    plt.show()