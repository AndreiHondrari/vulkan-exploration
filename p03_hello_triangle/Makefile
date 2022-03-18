
LDFLAGS = -lGLEW -lglfw -lvulkan -lpthread -ldl

main: main.cpp
	$(CXX) main.cpp $(LDFLAGS) -std=c++20 -stdlib=libc++ -Wall -o main.out

shaders:
	./compile_shaders.sh

run:
	./main.out

default: shaders main

.PHONY: run shaders
