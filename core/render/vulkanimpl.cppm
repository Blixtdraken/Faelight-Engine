module;

#include <fstream>
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
            else if (!createPipelne()) Log::err("Failed to create pipeline!");
            else if (!createFrameBuffers()) Log::err("Failed to create frame buffers!");
            else if (!createCommandPool()) Log::err("Failed to create command pool!");
            else if (!createCommandBuffers()) Log::err("Failed to create command buffers!");
            else if (!createSyncObjects()) Log::err("Failed to create sync object!");
            else {
                Log::info("Successfully setup Vulkan");
                return true;
            }
            Log::err("Vulkan setup creation failed!");
            return false;
        }

        VkSurfaceKHR& get_surface() {return surface;}

        bool drawFrame() {
            dispatch_table.waitForFences(1, &render_data.in_flight_fences[render_data.current_frame], VK_TRUE, UINT64_MAX);

            uint32_t image_index = 0;
            VkResult result = dispatch_table.acquireNextImageKHR(swapchain, UINT64_MAX, render_data.available_semaphores[render_data.current_frame], VK_NULL_HANDLE, &image_index);

            if (result == VK_ERROR_OUT_OF_DATE_KHR) {
                return recreateSwapchain();
            } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
                Log::err("Failed to use swapchain when drawing frame!");
                return false;
            }

            if (render_data.image_in_flight[image_index] != VK_NULL_HANDLE) {
                dispatch_table.waitForFences(1, &render_data.image_in_flight[image_index], VK_TRUE, UINT64_MAX);
            }
            render_data.image_in_flight[image_index] = render_data.in_flight_fences[render_data.current_frame];

            VkSubmitInfo submitInfo = {};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

            VkSemaphore wait_semaphores[] = { render_data.available_semaphores[render_data.current_frame] };
            VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
            submitInfo.waitSemaphoreCount = 1;
            submitInfo.pWaitSemaphores = wait_semaphores;
            submitInfo.pWaitDstStageMask = wait_stages;

            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &render_data.command_buffers[image_index];

            VkSemaphore signal_semaphores[] = { render_data.finished_semaphore[image_index] };
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = signal_semaphores;

            dispatch_table.resetFences(1, &render_data.in_flight_fences[render_data.current_frame]);

            if (dispatch_table.queueSubmit(render_data.graphics_queue, 1, &submitInfo, render_data.in_flight_fences[render_data.current_frame]) != VK_SUCCESS) {
               Log::err("Failed to submit draw command buffer");
                return false;
            }

            VkPresentInfoKHR present_info = {};
            present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

            present_info.waitSemaphoreCount = 1;
            present_info.pWaitSemaphores = signal_semaphores;

            VkSwapchainKHR swapChains[] = { swapchain };
            present_info.swapchainCount = 1;
            present_info.pSwapchains = swapChains;

            present_info.pImageIndices = &image_index;

            result = dispatch_table.queuePresentKHR(render_data.present_queue, &present_info);
            if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
                return recreateSwapchain();
            } else if (result != VK_SUCCESS) {
                Log::err("Failed to present swapchain!");
                return false;
            }

            render_data.current_frame = (render_data.current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
            return true;

        }


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
            auto g_queue_res = device.get_queue(vkb::QueueType::graphics);
            if (!g_queue_res) {
                Log::err("Failed to get graphics queue: {}", g_queue_res.error().message());
                return false;
            }
            render_data.graphics_queue = g_queue_res.value();

            auto p_queue_res = device.get_queue(vkb::QueueType::present);
            if (!p_queue_res) {
                Log::err("Failed to get present queue: {}", g_queue_res.error().message());
                return false;
            }
            render_data.present_queue = p_queue_res.value();

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
            //vkb::destroy_swapchain(swapchain);
            swapchain = swapchain_res.value();
            return true;
        }

        bool recreateSwapchain() {
            Log::debug("Recreating swapchain");

            dispatch_table.deviceWaitIdle();

            dispatch_table.destroyCommandPool(render_data.command_pool, nullptr);
            for (auto framebuffer : render_data.framebuffers) {
                dispatch_table.destroyFramebuffer(framebuffer, nullptr);
            }

            swapchain.destroy_image_views(render_data.swapchain_image_views);

            vkb::destroy_swapchain(swapchain);

            if (!createSwapchain()) Log::err("Failed to recreate swapchain!");
            else if (!createFrameBuffers()) Log::err("Failed to recreate framebuffers!");
            else if (!createCommandPool()) Log::err("Failed to recreate command pool!");
            else if (!createCommandBuffers()) Log::err("Failed to recreate command buffers!");
            else {
                Log::debug("Swapchain recreated!");
                return true;
            }

            return false;
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




        std::vector<char> readFile(const std::string& filename) {
            std::ifstream file(filename, std::ios::ate | std::ios::binary);

            if (!file.is_open()) {
                Log::err("Failed to open shader file: {}", filename.c_str());
                throw std::runtime_error("failed to open file!");
            }

            size_t file_size = (size_t)file.tellg();
            std::vector<char> buffer(file_size);

            file.seekg(0);
            file.read(buffer.data(), static_cast<std::streamsize>(file_size));

            file.close();

            return buffer;
        }

        VkShaderModule createShaderModule(const std::vector<char>& code) {
            VkShaderModuleCreateInfo create_info = {};
            create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            create_info.codeSize = code.size();
            create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());

            VkShaderModule shaderModule;
            if (dispatch_table.createShaderModule(&create_info, nullptr, &shaderModule) != VK_SUCCESS) {
                return VK_NULL_HANDLE; // failed to create shader module
            }

            return shaderModule;
        }

        bool createPipelne() {
            VkShaderModule vert_shader_module = createShaderModule(readFile("shaders/vert.spv"));
            VkShaderModule frag_shader_module = createShaderModule(readFile("shaders/frag.spv"));


            if (!vert_shader_module) {
                Log::err("Failed to create vertex shader module!");
                return false;
            }
            if (!frag_shader_module) {
                Log::err("Failed to create fragment shader module!");
                return false;
            }

            VkPipelineShaderStageCreateInfo vert_stage_info = {};
            vert_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            vert_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
            vert_stage_info.module = vert_shader_module;
            vert_stage_info.pName = "main";

            VkPipelineShaderStageCreateInfo frag_stage_info = {};
            frag_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            frag_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
            frag_stage_info.module = frag_shader_module;
            frag_stage_info.pName = "main";

            VkPipelineShaderStageCreateInfo shader_stages[] = { vert_stage_info, frag_stage_info };

            VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
            vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            vertex_input_info.vertexBindingDescriptionCount = 0;
            vertex_input_info.vertexAttributeDescriptionCount = 0;

            VkPipelineInputAssemblyStateCreateInfo input_assembly = {};
            input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
            input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            input_assembly.primitiveRestartEnable = VK_FALSE;

            VkViewport viewport = {};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = (float)swapchain.extent.width;
            viewport.height = (float)swapchain.extent.height;
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;

            VkRect2D scissor = {};
            scissor.offset = { 0, 0 };
            scissor.extent = swapchain.extent;

            VkPipelineViewportStateCreateInfo viewport_state = {};
            viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            viewport_state.viewportCount = 1;
            viewport_state.pViewports = &viewport;
            viewport_state.scissorCount = 1;
            viewport_state.pScissors = &scissor;

            VkPipelineRasterizationStateCreateInfo rasterizer = {};
            rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            rasterizer.depthClampEnable = VK_FALSE;
            rasterizer.rasterizerDiscardEnable = VK_FALSE;
            rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
            rasterizer.lineWidth = 1.0f;
            rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
            rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
            rasterizer.depthBiasEnable = VK_FALSE;

            VkPipelineMultisampleStateCreateInfo multisampling = {};
            multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            multisampling.sampleShadingEnable = VK_FALSE;
            multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

            VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
            colorBlendAttachment.colorWriteMask =
                VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            colorBlendAttachment.blendEnable = VK_FALSE;

            VkPipelineColorBlendStateCreateInfo color_blending = {};
            color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            color_blending.logicOpEnable = VK_FALSE;
            color_blending.logicOp = VK_LOGIC_OP_COPY;
            color_blending.attachmentCount = 1;
            color_blending.pAttachments = &colorBlendAttachment;
            color_blending.blendConstants[0] = 0.0f;
            color_blending.blendConstants[1] = 0.0f;
            color_blending.blendConstants[2] = 0.0f;
            color_blending.blendConstants[3] = 0.0f;

            VkPipelineLayoutCreateInfo pipeline_layout_info = {};
            pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipeline_layout_info.setLayoutCount = 0;
            pipeline_layout_info.pushConstantRangeCount = 0;

            if (dispatch_table.createPipelineLayout(&pipeline_layout_info, nullptr, &render_data.pipeline_layout) != VK_SUCCESS) {
                Log::err("Failed to create pipeline layout!");
                return false; // failed to create pipeline layout
            }

            std::vector<VkDynamicState> dynamic_states = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

            VkPipelineDynamicStateCreateInfo dynamic_info = {};
            dynamic_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
            dynamic_info.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
            dynamic_info.pDynamicStates = dynamic_states.data();

            VkGraphicsPipelineCreateInfo pipeline_info = {};
            pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            pipeline_info.stageCount = 2;
            pipeline_info.pStages = shader_stages;
            pipeline_info.pVertexInputState = &vertex_input_info;
            pipeline_info.pInputAssemblyState = &input_assembly;
            pipeline_info.pViewportState = &viewport_state;
            pipeline_info.pRasterizationState = &rasterizer;
            pipeline_info.pMultisampleState = &multisampling;
            pipeline_info.pColorBlendState = &color_blending;
            pipeline_info.pDynamicState = &dynamic_info;
            pipeline_info.layout = render_data.pipeline_layout;
            pipeline_info.renderPass = render_data.render_pass;
            pipeline_info.subpass = 0;
            pipeline_info.basePipelineHandle = VK_NULL_HANDLE;

            if (dispatch_table.createGraphicsPipelines(VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &render_data.graphics_pipeline) != VK_SUCCESS) {
                Log::err("Failed to turn layout into pipeline.");
                return false; // failed to create graphics pipeline
            }

            dispatch_table.destroyShaderModule(frag_shader_module, nullptr);
            dispatch_table.destroyShaderModule(vert_shader_module, nullptr);
            return true;
        }

        bool createFrameBuffers() {

            render_data.swapchain_images = swapchain.get_images().value();
            render_data.swapchain_image_views = swapchain.get_image_views().value();

            render_data.framebuffers.resize(render_data.swapchain_image_views.size());

            for (size_t i = 0; i < render_data.swapchain_image_views.size(); i++) {
                VkImageView attachments[] = { render_data.swapchain_image_views[i] };

                VkFramebufferCreateInfo framebuffer_info = {};
                framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                framebuffer_info.renderPass = render_data.render_pass;
                framebuffer_info.attachmentCount = 1;
                framebuffer_info.pAttachments = attachments;
                framebuffer_info.width = swapchain.extent.width;
                framebuffer_info.height = swapchain.extent.height;
                framebuffer_info.layers = 1;

                if (dispatch_table.createFramebuffer(&framebuffer_info, nullptr, &render_data.framebuffers[i]) != VK_SUCCESS) {
                    return false; // failed to create framebuffer
                }
            }
            return true;
        }

        bool createCommandPool() {
            VkCommandPoolCreateInfo pool_info = {};
            pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            pool_info.queueFamilyIndex = device.get_queue_index(vkb::QueueType::graphics).value();

            if (dispatch_table.createCommandPool(&pool_info, nullptr, &render_data.command_pool) != VK_SUCCESS) {
                return false; // failed to create command pool
            }
            return true;
        }

        bool createCommandBuffers() {

            render_data.command_buffers.resize(render_data.framebuffers.size());

            VkCommandBufferAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.commandPool = render_data.command_pool;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandBufferCount = (uint32_t)render_data.command_buffers.size();

            if (dispatch_table.allocateCommandBuffers(&allocInfo, render_data.command_buffers.data()) != VK_SUCCESS) {
                Log::err("Failed to allocate command buffers!");
                return false;
            }

            for (size_t i = 0; i < render_data.command_buffers.size(); i++) {
                VkCommandBufferBeginInfo begin_info = {};
                begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

                if (dispatch_table.beginCommandBuffer(render_data.command_buffers[i], &begin_info) != VK_SUCCESS) {
                    Log::err("Failed to begin command buffer!");
                    return false;
                }

                VkRenderPassBeginInfo render_pass_info = {};
                render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                render_pass_info.renderPass = render_data.render_pass;
                render_pass_info.framebuffer = render_data.framebuffers[i];
                render_pass_info.renderArea.offset = { 0, 0 };
                render_pass_info.renderArea.extent = swapchain.extent;
                VkClearValue clearColor{ { { 0.0f, 0.0f, 0.0f, 1.0f } } };
                render_pass_info.clearValueCount = 1;
                render_pass_info.pClearValues = &clearColor;

                VkViewport viewport = {};
                viewport.x = 0.0f;
                viewport.y = 0.0f;
                viewport.width =  (float)swapchain.extent.width;
                viewport.height = (float)swapchain.extent.height;
                viewport.minDepth = 0.0f;
                viewport.maxDepth = 1.0f;

                VkRect2D scissor = {};
                scissor.offset = { 0, 0 };
                scissor.extent = swapchain.extent;

                dispatch_table.cmdSetViewport(render_data.command_buffers[i], 0, 1, &viewport);
                dispatch_table.cmdSetScissor(render_data.command_buffers[i], 0, 1, &scissor);

                dispatch_table.cmdBeginRenderPass(render_data.command_buffers[i], &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

                dispatch_table.cmdBindPipeline(render_data.command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, render_data.graphics_pipeline);

                dispatch_table.cmdDraw(render_data.command_buffers[i], 3, 1, 0, 0);

                dispatch_table.cmdEndRenderPass(render_data.command_buffers[i]);

                if (dispatch_table.endCommandBuffer(render_data.command_buffers[i]) != VK_SUCCESS) {
                    Log::err("Failed to end command buffer!");
                    return false;
                }
            }
            return true;
        }


        const int MAX_FRAMES_IN_FLIGHT = 2;

        bool createSyncObjects() {
            render_data.available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
            render_data.finished_semaphore.resize(swapchain.image_count);
            render_data.in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);
            render_data.image_in_flight.resize(swapchain.image_count, VK_NULL_HANDLE);

            VkSemaphoreCreateInfo semaphore_info = {};
            semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            VkFenceCreateInfo fence_info = {};
            fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

            for (size_t i = 0; i < swapchain.image_count; i++) {
                if (dispatch_table.createSemaphore(&semaphore_info, nullptr, &render_data.finished_semaphore[i]) != VK_SUCCESS) {
                    Log::err("Failed to create semaphore!");
                    return false; // failed to create synchronization objects for a frame
                }
            }

            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                if (dispatch_table.createSemaphore(&semaphore_info, nullptr, &render_data.available_semaphores[i]) != VK_SUCCESS ||
                    dispatch_table.createFence(&fence_info, nullptr, &render_data.in_flight_fences[i]) != VK_SUCCESS) {
                    Log::err("Failed to create sync object!");
                    return false; // failed to create synchronization objects for a frame
                    }
            }
            return true;
        }

    };

}
