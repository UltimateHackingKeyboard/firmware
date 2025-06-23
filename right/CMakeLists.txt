cmake_minimum_required(VERSION 3.22.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(board frdmk22f) # TODO: how to remove the board selection?
set(device MK22F51212)

include(${SdkRootDirPath}/cmake/extension/mcux.cmake)
project(uhk-60-right
    LANGUAGES "ASM" "C" "CXX"
)
include(${SdkRootDirPath}/CMakeLists.txt)

set(IS_MCUX_SDK ON)
set(BL_APP_VECTOR_TABLE_ADDRESS 0xc000)

target_compile_definitions(${PROJECT_NAME} PUBLIC
    PROJECT_NAME=${DEVICE_PROJECT_NAME}
    DEVICE_ID=${DEVICE_ID}
    DEVICE_PID=${DEVICE_PID}
    BL_HAS_BOOTLOADER_CONFIG=1
    BL_APP_VECTOR_TABLE_ADDRESS=${BL_APP_VECTOR_TABLE_ADDRESS}
    __STARTUP_CLEAR_BSS
    #USB_STACK_BM
)

add_subdirectory(src)
target_include_directories(${PROJECT_NAME} PRIVATE
    # as some headers are included via shared/ prefix
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
include(${CMAKE_CURRENT_SOURCE_DIR}/../shared/CMakeLists.txt)
mcux_add_source(SOURCES ../lib/bootloader/src/bootloader/src/wormhole.c)
mcux_add_include(INCLUDES ../lib/bootloader/src)

mcux_add_linker_symbol(SYMBOLS
    __heap_size__=0x2000
    __stack_size__=0x0400
    __bl_app_vector_table_address__=${BL_APP_VECTOR_TABLE_ADDRESS}
)
mcux_add_armgcc_linker_script(LINKER src/link/MK22FN512xxx12_flash.ld)

mcux_convert_binary(
    BINARY "${APPLICATION_BINARY_DIR}/${PROJECT_NAME}.hex"
)
