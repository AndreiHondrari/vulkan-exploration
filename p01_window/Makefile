
LDFLAGS = -lGLEW -lglfw -lpthread -ldl

main: main.cpp
	$(CXX) main.cpp $(LDFLAGS) -std=c++17 -stdlib=libc++ -Wall -o main.out

run:
	./main.out

default: main

.PHONY: run
