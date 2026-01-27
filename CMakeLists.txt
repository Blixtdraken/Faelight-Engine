include(FetchContent)

cmake_minimum_required(VERSION 4.2)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

project(Faelight)

set(CMAKE_CXX_STANDARD 23)


find_package(Vulkan REQUIRED)

include(FetchContent)

FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG        3.4 # Or the latest stable version
)
# We don't need GLFW's internal tests or documentation
set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "")
set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "")
set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "")
set(GLFW_INSTALL OFF CACHE INTERNAL "")


FetchContent_Declare(
        vk_bootstrap
        GIT_REPOSITORY https://github.com/charles-lunarg/vk-bootstrap.git
        GIT_TAG        v1.4.335 # Or a specific release tag
)


FetchContent_MakeAvailable(glfw vk_bootstrap)

add_executable(Faelight entrypoint/main.cpp
)

target_sources(Faelight
        PUBLIC
        FILE_SET CXX_MODULES FILES
        common/log.cppm
        common/signals.cppm
        core/render/vulkanimpl.cppm
)



# Link it to your executable or library
target_link_libraries(Faelight
        PRIVATE
        vk-bootstrap
        glfw
        Vulkan::Vulkan
)