#include <iostream>

#include "GLFW/glfw3.h"

using namespace std;

int main(int argc, char const *argv[]) {
  cout << endl << "START_OF_PROGRAM" << endl;

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  //

  // initialize GLFW
  if (!glfwInit()) {
    cout << "ERROR_NO_INIT" << endl;
    return -1;
  }

  // create window
  GLFWwindow *window = glfwCreateWindow(
    640, 480, "First GLFW window", nullptr, nullptr
  );
  if (window == nullptr) {
    cout << "ERROR_NO_WINDOW" << endl;
    glfwTerminate();
    return -1;
  }

  // current context
  glfwMakeContextCurrent(window);

  // main loop
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

  }

  glfwTerminate();
  cout << endl << "END_OF_PROGRAM" << endl;
  return 0;
}
