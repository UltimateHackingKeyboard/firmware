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

# KSDK DIRECTORY
SET(KsdkDir $ENV{KSDK_DIR})

# CURRENT DIRECTORY
SET(ProjDirPath ${CMAKE_CURRENT_SOURCE_DIR})

# DEBUG LINK FILE
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -T${KsdkDir}/platform/devices/MK22F51212/linker/gcc/MK22FN512xxx12_flash.ld  -static")

# RELEASE LINK FILE
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -T${KsdkDir}/platform/devices/MK22F51212/linker/gcc/MK22FN512xxx12_flash.ld  -static")

# DEBUG ASM FLAGS
SET(CMAKE_ASM_FLAGS_DEBUG "${CMAKE_ASM_FLAGS_DEBUG} -g  -mcpu=cortex-m4  -mfloat-abi=hard  -mfpu=fpv4-sp-d16  -mthumb  -Wall  -fno-common  -ffunction-sections  -fdata-sections  -ffreestanding  -fno-builtin  -mapcs  -std=gnu99")

# DEBUG C FLAGS
SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g  -O0  -Wno-format  -fno-strict-aliasing  -mcpu=cortex-m4  -mfloat-abi=hard  -mfpu=fpv4-sp-d16  -mthumb  -MMD  -MP  -Wall  -fno-common  -ffunction-sections  -fdata-sections  -ffreestanding  -fno-builtin  -mapcs  -std=gnu99")

# DEBUG LD FLAGS
SET(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -g  -Xlinker --defsym=__ram_vector_table__=1  -mcpu=cortex-m4  -mfloat-abi=hard  -mfpu=fpv4-sp-d16  --specs=nano.specs  -lm  -Wall  -fno-common  -ffunction-sections  -fdata-sections  -ffreestanding  -fno-builtin  -mthumb  -mapcs  -Xlinker --gc-sections  -Xlinker -static  -Xlinker -z  -Xlinker muldefs  -Xlinker --defsym=__stack_size__=0x1000  -Xlinker --defsym=__heap_size__=0x0400")

# RELEASE ASM FLAGS
SET(CMAKE_ASM_FLAGS_RELEASE "${CMAKE_ASM_FLAGS_RELEASE} -mcpu=cortex-m4  -mfloat-abi=hard  -mfpu=fpv4-sp-d16  -mthumb  -Wall  -fno-common  -ffunction-sections  -fdata-sections  -ffreestanding  -fno-builtin  -mapcs  -std=gnu99")

# RELEASE C FLAGS
SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Os  -Wno-format  -fno-strict-aliasing  -mcpu=cortex-m4  -mfloat-abi=hard  -mfpu=fpv4-sp-d16  -mthumb  -MMD  -MP  -Wall  -fno-common  -ffunction-sections  -fdata-sections  -ffreestanding  -fno-builtin  -mapcs  -std=gnu99")

# RELEASE LD FLAGS
SET(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -Xlinker --defsym=__ram_vector_table__=1  -mcpu=cortex-m4  -mfloat-abi=hard  -mfpu=fpv4-sp-d16  --specs=nano.specs  -lm  -Wall  -fno-common  -ffunction-sections  -fdata-sections  -ffreestanding  -fno-builtin  -mthumb  -mapcs  -Xlinker --gc-sections  -Xlinker -static  -Xlinker -z  -Xlinker muldefs  -Xlinker --defsym=__stack_size__=0x1000  -Xlinker --defsym=__heap_size__=0x0400")

# ASM MACRO
SET(CMAKE_ASM_FLAGS_DEBUG "${CMAKE_ASM_FLAGS_DEBUG}  -DDEBUG")

# C MACRO
SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}  -D_DEBUG=1")
SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}  -DCPU_MK22FN512VLH12")
SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}  -DFRDM_K22F")
SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}  -DFREEDOM")
SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}  -D_DEBUG=0")
SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}  -DCPU_MK22FN512VLH12")
SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}  -DFRDM_K22F")
SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}  -DFREEDOM")

# CXX MACRO

# INCLUDE_DIRECTORIES
IF(CMAKE_BUILD_TYPE MATCHES Debug)
    INCLUDE_DIRECTORIES(${KsdkDir}/platform/osa/inc)
    INCLUDE_DIRECTORIES(${KsdkDir}/rtos)
    INCLUDE_DIRECTORIES(${KsdkDir}/platform/utilities/inc)
    INCLUDE_DIRECTORIES(${KsdkDir}/platform/CMSIS/Include)
    INCLUDE_DIRECTORIES(${KsdkDir}/platform/devices)
    INCLUDE_DIRECTORIES(${KsdkDir}/platform/devices/MK22F51212/include)
    INCLUDE_DIRECTORIES(${KsdkDir}/platform/devices/MK22F51212/startup)
    INCLUDE_DIRECTORIES(${KsdkDir}/examples/frdmk22f)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/adapter/sources)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/adapter/sources/sdk)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/usb_core/device/include/MK22F51212)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/usb_core/device/include)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/usb_core/device/sources/classes/audio)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/usb_core/device/sources/classes/cdc)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/usb_core/device/sources/classes/common)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/usb_core/device/sources/classes/composite)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/usb_core/device/sources/classes/hid)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/usb_core/device/sources/classes/include)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/usb_core/device/sources/classes/include/config)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/usb_core/device/sources/classes/msd)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/usb_core/device/sources/classes/phdc)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/usb_core/device/sources/classes/video)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/usb_core/device/sources/controller/khci)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/usb_core/device/sources/controller)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/usb_core/hal)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/usb_core/host/include/MK22F51212)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/usb_core/host/include)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/usb_core/host/sources/classes/audio)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/usb_core/host/sources/classes/cdc)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/usb_core/host/sources/classes/hid)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/usb_core/host/sources/classes/hub)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/usb_core/host/sources/classes/msd)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/usb_core/host/sources/classes/phdc)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/usb_core/host/sources/classes/printer)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/usb_core/host/sources/controller/khci)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/usb_core/host/sources/controller)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/usb_core/include)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/usb_core/otg/sources/driver/khci)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/usb_core/otg/sources/driver/max3353/sdk)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/usb_core/otg/include)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/usb_core/otg/sources/otg)
    INCLUDE_DIRECTORIES(${KsdkDir}/platform/drivers/inc)
    INCLUDE_DIRECTORIES(${KsdkDir}/platform/drivers/src/uart)
    INCLUDE_DIRECTORIES(${KsdkDir}/platform/drivers/src/gpio)
    INCLUDE_DIRECTORIES(${KsdkDir}/platform/drivers/src/pit)
    INCLUDE_DIRECTORIES(${KsdkDir}/platform/drivers/src/edma)
    INCLUDE_DIRECTORIES(${KsdkDir}/platform/hal/inc)
    INCLUDE_DIRECTORIES(${KsdkDir}/platform/system/inc)
ELSEIF(CMAKE_BUILD_TYPE MATCHES Release)
    INCLUDE_DIRECTORIES(${KsdkDir}/platform/osa/inc)
    INCLUDE_DIRECTORIES(${KsdkDir}/rtos)
    INCLUDE_DIRECTORIES(${KsdkDir}/platform/utilities/inc)
    INCLUDE_DIRECTORIES(${KsdkDir}/platform/CMSIS/Include)
    INCLUDE_DIRECTORIES(${KsdkDir}/platform/devices)
    INCLUDE_DIRECTORIES(${KsdkDir}/platform/devices/MK22F51212/include)
    INCLUDE_DIRECTORIES(${KsdkDir}/platform/devices/MK22F51212/startup)
    INCLUDE_DIRECTORIES(${KsdkDir}/examples/frdmk22f)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/adapter/sources)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/adapter/sources/sdk)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/usb_core/device/include/MK22F51212)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/usb_core/device/include)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/usb_core/device/sources/classes/audio)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/usb_core/device/sources/classes/cdc)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/usb_core/device/sources/classes/common)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/usb_core/device/sources/classes/composite)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/usb_core/device/sources/classes/hid)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/usb_core/device/sources/classes/include)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/usb_core/device/sources/classes/include/config)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/usb_core/device/sources/classes/msd)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/usb_core/device/sources/classes/phdc)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/usb_core/device/sources/classes/video)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/usb_core/device/sources/controller/khci)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/usb_core/device/sources/controller)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/usb_core/hal)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/usb_core/host/include/MK22F51212)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/usb_core/host/include)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/usb_core/host/sources/classes/audio)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/usb_core/host/sources/classes/cdc)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/usb_core/host/sources/classes/hid)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/usb_core/host/sources/classes/hub)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/usb_core/host/sources/classes/msd)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/usb_core/host/sources/classes/phdc)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/usb_core/host/sources/classes/printer)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/usb_core/host/sources/controller/khci)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/usb_core/host/sources/controller)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/usb_core/include)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/usb_core/otg/sources/driver/khci)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/usb_core/otg/sources/driver/max3353/sdk)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/usb_core/otg/include)
    INCLUDE_DIRECTORIES(${KsdkDir}/usb/usb_core/otg/sources/otg)
    INCLUDE_DIRECTORIES(${KsdkDir}/platform/drivers/inc)
    INCLUDE_DIRECTORIES(${KsdkDir}/platform/drivers/src/uart)
    INCLUDE_DIRECTORIES(${KsdkDir}/platform/drivers/src/gpio)
    INCLUDE_DIRECTORIES(${KsdkDir}/platform/drivers/src/pit)
    INCLUDE_DIRECTORIES(${KsdkDir}/platform/drivers/src/edma)
    INCLUDE_DIRECTORIES(${KsdkDir}/platform/hal/inc)
    INCLUDE_DIRECTORIES(${KsdkDir}/platform/system/inc)
ENDIF()

# ADD_EXECUTABLE
ADD_EXECUTABLE(uhk-right
    "${KsdkDir}/platform/utilities/src/fsl_misc_utilities.c"
    "${KsdkDir}/platform/devices/MK22F51212/startup/gcc/startup_MK22F51212.S"
    "${ProjDirPath}/../../audio_data.c"
    "${ProjDirPath}/../../audio_generator.c"
    "${ProjDirPath}/../../audio_generator.h"
    "${ProjDirPath}/../../composite_app.c"
    "${ProjDirPath}/../../composite_app.h"
    "${ProjDirPath}/../../mouse.c"
    "${ProjDirPath}/../../mouse.h"
    "${ProjDirPath}/../../usb_descriptor.c"
    "${ProjDirPath}/../../usb_descriptor.h"
    "${ProjDirPath}/../../usb_request.c"
    "${ProjDirPath}/../../usb_request.h"
    "${ProjDirPath}/../../hardware_init.c"
    "${KsdkDir}/examples/frdmk22f/gpio_pins.c"
    "${KsdkDir}/examples/frdmk22f/gpio_pins.h"
    "${KsdkDir}/examples/frdmk22f/pin_mux.c"
    "${KsdkDir}/examples/frdmk22f/pin_mux.h"
    "${KsdkDir}/examples/frdmk22f/board.h"
    "${KsdkDir}/examples/frdmk22f/board.c"
    "${KsdkDir}/platform/utilities/src/fsl_debug_console.c"
    "${KsdkDir}/platform/utilities/inc/fsl_debug_console.h"
    "${KsdkDir}/platform/utilities/src/print_scan.c"
    "${KsdkDir}/platform/utilities/src/print_scan.h"
    "${KsdkDir}/platform/devices/MK22F51212/startup/system_MK22F51212.c"
    "${KsdkDir}/platform/devices/startup.c"
    "${KsdkDir}/platform/devices/startup.h"
)
SET_TARGET_PROPERTIES(uhk-right PROPERTIES OUTPUT_NAME "uhk-right.elf")

TARGET_LINK_LIBRARIES(uhk-right -Wl,--start-group)
# LIBRARIES
IF(CMAKE_BUILD_TYPE MATCHES Debug)
    TARGET_LINK_LIBRARIES(uhk-right ${KsdkDir}/usb/usb_core/device/lib/bm/armgcc/MK22F51212/debug/libusbd_bm.a)
    TARGET_LINK_LIBRARIES(uhk-right ${KsdkDir}/lib/ksdk_platform_lib/armgcc/K22F51212/debug/libksdk_platform.a)
ELSEIF(CMAKE_BUILD_TYPE MATCHES Release)
    TARGET_LINK_LIBRARIES(uhk-right ${KsdkDir}/usb/usb_core/device/lib/bm/armgcc/MK22F51212/release/libusbd_bm.a)
    TARGET_LINK_LIBRARIES(uhk-right ${KsdkDir}/lib/ksdk_platform_lib/armgcc/K22F51212/release/libksdk_platform.a)
ENDIF()

# SYSTEM LIBRARIES
TARGET_LINK_LIBRARIES(uhk-right m)
TARGET_LINK_LIBRARIES(uhk-right c)
TARGET_LINK_LIBRARIES(uhk-right gcc)
TARGET_LINK_LIBRARIES(uhk-right nosys)
TARGET_LINK_LIBRARIES(uhk-right -Wl,--end-group)

# MAP FILE
SET(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG}  -Xlinker -Map=debug/uhk-right.map")
SET(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE}  -Xlinker -Map=release/uhk-right.map")

# BIN AND HEX
ADD_CUSTOM_COMMAND(TARGET uhk-right POST_BUILD COMMAND ${CMAKE_OBJCOPY} -Oihex ${EXECUTABLE_OUTPUT_PATH}/uhk-right.elf ${EXECUTABLE_OUTPUT_PATH}/uhk-right.hex)
ADD_CUSTOM_COMMAND(TARGET uhk-right POST_BUILD COMMAND ${CMAKE_OBJCOPY} -Obinary ${EXECUTABLE_OUTPUT_PATH}/uhk-right.elf ${EXECUTABLE_OUTPUT_PATH}/uhk-right.bin)
