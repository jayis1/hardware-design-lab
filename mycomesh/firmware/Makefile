# Makefile — MycoMesh Firmware Build System
# MycoMesh — Open-Source Fungal Mycelium Electrophysiology & Chemical Sensing Network
#
# Author: jayis1
# Copyright (c) 2026 jayis1. All rights reserved.
# SPDX-License-Identifier: GPL-2.0
#
# Build system for the MycoMesh STM32L4R9ZI firmware using the ARM GCC
# toolchain (arm-none-eabi-gcc).
#
# Usage:
#   make          — Build the firmware (mycomesh.elf + .bin + .hex)
#   make flash    — Flash to the target via ST-Link (openocd)
#   make clean    — Remove build artifacts
#   make size     — Show firmware size summary
#   make debug    — Start GDB server for debugging
#   make lst      — Generate disassembly listing

# =====================================================================
#  Toolchain configuration
# =====================================================================

CC      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
OBJDUMP = arm-none-eabi-objdump
SIZE    = arm-none-eabi-size
GDB     = arm-none-eabi-gdb

# =====================================================================
#  Target configuration
# =====================================================================

TARGET   = mycomesh
MCU      = cortex-m4
FPU      = fpv4-sp-d16
FLOAT_ABI = hard

# =====================================================================
#  Compiler flags
# =====================================================================

CFLAGS  = -mcpu=$(MCU) -mthumb -mfloat-abi=$(FLOAT_ABI) -mfpu=$(FPU)
CFLAGS += -std=c11 -Wall -Wextra -Wno-unused-parameter -Os -g3
CFLAGS += -ffunction-sections -fdata-sections -fno-common -fno-exceptions
CFLAGS += -DSTM32L4R9xx -DUSE_FULL_LL_DRIVER
CFLAGS += -I. -Idrivers

LDFLAGS = -mcpu=$(MCU) -mthumb -mfloat-abi=$(FLOAT_ABI) -mfpu=$(FPU)
LDFLAGS += -T linker.ld -Wl,--gc-sections -Wl,-Map=$(TARGET).map
LDFLAGS += -Wl,--print-memory-usage -nostdlib -lc -lm -lnosys

# =====================================================================
#  Source files
# =====================================================================

SRC_DIR = . drivers

C_SOURCES = \
	main.c \
	drivers/ads1298.c \
	drivers/dsp.c \
	drivers/spike_sort.c \
	drivers/classifier.c \
	drivers/env_sensors.c \
	drivers/sdlog.c \
	drivers/lorawan.c \
	drivers/power.c \
	drivers/protocol.c

# =====================================================================
#  Object files
# =====================================================================

OBJECTS = $(C_SOURCES:.c=.o)

# =====================================================================
#  Build rules
# =====================================================================

.PHONY: all clean flash size debug lst

all: $(TARGET).elf $(TARGET).bin $(TARGET).hex

$(TARGET).elf: $(OBJECTS) linker.ld
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS)
	$(SIZE) $@

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# =====================================================================
#  Flashing
# =====================================================================

flash: $(TARGET).bin
	openocd -f interface/stlink.cfg -f target/stm32l4x.cfg \
		-c "program $(TARGET).bin 0x08000000 verify reset exit"

# =====================================================================
#  Debugging
# =====================================================================

debug:
	openocd -f interface/stlink.cfg -f target/stm32l4x.cfg \
		-c "init" -c "halt" &
	sleep 1
	$(GDB) -ex "target remote :3333" -ex "load" $(TARGET).elf

# =====================================================================
#  Disassembly listing
# =====================================================================

lst: $(TARGET).elf
	$(OBJDUMP) -d -S $< > $(TARGET).lst

# =====================================================================
#  Size summary
# =====================================================================

size: $(TARGET).elf
	$(SIZE) $<

# =====================================================================
#  Clean
# =====================================================================

clean:
	rm -f $(OBJECTS) $(TARGET).elf $(TARGET).bin $(TARGET).hex \
	      $(TARGET).map $(TARGET).lst