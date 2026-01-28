#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

import faelight.log;
import faelight.signal;
import faelight.render.vulkanimpl;

int main() {

    FL::Log::Config::setLogLevel(FL::Log::eLogLevel::DEBUG);
    FL::Log::joke("Hello! Can I have your name? <3");

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Faelight", nullptr, nullptr);

    if (!window) {
        FL::Log::err("Failed to create window!");
        return 1;
    }



    VulkanImpl vulkan = VulkanImpl();
    vulkan.setup(window);

    VkResult err = glfwCreateWindowSurface(vulkan.instance, window, nullptr, &vulkan.get_surface());
    if (err != VK_SUCCESS) {
        FL::Log::err("Failed to create surface!");
        return 1;
    }



    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        //FL::Log::info("Pressed: {}", glfwGetKey(window, GLFW_KEY_LEFT));

    }

    return 0;
}