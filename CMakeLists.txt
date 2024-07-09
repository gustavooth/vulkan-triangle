cmake_minimum_required(VERSION 3.29)

project(triangle C)
set(CMAKE_C_COMPILER "C:/Program Files/LLVM/bin/clang.exe")
set(CMAKE_C_STANDARD 23)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)

file(GLOB SOURCE "src/*.c")

add_executable(triangle ${SOURCE})

target_link_directories(triangle PUBLIC $ENV{VULKAN_SDK}/Lib)
include_directories($ENV{VULKAN_SDK}/Include)

target_link_libraries(triangle user32 vulkan-1)