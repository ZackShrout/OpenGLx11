build:
	g++ main.cpp -o test.a -lGL -lX11

run:
	./test.a

clean:
	rm test.a