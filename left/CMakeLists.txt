cmake_minimum_required(VERSION 3.22.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(board frdmkl03z) # TODO: how to remove the board selection?
set(device MKL03Z4)

include(${SdkRootDirPath}/cmake/extension/mcux.cmake)
project(uhk-60-left
    LANGUAGES "ASM" "C" "CXX"
)
include(${SdkRootDirPath}/CMakeLists.txt)

set(IS_MCUX_SDK ON)

target_compile_definitions(${PROJECT_NAME} PUBLIC
    __STARTUP_CLEAR_BSS
)

target_include_directories(${PROJECT_NAME} PUBLIC
    src
)
target_sources(${PROJECT_NAME} PRIVATE
    src/module.c
)
include(${CMAKE_CURRENT_SOURCE_DIR}/../shared/CMakeLists.txt)
include(${CMAKE_CURRENT_SOURCE_DIR}/../shared/module/CMakeLists.txt)

mcux_add_linker_symbol(SYMBOLS
    __heap_size__=0
    __stack_size__=0x0050
)

mcux_convert_binary(
    BINARY "${APPLICATION_BINARY_DIR}/${PROJECT_NAME}.hex"
)
