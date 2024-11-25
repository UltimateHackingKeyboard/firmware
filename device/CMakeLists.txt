cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(uhk)

if(NOT CONFIG_DEBUG)
    add_definitions(-DNDEBUG)
endif()

add_subdirectory(src)

# add the versions.c source file conditionally
if(NOT NOVERSIONS)
    target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../shared/versions.c)
    set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/../shared/versions.c PROPERTIES GENERATED TRUE)
endif()

if(NOT CONFIG_BOARD_UHK_60_RIGHT)
    # C headers compiled with C++ give pointer conversion errors, turn them to warnings
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive")

    # Link app to c2usb
    target_link_libraries(app PRIVATE
        c2usb
    )
endif()
