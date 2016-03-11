INCLUDE(CMakeForceCompiler)

# CROSS COMPILER SETTING
SET(CMAKE_SYSTEM_NAME Generic)
CMAKE_MINIMUM_REQUIRED (VERSION 2.6)

# THE VERSION NUMBER
SET (Tutorial_VERSION_MAJOR 1)
SET (Tutorial_VERSION_MINOR 0)

# ENABLE ASM
ENABLE_LANGUAGE(ASM)

SET(CMAKE_STATIC_LIBRARY_PREFIX)
SET(CMAKE_STATIC_LIBRARY_SUFFIX)

SET(CMAKE_EXECUTABLE_LIBRARY_PREFIX)
SET(CMAKE_EXECUTABLE_LIBRARY_SUFFIX)

# CURRENT DIRECTORY
SET(SrcDir "${CMAKE_CURRENT_SOURCE_DIR}/../..")
SET(KsdkDir "${SrcDir}/../lib/KSDK_1.3_FRDM-KL03Z")
SET(IncludeDir "${SrcDir}/../include")

# DEBUG LINK FILE
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -T${KsdkDir}/platform/devices/MKL03Z4/linker/gcc/MKL03Z32xxx4_flash.ld  -static")

# RELEASE LINK FILE
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -T${KsdkDir}/platform/devices/MKL03Z4/linker/gcc/MKL03Z32xxx4_flash.ld  -static")

# DEBUG ASM FLAGS
SET(CMAKE_ASM_FLAGS_DEBUG "${CMAKE_ASM_FLAGS_DEBUG} -g  -mcpu=cortex-m0plus  -mthumb  -Wall  -fno-common  -ffunction-sections  -fdata-sections  -ffreestanding  -fno-builtin  -mapcs  -std=gnu99")

# DEBUG C FLAGS
SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g  -O0  -mcpu=cortex-m0plus  -mthumb  -MMD  -MP  -Wall  -fno-common  -ffunction-sections  -fdata-sections  -ffreestanding  -fno-builtin  -mapcs  -std=gnu99")

# DEBUG LD FLAGS
SET(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -g  --specs=nano.specs  -lm  -Wall  -fno-common  -ffunction-sections  -fdata-sections  -ffreestanding  -fno-builtin  -mthumb  -mapcs  -Xlinker --gc-sections  -Xlinker -static  -Xlinker -z  -Xlinker muldefs  -Xlinker --defsym=__stack_size__=0x300  -Xlinker --defsym=__heap_size__=0x200")

# RELEASE ASM FLAGS
SET(CMAKE_ASM_FLAGS_RELEASE "${CMAKE_ASM_FLAGS_RELEASE} -mcpu=cortex-m0plus  -mthumb  -Wall  -fno-common  -ffunction-sections  -fdata-sections  -ffreestanding  -fno-builtin  -mapcs  -std=gnu99")

# RELEASE C FLAGS
SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Os  -mcpu=cortex-m0plus  -mthumb  -MMD  -MP  -Wall  -fno-common  -ffunction-sections  -fdata-sections  -ffreestanding  -fno-builtin  -mapcs  -std=gnu99")

# RELEASE LD FLAGS
SET(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} --specs=nano.specs  -lm  -Wall  -fno-common  -ffunction-sections  -fdata-sections  -ffreestanding  -fno-builtin  -mthumb  -mapcs  -Xlinker --gc-sections  -Xlinker -static  -Xlinker -z  -Xlinker muldefs  -Xlinker --defsym=__stack_size__=0x300  -Xlinker --defsym=__heap_size__=0x200")

# ASM MACRO
SET(CMAKE_ASM_FLAGS_DEBUG "${CMAKE_ASM_FLAGS_DEBUG}  -DDEBUG")

# C MACRO
SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}  -DDEBUG")
SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}  -DCPU_MKL03Z32VFK4")
SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}  -DFRDM_KL03Z")
SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}  -DFREEDOM")
SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}  -DNDEBUG")
SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}  -DCPU_MKL03Z32VFK4")
SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}  -DFRDM_KL03Z")
SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}  -DFREEDOM")

# CXX MACRO

# INCLUDE_DIRECTORIES
IF(CMAKE_BUILD_TYPE MATCHES Debug)
    INCLUDE_DIRECTORIES(${KsdkDir}/platform/osa/inc)
    INCLUDE_DIRECTORIES(${KsdkDir}/platform/utilities/inc)
    INCLUDE_DIRECTORIES(${KsdkDir}/platform/CMSIS/Include)
    INCLUDE_DIRECTORIES(${KsdkDir}/platform/devices)
    INCLUDE_DIRECTORIES(${KsdkDir}/platform/devices/MKL03Z4/include)
    INCLUDE_DIRECTORIES(${KsdkDir}/platform/devices/MKL03Z4/startup)
    INCLUDE_DIRECTORIES(${KsdkDir}/platform/hal/inc)
    INCLUDE_DIRECTORIES(${KsdkDir}/platform/drivers/inc)
    INCLUDE_DIRECTORIES(${KsdkDir}/platform/system/inc)
    INCLUDE_DIRECTORIES(${KsdkDir}/platform/drivers/src/gpio)
    INCLUDE_DIRECTORIES(${KsdkDir}/examples)
    INCLUDE_DIRECTORIES(${KsdkDir}/examples/frdmkl03z)
    INCLUDE_DIRECTORIES(${KsdkDir})
    INCLUDE_DIRECTORIES(${IncludeDir})
ELSEIF(CMAKE_BUILD_TYPE MATCHES Release)
    INCLUDE_DIRECTORIES(${KsdkDir}/platform/osa/inc)
    INCLUDE_DIRECTORIES(${KsdkDir}/platform/utilities/inc)
    INCLUDE_DIRECTORIES(${KsdkDir}/platform/CMSIS/Include)
    INCLUDE_DIRECTORIES(${KsdkDir}/platform/devices)
    INCLUDE_DIRECTORIES(${KsdkDir}/platform/devices/MKL03Z4/include)
    INCLUDE_DIRECTORIES(${KsdkDir}/platform/devices/MKL03Z4/startup)
    INCLUDE_DIRECTORIES(${KsdkDir}/platform/hal/inc)
    INCLUDE_DIRECTORIES(${KsdkDir}/platform/drivers/inc)
    INCLUDE_DIRECTORIES(${KsdkDir}/platform/system/inc)
    INCLUDE_DIRECTORIES(${KsdkDir}/platform/drivers/src/gpio)
    INCLUDE_DIRECTORIES(${KsdkDir}/examples)
    INCLUDE_DIRECTORIES(${KsdkDir}/examples/frdmkl03z)
    INCLUDE_DIRECTORIES(${KsdkDir})
    INCLUDE_DIRECTORIES(${IncludeDir})
ENDIF()

# ADD_EXECUTABLE
ADD_EXECUTABLE(uhk-left
    "${KsdkDir}/platform/utilities/src/fsl_misc_utilities.c"
    "${KsdkDir}/platform/devices/MKL03Z4/startup/gcc/startup_MKL03Z4.S"
    "${SrcDir}/main.c"
    "${KsdkDir}/examples/frdmkl03z/gpio_pins.c"
    "${KsdkDir}/examples/frdmkl03z/gpio_pins.h"
    "${KsdkDir}/examples/frdmkl03z/pin_mux.c"
    "${KsdkDir}/examples/frdmkl03z/pin_mux.h"
    "${KsdkDir}/examples/frdmkl03z/board.c"
    "${KsdkDir}/examples/frdmkl03z/board.h"
    "${KsdkDir}/platform/utilities/src/fsl_debug_console.c"
    "${KsdkDir}/platform/utilities/inc/fsl_debug_console.h"
    "${KsdkDir}/platform/utilities/src/print_scan.c"
    "${KsdkDir}/platform/utilities/src/print_scan.h"
    "${KsdkDir}/platform/devices/MKL03Z4/startup/system_MKL03Z4.c"
    "${KsdkDir}/platform/devices/startup.c"
    "${KsdkDir}/platform/devices/startup.h"
)
SET_TARGET_PROPERTIES(uhk-left PROPERTIES OUTPUT_NAME "uhk-left.elf")

TARGET_LINK_LIBRARIES(uhk-left -Wl,--start-group)
# LIBRARIES
IF(CMAKE_BUILD_TYPE MATCHES Debug)
    TARGET_LINK_LIBRARIES(uhk-left ${KsdkDir}/lib/ksdk_platform_lib/armgcc/KL03Z4/debug/libksdk_platform.a)
ELSEIF(CMAKE_BUILD_TYPE MATCHES Release)
    TARGET_LINK_LIBRARIES(uhk-left ${KsdkDir}/lib/ksdk_platform_lib/armgcc/KL03Z4/release/libksdk_platform.a)
ENDIF()

# SYSTEM LIBRARIES
TARGET_LINK_LIBRARIES(uhk-left m)
TARGET_LINK_LIBRARIES(uhk-left c)
TARGET_LINK_LIBRARIES(uhk-left gcc)
TARGET_LINK_LIBRARIES(uhk-left nosys)
TARGET_LINK_LIBRARIES(uhk-left -Wl,--end-group)

# MAP FILE
SET(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG}  -Xlinker -Map=debug/uhk-left.map")
SET(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE}  -Xlinker -Map=release/uhk-left.map")

# BIN AND HEX
ADD_CUSTOM_COMMAND(TARGET uhk-left POST_BUILD COMMAND ${CMAKE_OBJCOPY} -Oihex ${EXECUTABLE_OUTPUT_PATH}/uhk-left.elf ${EXECUTABLE_OUTPUT_PATH}/uhk-left.hex)
ADD_CUSTOM_COMMAND(TARGET uhk-left POST_BUILD COMMAND ${CMAKE_OBJCOPY} -Obinary ${EXECUTABLE_OUTPUT_PATH}/uhk-left.elf ${EXECUTABLE_OUTPUT_PATH}/uhk-left.bin)
