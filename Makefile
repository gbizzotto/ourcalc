
debug:
	g++-10 -Wall -Wextra --std=c++2a -g -I/usr/include/python3.8 -I../pybind11/include/ -o main main.cpp -lSDL2 -lSDL2_ttf -lSDL2_image -lpython3.8

test: test.cpp util.hpp
	g++-10 -Wall -Wextra --std=c++2a -g -o test test.cpp -lSDL2 -lSDL2_ttf -lSDL2_image
