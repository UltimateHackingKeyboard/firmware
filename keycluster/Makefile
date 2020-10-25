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

# Set project name.
PROJECT_NAME = uhk_keycluster

# Defines the part type that this project uses.
PART = MKL17Z32VFM4

# Defines the linker script to use for the application.
LDSCRIPT = ../lib/SDK_2.8.0_MKL17Z32xxx4/devices/MKL17Z644/gcc/MKL17Z32xxx4_flash.ld

# Size of the heap and stack.
HEAP_SIZE = 0
STACK_SIZE = 0x0050

# Set the compiler CPU and FPU options.
CPU = -mcpu=cortex-m0plus
FPU = -mfloat-abi=soft

# Command for flashing the key cluster module.
FLASH_CMD = node ../lib/agent/packages/usb/update-module-firmware.js leftModule $(PROJECT_OBJ:.axf=.bin)

# Path to the JLink script used for the key cluster module.
JLINK_SCRIPT = ../scripts/flash-keycluster.jlink

# Source files.
SOURCE = $(wildcard src/*.c) \
         ../lib/SDK_2.8.0_MKL17Z32xxx4/devices/MKL17Z644/system_MKL17Z644.c \
         ../lib/SDK_2.8.0_MKL17Z32xxx4/devices/MKL17Z644/gcc/startup_MKL17Z644.S \
         ../lib/SDK_2.8.0_MKL17Z32xxx4/devices/MKL17Z644/drivers/fsl_clock.c \
         ../lib/SDK_2.8.0_MKL17Z32xxx4/devices/MKL17Z644/drivers/fsl_gpio.c \
         ../lib/SDK_2.8.0_MKL17Z32xxx4/devices/MKL17Z644/drivers/fsl_i2c.c \
         ../lib/SDK_2.8.0_MKL17Z32xxx4/devices/MKL17Z644/drivers/fsl_lptmr.c \
         ../lib/SDK_2.8.0_MKL17Z32xxx4/devices/MKL17Z644/drivers/fsl_spi.c \
         ../lib/SDK_2.8.0_MKL17Z32xxx4/devices/MKL17Z644/drivers/fsl_tpm.c \
         $(wildcard ../shared/*.c) \
         $(wildcard ../shared/module/*.c)

# Header files.
IPATH = src \
        ../lib/SDK_2.8.0_MKL17Z32xxx4/CMSIS/Include \
        ../lib/SDK_2.8.0_MKL17Z32xxx4/devices/MKL17Z644 \
        ../lib/SDK_2.8.0_MKL17Z32xxx4/devices/MKL17Z644/drivers \
        ../shared

# Include main Makefile.
include ../scripts/Makedefs.mk
