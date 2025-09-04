cmake_minimum_required(VERSION 3.20.0)

if(DEFINED SdkRootDirPath)
    execute_process(COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR
    " Wrong SDK is selected, please run\n"
    " west config manifest.file west_nrfsdk.yml\n"
    " and try again.")
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(app)

set(IS_NRF_SDK ON)
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
