# Makefile - Pollen Scout firmware
# Target: STM32H743VIT6 (Cortex-M7, Thumb-2)
#
# Author: jayis1
# Copyright (c) 2026 jayis1. All rights reserved.
# SPDX-License-Identifier: GPL-3.0

# ---- Toolchain ----
CC      := arm-none-eabi-gcc
OBJCOPY := arm-none-eabi-objcopy
SIZE    := arm-none-eabi-size
OBJDUMP := arm-none-eabi-objdump

# ---- Paths ----
SRC_DIR   := .
DRV_DIR   := drivers
ML_DIR    := ml
NET_DIR   := net
BUILD_DIR := build

# ---- MCU flags ----
MCU := -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard
CFLAGS := $(MCU) -O2 -g -Wall -Wextra -Wno-unused-parameter \
          -ffreestanding -fdata-sections -ffunction-sections \
          -I. -Idrivers -Iml -Inet \
          -DSTM32H743xx -DARM_MATH_CM7 -DUSE_FULL_LL_DRIVER \
          -D__AUTHOR__='"jayis1"'

LDFLAGS := $(MCU) -T linkerscript.ld -Wl,--gc-sections -Wl,--print-memory-usage -nostartfile
LDLIBS  := -lc -lm -lnosys

# ---- Sources ----
C_SRCS := \
    main.c \
    drivers/ov5640.c \
    drivers/bme280.c \
    drivers/si1145.c \
    drivers/pms5003.c \
    drivers/sdp810.c \
    drivers/sx1262.c \
    drivers/wind_rs485.c \
    drivers/bq25895.c \
    drivers/sd_fatfs.c \
    ml/segment.c \
    ml/classifier.c \
    ml/model_weights.c \
    ml/forecaster.c \
    net/lorawan.c

OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(C_SRCS))

# ---- Targets ----
.PHONY: all clean flash dump size

all: $(BUILD_DIR)/pollenscout.elf $(BUILD_DIR)/pollenscout.hex $(BUILD_DIR)/pollenscout.bin

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/pollenscout.elf: $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $@
	$(SIZE) $@

$(BUILD_DIR)/pollenscout.hex: $(BUILD_DIR)/pollenscout.elf
	$(OBJCOPY) -O ihex $< $@

$(BUILD_DIR)/pollenscout.bin: $(BUILD_DIR)/pollenscout.elf
	$(OBJCOPY) -O binary $< $@

flash: $(BUILD_DIR)/pollenscout.hex
	openocd -f interface/stlink.cfg -f target/stm32h7x.cfg \
	  -c "program $< verify reset exit"

dump: $(BUILD_DIR)/pollenscout.elf
	$(OBJDUMP) -D -S $< > $(BUILD_DIR)/pollenscout.lst

clean:
	rm -rf $(BUILD_DIR)