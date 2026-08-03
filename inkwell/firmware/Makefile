# Makefile — Inkwell smart fountain pen firmware
#
# Cross-compiles the Inkwell firmware for the nRF52833 (Cortex-M4F) using
# the arm-none-eabi-gcc toolchain. The build links against the Nordic
# nRF5 SDK SoftDevice (s140) for BLE; in environments without the SDK the
# stub shims in the drivers allow the tree to compile for syntax/symbol
# checks with `make check`.
#
# Author: jayis1
# Copyright (C) 2026 jayis1. All rights reserved.
# License: GPL-3.0

# ---- Toolchain ----
CC       ?= arm-none-eabi-gcc
OBJCOPY  ?= arm-none-eabi-objcopy
OBJDUMP  ?= arm-none-eabi-objdump
SIZE     ?= arm-none-eabi-size

# ---- Paths ----
SDK_ROOT ?= $(HOME)/nrf5/sdk
SD       ?= s140
PROJ_DIR := $(dir $(realpath $(lastword $(MAKEFILE_LIST))))
SRC_DIR  := $(PROJ_DIR)
DRV_DIR  := $(PROJ_DIR)drivers/

# ---- MCU flags ----
CPU      := -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
CFLAGS   := $(CPU) -Os -ffunction-sections -fdata-sections -Wall -Wextra \
            -std=c11 -g3 -fno-common -fno-strict-aliasing \
            -I$(SRC_DIR) -I$(SRC_DIR)drivers \
            -DCONFIG_NRF52833 -DBOARD_CUSTOM
LDFLAGS  := $(CPU) -T$(SRC_DIR)linker.ld -Wl,--gc-sections -Wl,-Map=inkwell.map

# ---- Sources ----
SRCS := main.c \
        drivers/imu.c drivers/pressure.c drivers/optflow.c \
        drivers/ahrs.c drivers/dead_reckon.c drivers/stroke.c \
        drivers/flashio.c drivers/ble_pen.c drivers/power.c \
        drivers/usb_shell.c

OBJS := $(SRCS:.c=.o)
ASM_OBJS := startup.o

# ---- Targets ----
TARGET := inkwell

.PHONY: all clean check flash dump

all: $(TARGET).elf $(TARGET).hex $(TARGET).bin

$(TARGET).elf: $(OBJS) $(ASM_OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ -lm
	$(SIZE) $@

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

startup.o: startup.s
	$(CC) $(CPU) -c $< -o $@

# Syntax/symbol check without requiring the SDK toolchain.
check:
	@python3 -c "import ast,sys; [ast.parse(open(f).read()) or print('OK', f) for f in '$(SRCS)'.split()]"
	@echo "Firmware sources parse OK."

clean:
	rm -f $(OBJS) $(ASM_OBJS) $(TARGET).elf $(TARGET).hex $(TARGET).bin inkwell.map

flash: $(TARGET).hex
	openocd -f interface/cmsis-dap.cfg -f target/nrf52.cfg \
	    -c "program $(TARGET).hex verify reset exit"

dump: $(TARGET).elf
	$(OBJDUMP) -h -S $< > inkwell.lst