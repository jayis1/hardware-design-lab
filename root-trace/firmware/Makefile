# Makefile — RootTrace Firmware
# RootTrace — Open-Source Electrical Impedance Tomography Root Imaging System
#
# Author: jayis1
# Copyright (c) 2026 jayis1. All rights reserved.
# SPDX-License-Identifier: GPL-3.0
#
# Build system for the RootTrace STM32H743 firmware using arm-none-eabi-gcc.
# Targets:
#   make        — build roottrace.elf
#   make flash  — flash via ST-Link (requires st-flash)
#   make clean   — remove build artifacts
#   make size    — show memory usage

# Toolchain
CROSS    = arm-none-eabi-
CC       = $(CROSS)gcc
OBJCOPY  = $(CROSS)objcopy
OBJDUMP  = $(CROSS)objdump
SIZE     = $(CROSS)size
GDB      = $(CROSS)gdb

# MCU flags: STM32H743ZIT6 Cortex-M7 with DP FPU
MCU      = -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard

# Compiler flags
CFLAGS   = $(MCU) -std=c11 -Wall -Wextra -Wno-unused-parameter -Os -g3
CFLAGS  += -ffunction-sections -fdata-sections -fno-common -fno-exceptions
CFLAGS  += -ffreestanding -fno-builtin
CFLAGS  += -DSTM32H743xx -DARM_MATH_CM7 -D__FPU_PRESENT=1
CFLAGS  += -I. -Idrivers

# Linker flags
LDFLAGS  = $(MCU) -T linker.ld -nostartfiles -Wl,--gc-sections
LDFLAGS += -Wl,-Map=roottrace.map -Wl,--print-memory-usage
LDFLAGS += -lm -lc

# Source files
SRCDIR   = .
DRVDIR   = drivers
SOURCES  = $(SRCDIR)/main.c
SOURCES += $(DRVDIR)/ad5940.c
SOURCES += $(DRVDIR)/mux.c
SOURCES += $(DRVDIR)/eit_acq.c
SOURCES += $(DRVDIR)/reconstruct.c
SOURCES += $(DRVDIR)/display.c
SOURCES += $(DRVDIR)/sdlog.c
SOURCES += $(DRVDIR)/ble_proto.c
SOURCES += $(DRVDIR)/power.c
SOURCES += $(DRVDIR)/env_sens.c

# Object files (built into build/)
BUILD    = build
OBJECTS  = $(patsubst %.c,$(BUILD)/%.o,$(SOURCES))

# Default target
TARGET   = roottrace

.PHONY: all clean flash size debug

all: $(TARGET).elf $(TARGET).hex $(TARGET).bin size

# Build directories
$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Link
$(TARGET).elf: $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

# Generate hex and binary
$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

# Disassembly (for debugging)
$(TARGET).lst: $(TARGET).elf
	$(OBJDUMP) -d -S $< > $@

# Flash via ST-Link
flash: $(TARGET).bin
	st-flash write $(TARGET).bin 0x08000000

# Size report
size: $(TARGET).elf
	$(SIZE) $<

# Debug with GDB + ST-Link
debug: $(TARGET).elf
	$(GDB) -ex "target extended-remote localhost:4242" $<

# Clean
clean:
	rm -rf $(BUILD) $(TARGET).elf $(TARGET).hex $(TARGET).bin $(TARGET).map $(TARGET).lst