module;

#include <VkBootstrap.h>
#include "GLFW/glfw3.h"

export module faelight.render.vulkanimpl;

import faelight.log;


//https://github.com/charles-lunarg/vk-bootstrap/blob/main/docs/getting_started.md

export namespace FL{
    class VulkanBackend {

    public:

        vkb::Instance       instance;
        vkb::PhysicalDevice physical_device;
        vkb::Device         device;
        vkb::Swapchain      swapchain;


        VkQueue_T*          graphic_queue = {};
        VkSurfaceKHR        surface;

        VulkanBackend(){}
        ~VulkanBackend() {
            destroy();
        }

        void destroy() {
            Log::info("Destroying Vulkan");

            vkb::destroy_swapchain(swapchain);
            vkb::destroy_device(device);
            vkb::destroy_surface(instance, surface);
            vkb::destroy_instance(instance);
        }

        bool setup(GLFWwindow* _window) {
            if (!createInstance()) Log::err("Failed creating instance!");
            else if (!createSurface(instance, _window)) Log::err("Failed creating surface!");
            else if (!pickPhysicalDevice(instance)) Log::err("Failed to pick physical device!");
            else if (!createDevice(physical_device)) Log::err("Failed to create logical device!");
            else if (!createQueueFamilies(device)) Log::err("Failed to create queue families!");
            else if (!createSwapchain(device)) Log::err("Failed to create swapchain!");
            else {
                Log::info("Successfully setup Vulkan");
                return true;
            }
            Log::err("Vulkan setup creation failed!");
            return false;
        }

        VkSurfaceKHR& get_surface() {return surface;}


        bool createInstance() {
            Log::debug("Settting up Vulkan instance");
            auto instance_builder = vkb::InstanceBuilder()
                .set_app_name("The Faewild")
                .set_engine_name("Faelight")
                .require_api_version(1,3,0);

            auto system_res = vkb::SystemInfo::get_system_info();

            if (!system_res) {
                Log::err("Failed to get system info in instance creation!");
                return false;
            }
            auto system_info = system_res.value();

            if (system_info.validation_layers_available) {
                instance_builder.enable_validation_layers()
                .use_default_debug_messenger(); // TODO: Set to custom debug messanger for better logs
            }

            auto instance_res = instance_builder.build();

            if (!instance_res) {
                Log::err("Failed to create vulkan instance: {}", instance_res.error().message());
                return false;
            }


            instance = instance_res.value();
            return true;
        }


        bool createSurface(VkInstance _instance, GLFWwindow* _window) {
            Log::debug("Settting up Vulkan surface");
            VkResult err = glfwCreateWindowSurface(_instance, _window, nullptr, &surface);
            if (err != VK_SUCCESS) {
                return false;
            }
            return true;
        }

        bool pickPhysicalDevice(vkb::Instance _instance) {
            Log::debug("Selecting gpu for vulkan");
            vkb::PhysicalDeviceSelector phys_device_selector(_instance);

            auto physical_device_res = phys_device_selector
            .set_surface(surface)
            .select();

            if (!physical_device_res) {
                Log::err("Gpu pick failed: {}", physical_device_res.error().message());
                const auto& detailed_reasons = physical_device_res.detailed_failure_reasons();
                if (!detailed_reasons.empty()) {
                    for (const std::string& reason : detailed_reasons) {
                        Log::err("Gpu pick failed: {}", reason);
                    }
                }
                return false;
            }

            physical_device = physical_device_res.value();
            Log::info("Selected GPU: {}", physical_device.name);
            return true;
        }

        bool createDevice(vkb::PhysicalDevice _physical_device) {
            Log::debug("Setting up Vulkan logical device");
            vkb::DeviceBuilder device_builder{_physical_device};
            auto device_res = device_builder.build();
            if (!device_res) {
                Log::err("Failed to build device from physical device: {}", device_res.error().message());
                return false;
            }

            device = device_res.value();
            return true;
        }

        bool createQueueFamilies(vkb::Device _device) {
            Log::debug("Settting up Vulkan queue families");
            auto queue_res = _device.get_queue(vkb::QueueType::graphics);
            if (!queue_res) {
                Log::err("Failed to build device from physical device: {}", queue_res.error().message());
                return false;
            }

            graphic_queue = queue_res.value();
            return true;
        }

        bool createSwapchain(vkb::Device _device) {
            Log::debug("Settting up Vulkan swapchain");
            vkb::SwapchainBuilder swapchain_builder{_device};
            auto swapchain_res = swapchain_builder.build();

            if (!swapchain_res) {
                Log::err("Failed to create swapchain: {}", swapchain_res.error().message());
                return false;
            }

            swapchain = swapchain_res.value();
            return true;
        }

        bool recreateSwapchain(vkb::Device _device) {
            Log::debug("Recreating swapchain");

            vkb::SwapchainBuilder swapchain_builder{_device};
            auto swapchain_res = swapchain_builder
                .set_old_swapchain(swapchain)
                .build();

            if (!swapchain_res) {
                Log::err("Failed to recreate swapchain: {}", swapchain_res.error().message());
                swapchain.swapchain = VK_NULL_HANDLE;
                return false;
            }
            vkb::destroy_swapchain(swapchain);
            swapchain = swapchain_res.value();
            return true;
        }

        
    };

}
