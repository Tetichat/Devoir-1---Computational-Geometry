import sys
import ctypes
import os
import argparse
import numpy as np
import matplotlib.pyplot as plt
import time

def PlotTriangles(args,plot_circumcircles=False):

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
        ## Plot the circumcircle
        # if plot_circumcircles and tri[0] < len(points) and tri[1] < len(points) and tri[2] < len(points):
        #     x1 = points[tri[0],0]
        #     y1 = points[tri[0],1]
        #     x2 = points[tri[1],0]
        #     y2 = points[tri[1],1]
        #     x3 = points[tri[2],0]
        #     y3 = points[tri[2],1]

        #     A = np.array([[x2 - x1, y2 - y1],
        #                     [x3 - x1, y3 - y1]])
        #     B = np.array([[(x2**2 - x1**2 + y2**2 - y1**2)/2],
        #                     [(x3**2 - x1**2 + y3**2 - y1**2)/2]])
        #     try:
        #         center = np.linalg.solve(A, B)
        #         xc = center[0,0]
        #         yc = center[1,0]
        #         r = np.sqrt((xc - x1)**2 + (yc - y1)**2)
        #         circle = plt.Circle((xc, yc), r, color='g', fill=False, linestyle='dotted', linewidth=0.5)
        #         ax.add_artist(circle)
        #     except np.linalg.LinAlgError:
        #         pass

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



if __name__ == '__main__':

    time_start = time.time()

    # python3 del2d.py -i points.dat -o triangles.dat
    parser = argparse.ArgumentParser(description='Call a C function from Python')
    parser.add_argument('-i', '--input', type=str, required=True, help='Input file')
    parser.add_argument('-o', '--output', type=str, required=True, help='Output file')
    args = parser.parse_args()

    # Load the shared library
    lib = ctypes.CDLL(os.path.abspath("target/libmycode.so")) # "mycode.dll" on Windows

    # Call the C function
    # Tell ctypes the function signature
    lib.Cdelaunay.argtypes = [
        input := ctypes.c_char_p,
        output := ctypes.c_char_p,
        mesh := ctypes.c_void_p  # Not used by the binding but necessary for
                                 # the visualization
    ]
    lib.Cdelaunay.restype = ctypes.c_int

    # Call the C function
    result = lib.Cdelaunay(args.input.encode('utf-8'), args.output.encode('utf-8'), ctypes.c_void_p())

    time_end = time.time()
    time = time_end - time_start
    print(f"Time to compute triangulation: {time} seconds")

    # Visualize the result in Python
    # PlotTriangles(args,plot_circumcircles=False)
