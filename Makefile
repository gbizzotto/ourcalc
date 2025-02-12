
debug:
	g++-10 -Wall -Wextra --std=c++2a -g -o main main.cpp -lSDL2 -lSDL2_ttf -lSDL2_image

test: test.cpp util.hpp
	g++-10 -Wall -Wextra --std=c++2a -g -o test test.cpp -lSDL2 -lSDL2_ttf -lSDL2_image
