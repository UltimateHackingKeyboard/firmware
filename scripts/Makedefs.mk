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

# Debug using Semihosting.
SEMIHOSTING ?= 0

# Build directory.
BUILD_DIR ?= build_make

# Set default value (no bootloader) for the bootloader vector table address.
BL_APP_VECTOR_TABLE_ADDRESS ?= 0

# Preprocessor directives.
BUILD_FLAGS += -D__NEWLIB__ -D__USE_CMSIS -D__MCUXPRESSO -DCPU_$(PART) -D__STARTUP_CLEAR_BSS -DBL_APP_VECTOR_TABLE_ADDRESS=$(BL_APP_VECTOR_TABLE_ADDRESS)

# Path to project object file.
PROJECT_OBJ = $(BUILD_DIR)/$(PROJECT_NAME).axf

# Set the prefix for the tools to use.
PREFIX ?= arm-none-eabi

# Set WIN=1 manually for Windows Subsystem for Linux.
WIN ?= 0

# Determine if we are on a Windows machine and set the .exe suffix.
UNAME_S := $(shell uname -s)
ifeq ($(OS),Windows_NT) # Native Windows.
    WIN = 1
endif

# Set J-Link executable.
ifeq ($(WIN),1)
    JLINK := JLink.exe
else
    JLINK := JLinkExe
endif

# We can not use the suffix command, as the PREFIX might contain spaces.
ifeq ($(WIN),1)
    SUFFIX := .exe
endif

# The command for calling the compilers.
CC = $(PREFIX)-gcc$(SUFFIX)
CXX = $(PREFIX)-g++$(SUFFIX)

# The command for calling the linker.
LD = $(PREFIX)-g++$(SUFFIX)

# The command for extracting images from the linked executables.
OBJCOPY = $(PREFIX)-objcopy$(SUFFIX)

# The command for the size tool.
SIZE = $(PREFIX)-size$(SUFFIX)

# Auto-dependency generation flags.
DEPS = -MMD -MP

# The flags passed to the assembler.
AFLAGS = -mthumb                    \
         $(CPU)                     \
         $(FPU)                     \
         $(DEPS)                    \
         $(BUILD_FLAGS)             \
         -x assembler-with-cpp

# The flags passed to the compiler.
CFLAGS = -mthumb                    \
         $(CPU)                     \
         $(FPU)                     \
         $(DEPS)                    \
         -DDEVICE_ID=$(DEVICE_ID)   \
         -fno-builtin               \
         -ffunction-sections        \
         -fdata-sections            \
         -fno-common                \
         -Wdouble-promotion         \
         -Woverflow                 \
         -Wall                      \
         -Wextra                    \
         -Wno-unused-parameter      \
         -Wno-type-limits           \
         -Wshadow                   \
         -flto                      \
         $(BUILD_FLAGS)

# Compiler options for C++ only.
CXXFLAGS = -felide-constructors -fno-exceptions -fno-rtti

# Set the C/C++ standard to use.
CSTD = -std=gnu11
CXXSTD = -std=gnu++14

# Make all warnings into errors when building using Travis CI.
ifdef TRAVIS
    CFLAGS += -Werror
endif

# The flags passed to the linker.
LDFLAGS = --specs=nano.specs -mthumb $(CPU) $(FPU) -T $(LDSCRIPT) -Wl,-Map=$(PROJECT_OBJ:.axf=.map),--gc-sections,-print-memory-usage,-no-wchar-size-warning,--defsym=__heap_size__=$(HEAP_SIZE),--defsym=__stack_size__=$(STACK_SIZE),--defsym=__bl_app_vector_table_address__=$(BL_APP_VECTOR_TABLE_ADDRESS)

# Include the following archives.
LDARCHIVES = -Wl,--start-group -lg -lgcc -lm -Wl,--end-group

# Add flags to the build flags and linker depending on the build settings.
ifeq ($(SEMIHOSTING),1)
    # Include Semihost library.
    LDARCHIVES += -lcr_newlib_semihost

    # Enable printf floating numbers.
    LDFLAGS += -u _printf_float
else
    # Include libnosys.a if Semihosting is disabled.
    LDARCHIVES += -lnosys
endif

# Check if the DEBUG environment variable is set.
DEBUG ?= 0
ifeq ($(DEBUG),1)
    CFLAGS += -Os -g3 -DDEBUG
else
    CFLAGS += -Os -DNDEBUG
endif

# Add the include file paths to AFLAGS and CFLAGS.
AFLAGS += $(patsubst %,-I%,$(subst :, ,$(IPATH)))
CFLAGS += $(patsubst %,-I%,$(subst :, ,$(IPATH)))

# Create lists of C, C++ and assembly objects.
C_OBJS := $(addsuffix .o,$(addprefix $(BUILD_DIR)/,$(basename $(filter %.c,$(abspath $(SOURCE))))))
CPP_OBJS := $(addsuffix .o,$(addprefix $(BUILD_DIR)/,$(basename $(filter %.cpp,$(abspath $(SOURCE))))))
S_OBJS := $(addsuffix .o,$(addprefix $(BUILD_DIR)/,$(basename $(filter %.S,$(abspath $(SOURCE))))))

# Create a list of all objects.
OBJS := $(C_OBJS) $(CPP_OBJS) $(S_OBJS)

# Define the commands used for compiling the project.
LD_CMD = $(LD) $(LDFLAGS) -o $(@) $(filter %.o %.a, $(^)) $(LDARCHIVES)
CC_CMD = $(CC) $(CFLAGS) $(CSTD) -c $(<) -o $(@)
CXX_CMD = $(CXX) $(CFLAGS) $(CXXFLAGS) $(CXXSTD) -c $(<) -o $(@)
AS_CMD = $(CC) $(AFLAGS) $(CSTD) -c $(<) -o $(@)

# Everyone need colors in their life!
INTERACTIVE := $(shell [ -t 0 ] && echo 1)
ifeq ($(INTERACTIVE),1)
    color_default = \033[0m
    color_bold = \033[01m
    color_red = \033[31m
    color_green = \033[32m
    color_yellow = \033[33m
    color_blue = \033[34m
    color_magenta = \033[35m
    color_cyan = \033[36m
    color_orange = \033[38;5;172m
    color_light_blue = \033[38;5;039m
    color_gray = \033[38;5;008m
    color_purple = \033[38;5;097m
endif

.PHONY: all clean flash flash-jlink

# The default rule, which causes the project to be built.
all: $(PROJECT_OBJ)

# The rule to clean out all the build products.
clean:
	@rm -rf $(BUILD_DIR) $(wildcard *~)
	@echo "$(color_red)Done cleaning!$(color_default)"

flash: all
	@$(FLASH_CMD) || exit 1

flash-jlink: all
	$(JLINK) -if SWD -CommandFile $(JLINK_SCRIPT) || exit 1

# Rebuild all objects when the Makefiles change.
$(OBJS): $(MAKEFILE_LIST)

# The rule for linking the application.
$(PROJECT_OBJ): $(OBJS) $(LDSCRIPT)
	@if [ '$(VERBOSE)' = 1 ]; then                               \
	     echo $(LD_CMD);                                         \
	 else                                                        \
	     echo "    $(color_purple)LD$(color_default)    $(@F)";  \
	 fi
	@echo
	@$(LD_CMD)
	@echo
	@if [ '$(VERBOSE)' = 1 ]; then                               \
	     $(SIZE) -Ax $(@);                                       \
	 fi
	@$(OBJCOPY) -O binary $(@) $(@:.axf=.bin)
	@$(OBJCOPY) -O ihex $(@) $(@:.axf=.hex)

# The rule for building the object files from each source file.
$(C_OBJS): $(BUILD_DIR)/%.o : %.c
	@mkdir -p $(@D)
	@if [ '$(VERBOSE)' = 1 ]; then                               \
	     echo $(CC_CMD);                                         \
	 else                                                        \
	     echo "    $(color_green)CC$(color_default)    $(<F)";   \
	 fi
	@$(CC_CMD)

$(CPP_OBJS): $(BUILD_DIR)/%.o : %.cpp
	@mkdir -p $(@D)
	@if [ '$(VERBOSE)' = 1 ]; then                               \
	     echo $(CXX_CMD);                                        \
	 else                                                        \
	     echo "    $(color_cyan)CXX$(color_default)   $(<F)";    \
	 fi
	@$(CXX_CMD)

$(S_OBJS): $(BUILD_DIR)/%.o: %.S
	@mkdir -p $(@D)
	@if [ '$(VERBOSE)' = 1 ]; then                               \
	     echo $(AS_CMD);                                         \
	 else                                                        \
	     echo "    $(color_magenta)AS$(color_default)    $(<F)"; \
	 fi
	@$(AS_CMD)

# Include the automatically generated dependency files.
-include $(OBJS:.o=.d)
