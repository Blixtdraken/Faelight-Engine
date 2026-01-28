module;

#include <VkBootstrap.h>
#include "GLFW/glfw3.h"

export module faelight.render.vulkanimpl;

import faelight.log;


//https://github.com/charles-lunarg/vk-bootstrap/blob/main/docs/getting_started.md

export namespace FL{

    struct VkRenderData {
        VkQueue graphics_queue;
        VkQueue present_queue;

        std::vector<VkImage> swapchain_images;
        std::vector<VkImageView> swapchain_image_views;
        std::vector<VkFramebuffer> framebuffers;

        VkRenderPass render_pass;
        VkPipelineLayout pipeline_layout;
        VkPipeline graphics_pipeline;

        VkCommandPool command_pool;
        std::vector<VkCommandBuffer> command_buffers;

        std::vector<VkSemaphore> available_semaphores;
        std::vector<VkSemaphore> finished_semaphore;
        std::vector<VkFence> in_flight_fences;
        std::vector<VkFence> image_in_flight;
        size_t current_frame = 0;
    };


    class VulkanBackend {

    public:

        vkb::Instance       instance;
        vkb::PhysicalDevice physical_device;
        vkb::Device         device;
        vkb::Swapchain      swapchain;
        vkb::DispatchTable  dispatch_table;

        VkSurfaceKHR        surface;

        VkRenderData        render_data;

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
            else if (!createSurface(_window)) Log::err("Failed creating surface!");
            else if (!pickPhysicalDevice()) Log::err("Failed to pick physical device!");
            else if (!createDevice()) Log::err("Failed to create logical device!");
            else if (!createDispatchTable()){} // uh, it can't really fail? maybe I should put it somwhere else
            else if (!createQueueFamilies()) Log::err("Failed to create queue families!");
            else if (!createSwapchain()) Log::err("Failed to create swapchain!");
            else if (!createRenderPass()) Log::err("Failed to create renderpass!");
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


        bool createSurface(GLFWwindow* _window) {
            Log::debug("Settting up Vulkan surface");
            VkResult err = glfwCreateWindowSurface(instance, _window, nullptr, &surface);
            if (err != VK_SUCCESS) {
                return false;
            }
            return true;
        }

        bool pickPhysicalDevice() {
            Log::debug("Selecting gpu for vulkan");
            vkb::PhysicalDeviceSelector phys_device_selector(instance);

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

        bool createDevice() {
            Log::debug("Setting up Vulkan logical device");
            vkb::DeviceBuilder device_builder{physical_device};
            auto device_res = device_builder.build();
            if (!device_res) {
                Log::err("Failed to build device from physical device: {}", device_res.error().message());
                return false;
            }

            device = device_res.value();
            return true;
        }

        bool createDispatchTable() {
            dispatch_table = device.make_table();
            return true;
        }

        bool createQueueFamilies() {
            Log::debug("Settting up Vulkan queue families");
            auto queue_res = device.get_queue(vkb::QueueType::graphics);
            if (!queue_res) {
                Log::err("Failed to build device from physical device: {}", queue_res.error().message());
                return false;
            }

            render_data.graphics_queue = queue_res.value();
            return true;
        }

        bool createSwapchain() {
            Log::debug("Settting up Vulkan swapchain");
            vkb::SwapchainBuilder swapchain_builder{device};
            auto swapchain_res = swapchain_builder.build();

            if (!swapchain_res) {
                Log::err("Failed to create swapchain: {}", swapchain_res.error().message());
                return false;
            }

            swapchain = swapchain_res.value();
            return true;
        }

        bool recreateSwapchain() {
            Log::debug("Recreating swapchain");

            vkb::SwapchainBuilder swapchain_builder{device};
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

        bool createRenderPass() {
            Log::debug("Setting up Vulkan render pass");

            VkAttachmentDescription color_attachment{};
            color_attachment.format = swapchain.image_format;
            color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
            color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

            VkAttachmentReference color_attachment_ref = {};
            color_attachment_ref.attachment = 0;
            color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            VkSubpassDescription subpass = {};
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.colorAttachmentCount = 1;
            subpass.pColorAttachments = &color_attachment_ref;

            VkSubpassDependency dependency = {};
            dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
            dependency.dstSubpass = 0;
            dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependency.srcAccessMask = 0;
            dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

            VkRenderPassCreateInfo render_pass_info = {};
            render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            render_pass_info.attachmentCount = 1;
            render_pass_info.pAttachments = &color_attachment;
            render_pass_info.subpassCount = 1;
            render_pass_info.pSubpasses = &subpass;
            render_pass_info.dependencyCount = 1;
            render_pass_info.pDependencies = &dependency;

            if (dispatch_table.createRenderPass(&render_pass_info, nullptr, &render_data.render_pass) != VK_SUCCESS) {
                return false; // failed to create render pass!
            }
            return true;

        }

        
    };

}
