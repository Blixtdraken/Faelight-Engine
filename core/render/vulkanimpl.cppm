module;

#include <VkBootstrap.h>

import faelight.log;

export module vulkanimpl;


//https://github.com/charles-lunarg/vk-bootstrap/blob/main/docs/getting_started.md


class VulkanImpl {

    vkb::Instance instance;


    bool createInstance() {

        auto instance_builder = vkb::InstanceBuilder()
            .set_app_name("Fairy Forest")
            .set_engine_name("Faelight")
            .require_api_version(1,3,0);

        auto system_rep = vkb::SystemInfo::get_system_info();

        if (!system_rep) {
            FL::Log::err("Failed to get system info in instance creation!");
            return false;
        }
        auto system_info = system_rep.value();

        if (system_info.validation_layers_available) {
            instance_builder.enable_validation_layers()
            .use_default_debug_messenger(); // TODO: Set to custom debug messanger for better logs
        }

        auto instance_rep = instance_builder.build();

        if (!instance_rep) {
            FL::Log::err("Failed to create vulkan instance!");
            return false;
        }


        instance = new vkb::Instance(instance_rep.value());
        return true;
    }

    bool pickPhysicalDevice(vkb::Instance _instance) {
        vkb::PhysicalDeviceSelector phys_device_selector(_instance);
        
    }

};
