#include <iostream>

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

using namespace std;

void errorCallback(int code, const char * errorDescription) {
  cout << "ERROR | " << code << " " << errorDescription << endl;
}

int main(int argc, char const *argv[]) {
  cout << endl << "START_OF_PROGRAM" << endl;

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

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

  // some vulkan stuff
  uint32_t extensionCount = 0;
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

  std::cout << extensionCount << " extensions supported\n";

  // some glm stuff
  glm::mat4 matrix;
  glm::vec4 vector;

  auto test = matrix * vector;

  // main loop
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
  }

  glfwTerminate();
  cout << endl << "END_OF_PROGRAM" << endl;
  return 0;
}
