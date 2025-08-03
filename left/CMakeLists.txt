cmake_minimum_required(VERSION 3.22.0)

add_subdirectory(sysbuild)

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

include(${CMAKE_CURRENT_SOURCE_DIR}/../shared/CMakeLists.txt)
include(${CMAKE_CURRENT_SOURCE_DIR}/../shared/module/CMakeLists.txt)
