# Vulkan Exploration

## Objective

Learn about the primitives of programming using the
[Khronos Vulkan API](https://www.vulkan.org/).

## Install dependencies

### Macos

- `brew install glew`
- `brew install glfw`

## How to use the libraries

### In our source code

```cpp
#include "glfw/glfw3.h"
```

This is imported automatically from `/usr/local/include` in `'nix` systems

### How to compile

`g++ main.cpp -lGLEW -lglfw -Wall -std=c++20 -o main`
