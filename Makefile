# Build shared library
libmycode.so: mycode.c functions.c functions.h
	gcc -O3 -shared -o libmycode.so -fPIC mycode.c functions.c

# Build test executable
test_delaunay: test_main.c libmycode.so
	gcc -o test_delaunay test_main.c -L. -lmycode

# Default target
run: libmycode.so

# Run valgrind on the test executable
valgrind: test_delaunay
	LD_LIBRARY_PATH=. valgrind --leak-check=yes ./test_delaunay pts.dat triangles.dat

clean:
	rm -f libmycode.so test_delaunay