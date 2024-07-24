cmake_minimum_required(VERSION 3.29)

if(BOOTSTRAP_CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(LINUX TRUE)
endif()

project(triangle C)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_C_STANDARD 23)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)
file(GLOB_RECURSE SOURCE "src/*.c")
include_directories(${CMAKE_SOURCE_DIR}/src/)

if(WIN32)
    set(CMAKE_C_COMPILER "C:/Program Files/LLVM/bin/clang.exe")
    add_definitions(-DPLATFORM_WIN32)
    include_directories($ENV{VULKAN_SDK}/Include)
elseif(LINUX)
    set(CMAKE_C_COMPILER clang)
    add_definitions(-DPLATFORM_WAYLAND)
    include_directories(/usr/include)
endif()

add_executable(triangle ${SOURCE})

if(WIN32)
    target_link_directories(triangle PUBLIC $ENV{VULKAN_SDK}/Lib)
    target_link_libraries(triangle user32 vulkan-1)
elseif(LINUX)
    target_link_libraries(triangle wayland-client vulkan)
endif()