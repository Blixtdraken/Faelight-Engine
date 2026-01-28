module;

#include <VkBootstrap.h>

#include "GLFW/glfw3.h"

import faelight.log;

export module faelight.render.vulkanimpl;



//https://github.com/charles-lunarg/vk-bootstrap/blob/main/docs/getting_started.md


export class VulkanImpl {

public:
    vkb::Instance       instance;
    vkb::PhysicalDevice physical_device;
    vkb::Device         device;

    VkQueue_T*          graphic_queue = {};
    VkSurfaceKHR        surface;

    VulkanImpl(){}

    bool setup(GLFWwindow* _window) {
        if (!createInstance()) FL::Log::err("Failed creating instance!");
        if (!createSurface(instance, _window)) FL::Log::err("Failed creating surface!");
        else if (!pickPhysicalDevice(instance)) FL::Log::err("Failed to pick physical device!");
        else if (!createDevice(physical_device)) FL::Log::err("Failed to create logical device!");
        else if (!createQueueFamilies(device)) FL::Log::err("Failed to create queue families!");
        else {
            FL::Log::debug("Successfully setup Vulkan");
            return true;
        }
        FL::Log::err("Vulkan setup creation failed!");
        return false;
    }

    VkSurfaceKHR& get_surface() {return surface;}


    bool createInstance() {

        auto instance_builder = vkb::InstanceBuilder()
            .set_app_name("Fairy Forest")
            .set_engine_name("Faelight")
            .require_api_version(1,2,0);

        auto system_res = vkb::SystemInfo::get_system_info();

        if (!system_res) {
            FL::Log::err("Failed to get system info in instance creation!");
            return false;
        }
        auto system_info = system_res.value();

        if (system_info.validation_layers_available) {
            instance_builder.enable_validation_layers()
            .use_default_debug_messenger(); // TODO: Set to custom debug messanger for better logs
        }

        auto instance_res = instance_builder.build();

        if (!instance_res) {
            FL::Log::err("Failed to create vulkan instance: {}", instance_res.error().message());
            return false;
        }


        instance = instance_res.value();
        return true;
    }


    bool createSurface(VkInstance _instance, GLFWwindow* _window) {
        VkResult err = glfwCreateWindowSurface(_instance, _window, nullptr, &surface);
        if (err != VK_SUCCESS) {
            return false;
        }
        return true;
    }

    bool pickPhysicalDevice(vkb::Instance _instance) {

        vkb::PhysicalDeviceSelector phys_device_selector(_instance);

        auto physical_device_res = phys_device_selector
        .set_surface(surface)
        .select();

        if (!physical_device_res) {
            FL::Log::err("Gpu pick failed: {}", physical_device_res.error().message());
            const auto& detailed_reasons = physical_device_res.detailed_failure_reasons();
            if (!detailed_reasons.empty()) {
                for (const std::string& reason : detailed_reasons) {
                    FL::Log::err("Gpu pick failed: {}", reason);
                }
            }
            return false;
        }

        physical_device = physical_device_res.value();
        return true;
    }

    bool createDevice(vkb::PhysicalDevice _physical_device) {

        vkb::DeviceBuilder device_builder{_physical_device};
        auto device_res = device_builder.build();
        if (!device_res) {
            FL::Log::err("Failed to build device from physical device: {}", device_res.error().message());
            return false;
        }

        device = device_res.value();
        return true;
    }

    bool createQueueFamilies(vkb::Device _device) {

        auto queue_res = _device.get_queue(vkb::QueueType::graphics);
        if (!queue_res) {
            FL::Log::err("Failed to build device from physical device: {}", queue_res.error().message());
            return false;
        }

        graphic_queue = queue_res.value();
        return true;
    }

};
