run: $(wildcard mycode.c)
	gcc -shared -o libmycode.so -fPIC $^
clean:
	rm libmycode.so