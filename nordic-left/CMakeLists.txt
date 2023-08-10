# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(uhk_nordic_left)

target_include_directories(app PUBLIC
    src
)

FILE(GLOB app_sources src/*.c)
FILE(GLOB app_cpp_sources src/*.cpp)
target_sources(app PRIVATE ${app_sources} ${app_cpp_sources})

# C headers compiled with C++ give pointer conversion errors, turn them to warnings
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive")

set(C2USB_PATH "c2usb")

add_subdirectory(${C2USB_PATH})
# link c2usb to the abstract Zephyr interface to inherit the build flags
target_link_libraries(c2usb zephyr_interface)

# Link app to c2usb
target_link_libraries(app PRIVATE
    c2usb
)
