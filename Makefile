INC = -Iclay -Iraylib/include
LIB = -L. -Lraylib/lib -lraylib -lm


# Build shared library
libmycode.so: mycode.c functions.c functions.h
	gcc -D PYTHON_BINDING -O3 -shared -o libmycode.so -fPIC mycode.c functions.c

visualize: visualize.c mycode.c functions.c functions.h
	gcc -O3 -shared -o libmycode.so -fPIC mycode.c functions.c
	gcc $(INC) -o visualize visualize.c $(LIB) -lmycode
	LD_LIBRARY_PATH=.:raylib/lib ./visualize pts.dat triangles.dat

# Run valgrind on the test executable
# valgrind: test_delaunay
# 	LD_LIBRARY_PATH=.:raylib/lib valgrind --leak-check=yes ./test_delaunay pts.dat triangles.dat

clean:
	rm -f libmycode.so visualize
