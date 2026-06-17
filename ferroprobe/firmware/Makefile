# Makefile — FerroProbe Firmware Build System
# FerroProbe — Open-Source 3-Axis Fluxgate Magnetometer
#
# Author: jayis1
# Copyright (c) 2026 jayis1. All rights reserved.
# SPDX-License-Identifier: GPL-2.0
#
# Build system for the FerroProbe STM32H743 firmware using
# the ARM GCC toolchain (arm-none-eabi-gcc).
#
# Usage:
#   make          — Build the firmware (ferroprobe.elf + .bin + .hex)
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

TARGET   = ferroprobe
MCU      = cortex-m7
FPU      = fpv5-sp-d16
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
CFLAGS += -DSTM32H743xx
CFLAGS += -DARM_MATH_CM7
CFLAGS += -I. -Idrivers

# Linker flags
LDFLAGS  = $(CPU_FLAGS)
LDFLAGS += -T$(LINKER_SCRIPT)
LDFLAGS += -Wl,--gc-sections
LDFLAGS += -Wl,-Map=$(TARGET).map
LDFLAGS += -nostartup
LDFLAGS += -specs=nano.specs
LDFLAGS += -lc -lm -lnosys

# =====================================================================
#  Source files
# =====================================================================

SRCS = \
    main.c \
    drivers/fluxgate.c \
    drivers/lockin.c \
    drivers/adc_drv.c \
    drivers/imu.c \
    drivers/gps.c \
    drivers/sdlog.c \
    drivers/ble.c \
    drivers/calib.c

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

main.o: board.h registers.h drivers/fluxgate.h drivers/lockin.h drivers/adc_drv.h \
         drivers/imu.h drivers/gps.h drivers/sdlog.h drivers/ble.h drivers/calib.h
drivers/fluxgate.o: drivers/fluxgate.h board.h registers.h
drivers/lockin.o: drivers/lockin.h board.h registers.h
drivers/adc_drv.o: drivers/adc_drv.h drivers/lockin.h board.h registers.h
drivers/imu.o: drivers/imu.h board.h registers.h
drivers/gps.o: drivers/gps.h board.h registers.h
drivers/sdlog.o: drivers/sdlog.h board.h registers.h
drivers/ble.o: drivers/ble.h board.h registers.h
drivers/calib.o: drivers/calib.h drivers/lockin.h board.h

# =====================================================================
#  Linker script (generated inline for self-contained build)
# =====================================================================

# A minimal linker script for the STM32H743VIT6 (2 MB flash, 1 MB SRAM)
# is provided in linker.ld. If it doesn't exist, create it with:
#   make linker.ld

linker.ld:
	@echo "Creating linker.ld..."
	@cat > linker.ld << 'EOF'
/* Linker script for STM32H743VIT6 — FerroProbe
 * Author: jayis1
 * 2 MB Flash @ 0x08000000, 1 MB SRAM @ 0x20000000
 */
ENTRY(Reset_Handler)
MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 2048K
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 1024K
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