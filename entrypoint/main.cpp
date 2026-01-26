#include <GLFW/glfw3.h>

import faelight.log;
import faelight.signal;


int main() {

    FL::Log::Config::setLogLevel(FL::Log::eLogLevel::DEBUG);
    FL::Log::info("Hello! Can I have your name? <3");

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Faelight", nullptr, nullptr);

    if (!window) {
        FL::Log::err("Failed to create window");
        return 0;
    }

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        FL::Log::info("Pressed: {}", glfwGetKey(window, GLFW_KEY_LEFT));
    }

    return 0;
}