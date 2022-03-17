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

```shell
export LDFLAGS = -lGLEW -lglfw -lvulkan -lpthread -ldl
g++ main.cpp $(LDFLAGS) -Wall -std=c++20 -o main
```

### On MacOS you could probably use LLVM

```Makefile
LLVM_PATH=/usr/local/opt/llvm
LLVM_VERSION=11.0.0
PATH=$(LLVM_PATH):$PATH
SDKROOT=$(xcrun --sdk macosx --show-sdk-path)
LD_LIBRARY_PATH=$(LLVM_PATH)/lib/:$LD_LIBRARY_PATH
DYLD_LIBRARY_PATH=$(LLVM_PATH)/lib/:$DYLD_LIBRARY_PATH
CPATH=$(LLVM_PATH)/lib/clang/$LLVM_VERSION/include/
LDFLAGS=-L$(LLVM_PATH)/lib -Wl,-rpath,$(LLVM_PATH)/lib
CPPFLAGS=-I$(LLVM_PATH)/include
CC=$(LLVM_PATH)/bin/clang
CXX=$(LLVM_PATH)/bin/clang++
```
