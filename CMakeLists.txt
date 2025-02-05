cmake_minimum_required(VERSION 3.20)

# until https://github.com/zephyrproject-rtos/zephyr/pull/69490 is merged
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# zephyr's build system doesn't define standard build type symbols, fix that here
if(NOT CONFIG_DEBUG)
    add_definitions(-DNDEBUG)
endif()

if(NOT CONFIG_DEVICE_ID)
    # uhk code isn't added to bootloader
    return()
endif()

project(uhk)
add_subdirectory(shared)
add_subdirectory(right/src)
add_subdirectory(device/src)
target_link_libraries(${PROJECT_NAME} PUBLIC
    zephyr_interface
)
