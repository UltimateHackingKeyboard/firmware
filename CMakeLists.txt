cmake_minimum_required(VERSION 3.20)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CONFIG_DEVICE_ID)
    # uhk code isn't added to bootloader
    return()
endif()

set(IS_NRF_SDK ON)

project(uhk)
add_library(${PROJECT_NAME} OBJECT)
target_include_directories(${PROJECT_NAME} PUBLIC
    # this really needs to be changed when time and restructuring permits
    ${CMAKE_CURRENT_SOURCE_DIR}
)
include(shared/CMakeLists.txt)
add_subdirectory(device/src)
add_subdirectory(right/src)
target_link_libraries(${PROJECT_NAME} PUBLIC
    zephyr_interface
    c2usb
)
