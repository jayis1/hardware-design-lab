# TactiScript Firmware Makefile
#
# Author: jayis1
# Copyright (C) 2026 jayis1. All rights reserved.
# License: GPL-2.0
#
# Build system for the TactiScript ring firmware.
# Targets the Nordic nRF5340 (dual Cortex-M33) using the nRF Connect SDK
# (Zephyr-based). This Makefile wraps the Zephyr west build system.
#
# Prerequisites:
#   - nRF Connect SDK v2.x installed (with west, Zephyr, nRFx)
#   - GNU Arm Embedded Toolchain (arm-none-eabi-gcc 12.x+)
#   - nRF Command Line Tools (for flashing with nrfjprog)
#
# Usage:
#   make            # Build firmware (default: debug)
#   make release    # Build optimized release
#   make flash      # Flash to device via J-Link
#   make clean      # Remove build artifacts
#   make size       # Show firmware size breakdown

# ----------------------------------------------------------------
# Toolchain and SDK paths
# ----------------------------------------------------------------
NCSDIR     ?= /opt/nordic/ncs
ZEPHYR_DIR ?= $(NCSDIR)/zephyr
TOOLCHAIN  ?= /opt/gcc-arm-none-eabi

# ----------------------------------------------------------------
# Build configuration
# ----------------------------------------------------------------
BOARD      := tactiscript_nrf5340
BUILD_DIR  := build
TARGET     := tactiscript-fw

# Build type: debug or release
BUILD_TYPE ?= debug

# Source files
SRC_DIR    := .
DRIVER_DIR := drivers

SRCS := \
    $(SRC_DIR)/main.c \
    $(DRIVER_DIR)/actuator_drv.c \
    $(DRIVER_DIR)/braille.c \
    $(DRIVER_DIR)/imu_drv.c \
    $(DRIVER_DIR)/ble_svc.c \
    $(DRIVER_DIR)/oled_drv.c \
    $(DRIVER_DIR)/power.c \
    $(DRIVER_DIR)/haptic_render.c

# Include paths
INCLUDES := \
    -I$(SRC_DIR) \
    -I$(DRIVER_DIR) \
    -I$(ZEPHYR_DIR)/include \
    -I$(NCSDIR)/nrfx/mdk \
    -I$(NCSDIR)/modules/hal/nordic/nrfx

# Compiler flags
CFLAGS_COMMON := \
    -mcpu=cortex-m33 \
    -mthumb \
    -mfloat-abi=hard \
    -mfpu=fpv5-sp-d16 \
    -std=c11 \
    -Wall \
    -Wextra \
    -Wno-unused-parameter \
    -fdata-sections \
    -ffunction-sections \
    -g3

CFLAGS_DEBUG := -O0 -DDEBUG
CFLAGS_RELEASE := -O2 -DNDEBUG -fomit-frame-pointer

LDFLAGS := \
    -mcpu=cortex-m33 \
    -mthumb \
    -Wl,--gc-sections \
    -Wl,-Map=$(BUILD_DIR)/$(TARGET).map \
    -T$(NCSDIR)/modules/hal/nordic/nrfx/mdk/nrf5340_xxaa_application.ld

# ----------------------------------------------------------------
# Targets
# ----------------------------------------------------------------

.PHONY: all release flash clean size help

all: debug

debug: CFLAGS := $(CFLAGS_COMMON) $(CFLAGS_DEBUG)
debug: $(BUILD_DIR)/$(TARGET).elf
	@echo "[BUILD] Debug build complete: $(BUILD_DIR)/$(TARGET).elf"

release: CFLAGS := $(CFLAGS_COMMON) $(CFLAGS_RELEASE)
release: $(BUILD_DIR)/$(TARGET).elf
	@echo "[BUILD] Release build complete: $(BUILD_DIR)/$(TARGET).elf"

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Compile each source file
OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRCS))

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	@echo "[CC] $<"
	@$(TOOLCHAIN)/bin/arm-none-eabi-gcc $(CFLAGS) $(INCLUDES) -c $< -o $@

# Link
$(BUILD_DIR)/$(TARGET).elf: $(OBJS)
	@echo "[LD] Linking $@..."
	@$(TOOLCHAIN)/bin/arm-none-eabi-gcc $(LDFLAGS) $(OBJS) -o $@ \
	    -lc -lm -lnosys
	@$(TOOLCHAIN)/bin/arm-none-eabi-size $@

# Generate hex and bin
$(BUILD_DIR)/$(TARGET).hex: $(BUILD_DIR)/$(TARGET).elf
	@$(TOOLCHAIN)/bin/arm-none-eabi-objcopy -O ihex $< $@

$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	@$(TOOLCHAIN)/bin/arm-none-eabi-objcopy -O binary $< $@

# Flash via J-Link
flash: $(BUILD_DIR)/$(TARGET).hex
	@echo "[FLASH] Programming device..."
	@nrfjprog --program $(BUILD_DIR)/$(TARGET).hex --chiperase -f nrf5340
	@nrfjprog --reset -f nrf5340

# Size report
size: $(BUILD_DIR)/$(TARGET).elf
	@$(TOOLCHAIN)/bin/arm-none-eabi-size -A $<

# Clean
clean:
	@echo "[CLEAN] Removing build artifacts..."
	@rm -rf $(BUILD_DIR)

# Help
help:
	@echo "TactiScript Firmware Build System"
	@echo "Author: jayis1"
	@echo ""
	@echo "Targets:"
	@echo "  make         - Build debug firmware"
	@echo "  make release - Build optimized release firmware"
	@echo "  make flash   - Flash firmware to device (requires J-Link)"
	@echo "  make clean   - Remove build artifacts"
	@echo "  make size    - Show firmware size breakdown"
	@echo ""
	@echo "Variables:"
	@echo "  NCSDIR      - nRF Connect SDK path (default: /opt/nordic/ncs)"
	@echo "  TOOLCHAIN   - ARM GCC toolchain path"
	@echo "  BUILD_TYPE  - debug (default) or release"