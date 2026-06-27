# Makefile — TremorSense firmware build
# TremorSense — Wearable Tremor Characterization Band
#
# Author: jayis1
# Copyright (c) 2026 jayis1. All rights reserved.
# SPDX-License-Identifier: MIT
#
# Build system for nRF5340 (Cortex-M33) firmware.
# Requires: arm-none-eabi-gcc toolchain, GNU make.
#
# Usage:
#   make            — build firmware (tremorsense.elf)
#   make flash      — flash via J-Link (requires nRF Command Line Tools)
#   make clean      — remove build artifacts
#   make size       — show code size breakdown
#   make dump       — generate disassembly
#   make hex        — generate .hex file for DFU

# ---- Toolchain ----
TARGET      = tremorsense
CROSS       = arm-none-eabi-
CC          = $(CROSS)gcc
OBJCOPY     = $(CROSS)objcopy
OBJDUMP     = $(CROSS)objdump
SIZE        = $(CROSS)size
AR          = $(CROSS)ar
RANLIB      = $(CROSS)ranlib

# ---- Build directories ----
BUILD_DIR   = build
SRCDIR      = .

# ---- MCU configuration ----
MCU         = cortex-m33
FPU         = fpv5-sp-d16
FLOAT_ABI   = hard
ARCH_FLAGS  = -mcpu=$(MCU) -mthumb -mfloat-abi=$(FLOAT_ABI) -mfpu=$(FPU)

# ---- Compiler flags ----
CFLAGS      = $(ARCH_FLAGS) -std=c11 -g3 -Wall -Wextra -Werror \
              -Os -ffunction-sections -fdata-sections \
              -fno-common -fno-builtin -ffreestanding \
              -I$(SRCDIR) -I$(SRCDIR)/drivers \
              -DBOARD_TREMORSENSE=1 \
              -DVERSION=\"1.0.0\" \
              -DAUTHOR=\"jayis1\"

# Include paths for Zephyr (optional, if using Zephyr RTOS)
# ZEPHYR_BASE ?= /opt/zephyrproject/zephyr
# CFLAGS += -I$(ZEPHYR_BASE)/include

# ---- Linker flags ----
LDFLAGS     = $(ARCH_FLAGS) -T linker.ld -Wl,--gc-sections \
              -Wl,-Map=$(BUILD_DIR)/$(TARGET).map \
              -Wl,--print-memory-usage \
              -nostdlib -lc -lnosys -lm

# ---- Source files ----
SRCS        = main.c \
              drivers/imu_drv.c \
              drivers/dsp.c \
              drivers/ml_model.c \
              drivers/flash_drv.c \
              drivers/ble_drv.c \
              drivers/display_drv.c \
              drivers/power_drv.c \
              drivers/env_drv.c

OBJS        = $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRCS))

# ---- Assembly files (startup) ----
ASRCS       = # startup_nrf5340.s (if using bare-metal; Zephyr provides its own)
AOBJS       = $(patsubst %.s,$(BUILD_DIR)/%.o,$(ASRCS))

# ---- Targets ----
.PHONY: all clean flash size hex dump

all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex

$(BUILD_DIR)/$(TARGET).elf: $(OBJS) $(AOBJS) linker.ld | $(BUILD_DIR)
	$(CC) $(ARCH_FLAGS) $(OBJS) $(AOBJS) $(LDFLAGS) -o $@

$(BUILD_DIR)/$(TARGET).hex: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

# ---- Compile rules ----
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.s
	@mkdir -p $(dir $@)
	$(CC) $(ARCH_FLAGS) -x assembler-with-cpp -c $< -o $@

# ---- Build directory ----
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# ---- Flash via J-Link ----
JLINK      = JLinkExe
JLINK_DEV  = nRF5340_xxAA_APP
flash: $(BUILD_DIR)/$(TARGET).hex
	@echo "connect\n$(JLINK_DEV)\nswd\n4000\nloadfile $< \nreset\nr\ng\nexit\n" | \
		$(JLINK) -if swd -speed 4000 -device $(JLINK_DEV)

# ---- Size report ----
size: $(BUILD_DIR)/$(TARGET).elf
	$(SIZE) --format=berkeley $<

# ---- Disassembly ----
dump: $(BUILD_DIR)/$(TARGET).elf
	$(OBJDUMP) -d $< > $(BUILD_DIR)/$(TARGET).lst

# ---- Clean ----
clean:
	rm -rf $(BUILD_DIR)

# ---- Dependency tracking ----
-include $(OBJS:.o=.d)

# ---- End of Makefile ----