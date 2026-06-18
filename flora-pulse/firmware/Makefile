# Makefile — FloraPulse Firmware Build System
# FloraPulse — Plant Electrophysiology & Sap-Flow Stress Monitor
#
# Author: jayis1
# Copyright (c) 2026 jayis1. All rights reserved.
# SPDX-License-Identifier: GPL-2.0
#
# Build system for the FloraPulse STM32L476 firmware using
# the ARM GCC toolchain (arm-none-eabi-gcc).
#
# Usage:
#   make          — Build the firmware (flora-pulse.elf + .bin + .hex)
#   make flash    — Flash to the target via ST-Link (openocd)
#   make clean    — Remove build artifacts
#   make size     — Show firmware size summary
#   make debug    — Start GDB server for debugging

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

TARGET   = flora-pulse
MCU      = cortex-m4
FPU      = fpv4-sp-d16
FLOAT_ABI = hard
LINKER_SCRIPT = linker.ld

# =====================================================================
#  Compiler flags
# =====================================================================

# CPU and FPU
CPU_FLAGS = -mcpu=$(MCU) -mthumb -mfpu=$(FPU) -mfloat-abi=$(FLOAT_ABI)

# C flags
CFLAGS  = $(CPU_FLAGS)
CFLAGS += -std=c11
CFLAGS += -O2
CFLAGS += -g3
CFLAGS += -Wall -Wextra -Wno-unused-parameter
CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += -fno-common -fno-builtin
CFLAGS += -ffreestanding
CFLAGS += -DSTM32L476xx
CFLAGS += -DARM_MATH_CM4
CFLAGS += -I. -Idrivers

# Linker flags
LDFLAGS  = $(CPU_FLAGS)
LDFLAGS += -T$(LINKER_SCRIPT)
LDFLAGS += -Wl,--gc-sections
LDFLAGS += -Wl,-Map=$(TARGET).map
LDFLAGS += -specs=nano.specs
LDFLAGS += -lc -lm -lnosys

# =====================================================================
#  Source files
# =====================================================================

SRCS = \
    main.c \
    drivers/ads1292.c \
    drivers/bme280.c \
    drivers/sapflow.c \
    drivers/leafwet.c \
    drivers/lora.c \
    drivers/ble.c \
    drivers/sdlog.c \
    drivers/apdetection.c

OBJS = $(SRCS:.c=.o)

# =====================================================================
#  Build rules
# =====================================================================

.PHONY: all clean flash size debug

all: $(TARGET).elf $(TARGET).bin $(TARGET).hex size

$(TARGET).elf: $(OBJS) $(LINKER_SCRIPT)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)
	$(SIZE) $@

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# =====================================================================
#  Flash and debug
# =====================================================================

OPENOCD = openocd
OPENOCD_CFG = openocd.cfg
OPENOCD_FLAGS = -f $(OPENOCD_CFG)

flash: $(TARGET).bin
	$(OPENOCD) $(OPENOCD_FLAGS) -c "program $(TARGET).bin verify reset exit 0x08000000"

debug: $(TARGET).elf
	$(OPENOCD) $(OPENOCD_FLAGS) &
	sleep 1
	$(GDB) -ex "target remote :3333" -ex "load" $(TARGET).elf

size: $(TARGET).elf
	$(SIZE) $(TARGET).elf

# =====================================================================
#  Clean
# =====================================================================

clean:
	rm -f $(OBJS) $(TARGET).elf $(TARGET).bin $(TARGET).hex $(TARGET).map
	rm -f drivers/*.o

# =====================================================================
#  Dependencies
# =====================================================================

main.o: board.h registers.h \
         drivers/ads1292.h drivers/bme280.h drivers/sapflow.h \
         drivers/leafwet.h drivers/lora.h drivers/ble.h \
         drivers/sdlog.h drivers/apdetection.h
drivers/ads1292.o: drivers/ads1292.h board.h registers.h
drivers/bme280.o: drivers/bme280.h board.h registers.h
drivers/sapflow.o: drivers/sapflow.h board.h registers.h
drivers/leafwet.o: drivers/leafwet.h board.h registers.h
drivers/lora.o: drivers/lora.h board.h registers.h
drivers/ble.o: drivers/ble.h board.h registers.h
drivers/sdlog.o: drivers/sdlog.h board.h registers.h
drivers/apdetection.o: drivers/apdetection.h board.h registers.h

# =====================================================================
#  Linker script (generated inline for self-contained build)
# =====================================================================

# A minimal linker script for the STM32L476RGT6 (1 MB flash, 128 KB SRAM)
# is provided in linker.ld. If it doesn't exist, create it with:
#   make linker.ld

linker.ld:
	@echo "Creating linker.ld..."
	@cat > linker.ld << 'EOF'
/* Linker script for STM32L476RGT6 — FloraPulse
 * Author: jayis1
 * 1 MB Flash @ 0x08000000, 128 KB SRAM @ 0x20000000
 * (96 KB SRAM1 + 32 KB SRAM2)
 */
ENTRY(Reset_Handler)
MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 1024K
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 96K
    RAM2  (rwx) : ORIGIN = 0x10000000, LENGTH = 32K
}
_estack = ORIGIN(RAM) + LENGTH(RAM);
SECTIONS
{
    .isr_vector : { KEEP(*(.isr_vector)) } > FLASH
    .text : { *(.text*) *(.rodata*) } > FLASH
    .ARM.exidx : { *(.ARM.exidx*) } > FLASH
    .data : { *(.data*) } > RAM AT > FLASH
    .bss : { *(.bss*) *(COMMON) } > RAM
    . = ALIGN(8);
    _end = .;
}
EOF