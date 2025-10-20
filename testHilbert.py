import numpy as np
from matplotlib.collections import LineCollection


points = np.loadtxt("pts.dat", dtype=np.float32,skiprows=1)
print("Loaded",len(points),"points from pts.dat")

# Plotting 

hilbert_class = np.loadtxt("debug_hilbert.txt", dtype=np.int32)
print("Loaded",len(hilbert_class),"Hilbert indices from debug_hilbert.txt")

import matplotlib.pyplot as plt

fig, ax = plt.subplots()
ax.set_aspect('equal')

# # Use a colormap for the line segments while keeping line structure

# # Create lines in order of Hilbert curve
segments = []
for i in range(len(points)-1) :
    p1 = points[hilbert_class[i],:]
    p2 = points[hilbert_class[i+1],:]
    segments.append( [p1,p2] )

segments = np.array(segments)

# Color by Hilbert sorted index

lc = LineCollection(segments, cmap='viridis', norm=plt.Normalize(0, len(points)))
lc.set_array(np.arange(len(points)-1))
lc.set_linewidth(2)
line = ax.add_collection(lc)
fig.colorbar(line, ax=ax)

plt.title("Points ordered along Hilbert curve")
plt.show()