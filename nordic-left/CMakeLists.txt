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

# Add the c2usb sources, and the support libraries
add_subdirectory(${C2USB_PATH}/c2usb)
add_subdirectory(${C2USB_PATH}/modules/magic_enum)
add_subdirectory(${C2USB_PATH}/modules/etl)
add_subdirectory(${C2USB_PATH}/modules/hid-rp)

# Add the c2usb sources to your app build
target_sources(app PRIVATE
    ${C2USB_PUBLIC_HEADERS}
    ${C2USB_PRIVATE_HEADERS}
    ${C2USB_SOURCES}
)

# Add c2usb include path
target_include_directories(app PUBLIC
    c2usb/c2usb
    c2usb/examples
)

# Link to c2usb's dependencies
target_link_libraries(app PRIVATE
    magic_enum
    etl
    hid-rp
)
