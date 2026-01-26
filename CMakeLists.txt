include(FetchContent)

cmake_minimum_required(VERSION 4.2)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

project(Faelight)

set(CMAKE_CXX_STANDARD 23)

add_executable(Faelight entrypoint/main.cpp)

target_sources(Faelight
        PUBLIC
        FILE_SET CXX_MODULES FILES
        common/log.cppm
        common/signals.cppm
)



include(FetchContent)

FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG        3.3.9 # Or the latest stable version
)

# We don't need GLFW's internal tests or documentation
set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "")
set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "")
set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "")
set(GLFW_INSTALL OFF CACHE INTERNAL "")

FetchContent_MakeAvailable(glfw)

# Link it to your executable or library
target_link_libraries(Faelight PRIVATE glfw)