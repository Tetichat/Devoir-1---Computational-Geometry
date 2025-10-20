INC = -Iclay -Iraylib/include
LIB = -L. -Lraylib/lib -lraylib -lm


# Build shared library
libmycode.so: mycode.c functions.c functions.h
	gcc -O3 -shared -o libmycode.so -fPIC mycode.c functions.c

# Build test executable
test_delaunay: test_main.c mycode.c functions.c functions.h
	gcc $(INC) -D DEBUG -shared -o libmycode.so -fPIC mycode.c functions.c $(LIB)
	gcc $(INC) -D DEBUG -o test_delaunay test_main.c $(LIB) -lmycode
	LD_LIBRARY_PATH=.:raylib/lib ./test_delaunay pts.dat triangles.dat

# Run valgrind on the test executable
valgrind: test_delaunay
	LD_LIBRARY_PATH=.:raylib/lib valgrind --leak-check=yes ./test_delaunay pts.dat triangles.dat

clean:
	rm -f libmycode.so test_delaunay
