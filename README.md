# Computation Geometry - Homework 1

## Contributions
- **Abel:** Implementation of the visualize application in [visualize.c](src/visualize.c) and [visualize.h](src/visualize.h) as well as the Lloyd's relaxation algorithm in [lloyd.c](src/lloyd.c) and [lloyd.h](src/lloyd.h).
- **Nicolas:** Implementation of the Bowyer-Watson algorithm and associated Hilbert sorting and "triangle walking" in [mycode.c](src/mycode.c),
[functions.c](src/functions.c) and [functions.h](src/functions.h).
- **Diego:** Implementation of the infinite point removal function and hull filling function to make it convex in [functions.c](src/functions.c) and [functions.h](src/functions.h).

## Build
To build the project normally, just run:
```
python genpts.py <number_points> pts.dat
make -Bsj12 && python del2d.py -i pts.dat -o triangles.dat
```
This outputs the result of the triangulation of the generated point cloud (inside `pts.dat`) into `triangles.dat`.

## Visualize
You can use visualize in an interactive way by launching with the following command:
```
py genpts.py <number_points> pts.dat && make -Bsj12 visualize
```

This will draw on the screen a visualization of the triangulation of the generated point cloud (found inside `pts.dat`).

You can pan `[MMB]` and zoom `[Scroll]` around, add points `[LMB]` and move them `[LMB + Drag]`. You can also toggle the display of the triangulation with `[D]` and the Voronoi diagram with `[V]`, Pressing `[P]` toggles the preview of the insertion.
