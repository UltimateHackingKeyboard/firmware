cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(uhk)

target_include_directories(app PUBLIC
    src
)

file(GLOB usb_sources src/usb/*.c)
file(GLOB usb_cpp_sources src/usb/*.cpp)
file(GLOB keyboard_sources src/keyboard/*.c)
file(GLOB app_sources src/*.c)
file(GLOB app_cpp_sources src/*.cpp)

if(BOARD STREQUAL "uhk-60-right")
    file(GLOB main src/uhk60/main.c)
    target_sources(app PRIVATE ${main})
elseif ((BOARD STREQUAL "uhk-dongle"))
    target_sources(app PRIVATE ${app_sources} ${app_cpp_sources} ${usb_sources} ${usb_cpp_sources})
else()
    target_sources(app PRIVATE ${app_sources} ${app_cpp_sources} ${usb_sources} ${usb_cpp_sources} ${keyboard_sources})
endif()

if(NOT BOARD STREQUAL "uhk-60-right")
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
endif()
