cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(app)

if(NOT CONFIG_DEBUG)
    add_definitions(-DNDEBUG)
endif()

target_link_libraries(app PRIVATE
    uhk
)

if(CONFIG_BOARD_UHK_60_RIGHT)
    target_sources(app PRIVATE
        src/uhk60/main.c
    )
else()
    target_sources(app PRIVATE
        src/main.c
    )
endif()
