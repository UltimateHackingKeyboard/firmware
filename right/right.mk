# Copyright (C) 2018 Kristian Lauszus. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# Contact information
# -------------------
# Kristian Lauszus
# Web      :  http://www.lauszus.com
# e-mail   :  lauszus@gmail.com

ifndef DEVICE_ID
$(error DEVICE_ID is not set)
endif

ifndef PROJECT_NAME
$(error PROJECT_NAME is not set)
endif

BUILD_DIR = build_make/$(PROJECT_NAME)

# Defines the part type that this project uses.
PART = MK22FN512VLH12

# Defines the linker script to use for the application.
LDSCRIPT = src/link/MK22FN512xxx12_flash.ld

# Size of the heap and stack.
HEAP_SIZE = 0x2000
STACK_SIZE = 0x0400

# Set the compiler CPU and FPU options.
CPU = -mcpu=cortex-m4
FPU = -mfpu=fpv4-sp-d16 -mfloat-abi=hard

# Command for flashing the right half of the keyboard.
FLASH_CMD = node ../lib/agent/packages/usb/update-device-firmware.js $(PROJECT_OBJ:.axf=.hex)

# Path to the JLink script used for the right half.
JLINK_SCRIPT = ../scripts/flash-right.jlink

# Preprocessor directives.
BUILD_FLAGS = -DCPU_$(PART)_cm4 -DUSB_STACK_BM -DBL_HAS_BOOTLOADER_CONFIG=1

# Address of the app vector table. The bootloader will take up the flash before this address.
BL_APP_VECTOR_TABLE_ADDRESS ?= 0xc000

# Source files.
SOURCE = $(wildcard src/*.c) \
         $(wildcard src/*/*.c) \
         $(wildcard src/*/*/*.c) \
         ../lib/bootloader/src/bootloader/src/wormhole.c \
         $(wildcard ../lib/KSDK_2.0_MK22FN512xxx12/middleware/usb_1.0.0/device/*.c) \
         $(wildcard ../lib/KSDK_2.0_MK22FN512xxx12/middleware/usb_1.0.0/osa/*.c) \
         ../lib/KSDK_2.0_MK22FN512xxx12/devices/MK22F51212/system_MK22F51212.c \
         ../lib/KSDK_2.0_MK22FN512xxx12/devices/MK22F51212/gcc/startup_MK22F51212.S \
         ../lib/KSDK_2.0_MK22FN512xxx12/devices/MK22F51212/drivers/fsl_adc16.c \
         ../lib/KSDK_2.0_MK22FN512xxx12/devices/MK22F51212/drivers/fsl_clock.c \
         ../lib/KSDK_2.0_MK22FN512xxx12/devices/MK22F51212/drivers/fsl_ftm.c \
         ../lib/KSDK_2.0_MK22FN512xxx12/devices/MK22F51212/drivers/fsl_gpio.c \
         ../lib/KSDK_2.0_MK22FN512xxx12/devices/MK22F51212/drivers/fsl_i2c.c \
         ../lib/KSDK_2.0_MK22FN512xxx12/devices/MK22F51212/drivers/fsl_pit.c \
         ../lib/KSDK_2.0_MK22FN512xxx12/devices/MK22F51212/drivers/fsl_smc.c \
         $(wildcard ../shared/*.c)

# Header files.
IPATH = src \
        src/ksdk_usb \
        src/buspal \
        src/buspal/bm_usb \
        ../lib/bootloader/src \
        ../lib/KSDK_2.0_MK22FN512xxx12/middleware/usb_1.0.0/device \
        ../lib/KSDK_2.0_MK22FN512xxx12/middleware/usb_1.0.0/include \
        ../lib/KSDK_2.0_MK22FN512xxx12/middleware/usb_1.0.0/osa \
        ../lib/KSDK_2.0_MK22FN512xxx12/CMSIS/Include \
        ../lib/KSDK_2.0_MK22FN512xxx12/devices/MK22F51212 \
        ../lib/KSDK_2.0_MK22FN512xxx12/devices/MK22F51212/drivers \
        ../shared

# Include main Makefile.
include ../scripts/Makedefs.mk
