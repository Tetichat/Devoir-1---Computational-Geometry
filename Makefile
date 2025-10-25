INC = -Iclay -Iraylib/include
LIB = -L. -Lraylib/lib -Ltarget -lraylib -lm
TARGET = target/
SRC = src/


# Build shared library
lib: $(SRC)mycode.c $(SRC)functions.c $(SRC)functions.h
	mkdir -p $(TARGET)
	gcc -D PYTHON_BINDING -O3 -shared -o $(TARGET)libmycode.so -fPIC $(SRC)mycode.c $(SRC)functions.c

visualize: $(SRC)visualize.c $(SRC)mycode.c $(SRC)functions.c $(SRC)functions.h
	mkdir -p $(TARGET)
	gcc -D DEBUG -O3 -shared -o $(TARGET)libmycode.so -fPIC $(SRC)mycode.c $(SRC)functions.c
	gcc $(INC) -D DEBUG -o $(TARGET)visualize $(SRC)visualize.c $(LIB) -lmycode
	LD_LIBRARY_PATH=target:raylib/lib ./$(TARGET)visualize pts.dat triangles.dat

# Run valgrind on the test executable
# valgrind: test_delaunay
# 	LD_LIBRARY_PATH=.:raylib/lib valgrind --leak-check=yes ./test_delaunay pts.dat triangles.dat

clean:
	rm -f $(TARGET)libmycode.so $(TARGET)visualize
	rmdir $(TARGET) 2>/dev/null || true
