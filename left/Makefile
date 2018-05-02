# Copyright (C) 2018 Kristian Lauszus. All rights reserved.
#
# This software may be distributed and modified under the terms of the GNU
# General Public License version 2 (GPL2) as published by the Free Software
# Foundation and appearing in the file GPL2.TXT included in the packaging of
# this file. Please note that GPL2 Section 2[b] requires that all works based
# on this software must also be made publicly available under the terms of
# the GPL2 ("Copyleft").
#
# Contact information
# -------------------
# Kristian Lauszus
# Web      :  http://www.lauszus.com
# e-mail   :  lauszus@gmail.com

# Set project name.
PROJECT_NAME = uhk_left

# Defines the part type that this project uses.
PART = MKL03Z32VFK4

# Defines the linker script to use for the application.
LDSCRIPT = ../lib/KSDK_2.0_MKL03Z8xxx4/devices/MKL03Z4/gcc/MKL03Z32xxx4_flash.ld

# Size of the heap and stack.
HEAP_SIZE = 0
STACK_SIZE = 0x0050

# Set the compiler CPU and FPU options.
CPU = -mcpu=cortex-m0plus
FPU = -mfloat-abi=soft

# Command for flashing the left half of the keyboard.
FLASH_CMD = node ../lib/agent/packages/usb/update-module-firmware.js leftHalf $(PROJECT_OBJ:.axf=.bin)

# Source files.
SOURCE = $(wildcard src/*.c) \
         ../lib/KSDK_2.0_MKL03Z8xxx4/devices/MKL03Z4/system_MKL03Z4.c \
         ../lib/KSDK_2.0_MKL03Z8xxx4/devices/MKL03Z4/gcc/startup_MKL03Z4.S \
         ../lib/KSDK_2.0_MKL03Z8xxx4/devices/MKL03Z4/drivers/fsl_clock.c \
         ../lib/KSDK_2.0_MKL03Z8xxx4/devices/MKL03Z4/drivers/fsl_gpio.c \
         ../lib/KSDK_2.0_MKL03Z8xxx4/devices/MKL03Z4/drivers/fsl_i2c.c \
         ../lib/KSDK_2.0_MKL03Z8xxx4/devices/MKL03Z4/drivers/fsl_lptmr.c \
         ../lib/KSDK_2.0_MKL03Z8xxx4/devices/MKL03Z4/drivers/fsl_smc.c \
         ../lib/KSDK_2.0_MKL03Z8xxx4/devices/MKL03Z4/drivers/fsl_spi.c \
         ../lib/KSDK_2.0_MKL03Z8xxx4/devices/MKL03Z4/drivers/fsl_tpm.c \
         $(wildcard ../shared/*.c)

# Header files.
IPATH = src \
        ../lib/KSDK_2.0_MKL03Z8xxx4/CMSIS/Include \
        ../lib/KSDK_2.0_MKL03Z8xxx4/devices/MKL03Z4 \
        ../lib/KSDK_2.0_MKL03Z8xxx4/devices/MKL03Z4/drivers \
        ../shared

# Include main Makefile.
include ../scripts/Makedefs.mk
