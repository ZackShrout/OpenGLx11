build:
	g++ main.cpp -o test.a -lGL -lGLU -lX11

run:
	./test.a

clean:
	rm test.a