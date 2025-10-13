
import numpy as np
from matplotlib.collections import LineCollection


def HilbertIndex(x, y, x0, y0, xBlue, yBlue, xRed, yRed, order, bits):
    
    for i in range(order):
        coordRed = (x-x0)*xRed + (y-y0)*yRed
        coordBlue = (x-x0)*xBlue + (y-y0)*yBlue
        xRed/=2; yRed/=2
        xBlue/=2; yBlue/=2
        if (coordBlue >= 0 and coordRed >= 0):
            bits[i] = 2
            x0 += (xRed + xBlue); y0 += (yRed + yBlue)
        elif (coordBlue <= 0 and coordRed >= 0):
            bits[i] = 3
            x0 -= (xBlue-xRed); y0 -= (yBlue-yRed)
            temp = xRed; xRed = xBlue; xBlue = temp
            temp = yRed; yRed = yBlue; yBlue = temp
            xRed = -xRed; xBlue = -xBlue
            yRed = -yRed; yBlue = -yBlue
        elif (coordBlue <= 0 and coordRed <= 0):
            bits[i] = 0
            x0 -= (xRed + xBlue); y0 -= (yRed + yBlue)
            temp = xRed; xRed = xBlue; xBlue = temp
            temp = yRed; yRed = yBlue; yBlue = temp
        elif (coordBlue >= 0 and coordRed <= 0):
            bits[i] = 1
            x0 += (xBlue - xRed); y0 += (yBlue - yRed)



points = np.loadtxt("pts.dat", dtype=np.float32,skiprows=1)
print("Loaded",len(points),"points from pts.dat")

# Hilbert curve sorting of points
Hilbert_order = 12
point_bits = np.empty((len(points),Hilbert_order),dtype=np.int32)

for i,p in enumerate(points) :
    HilbertIndex(p[0],p[1],0,0,0,1,1,0, Hilbert_order, point_bits[i,:])

    # sort points according to Hilbert bits
hilbert_indices = np.zeros(len(points),dtype=np.int32)

for i in range(len(points)):
    index = 0
    for j in range(Hilbert_order):
        index = (index << 2) | point_bits[i,j]
    hilbert_indices[i] = index
sorted_indices = np.argsort(hilbert_indices)
points = points[sorted_indices,:]

print("Points sorted according to Hilbert curve")
print(" Number of points:", len(points))


# Plotting 

import matplotlib.pyplot as plt

fig, ax = plt.subplots()
ax.set_aspect('equal')

# Use a colormap for the line segments while keeping line structure

# Create line segments from sorted points
segments = np.array([points[:-1], points[1:]]).transpose(1,0,2)
# Color by Hilbert sorted index
colors = plt.cm.viridis(np.linspace(0, 1, len(segments)))

lc = LineCollection(segments, colors=colors, linewidths=1)
ax.add_collection(lc)

#sc = ax.scatter(points[:,0], points[:,1], c=np.arange(len(points)), cmap='viridis', s=5)
#plt.colorbar(sc, ax=ax, label='Hilbert sorted index')
plt.show()
