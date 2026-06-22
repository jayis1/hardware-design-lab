# Makefile — LumiCast Firmware Build System
# LumiCast — Open-Source Circadian Light & SPD Analyzer
#
# Author: jayis1
# Copyright (c) 2026 jayis1. All rights reserved.
# SPDX-License-Identifier: GPL-2.0
#
# Build system for the LumiCast STM32WB55 firmware using the ARM GCC
# toolchain (arm-none-eabi-gcc).  The BLE stack binary is expected to be
# pre-flashed on the Cortex-M0+ core.
#
# Usage:
#   make          — Build the firmware (lumicast.elf + .bin + .hex)
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

TARGET   = lumicast
MCU      = cortex-m4
FPU      = fpv4-sp-d16
FLOAT_ABI = hard
LINKER_SCRIPT = linker.ld

# =====================================================================
#  Compiler flags
# =====================================================================

CPU_FLAGS = -mcpu=$(MCU) -mthumb -mfloat-abi=$(FLOAT_ABI) -mfpu=$(FPU)

CFLAGS  = $(CPU_FLAGS)
CFLAGS += -std=c11
CFLAGS += -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers
CFLAGS += -fdata-sections -ffunction-sections
CFLAGS += -fno-common -fno-exceptions
CFLAGS += -g3 -gdwarf-2
CFLAGS += -O2
CFLAGS += -I. -Idrivers
CFLAGS += -DBOARD_NAME=\"LumiCast\"
CFLAGS += -DFW_VERSION=\"1.0.0\"
CFLAGS += -DAUTHOR=\"jayis1\"

LDFLAGS  = $(CPU_FLAGS)
LDFLAGS += -T$(LINKER_SCRIPT)
LDFLAGS += -Wl,--gc-sections
LDFLAGS += -Wl,-Map=$(TARGET).map,--cref
LDFLAGS += -lm -lc -lnosys

# =====================================================================
#  Source files
# =====================================================================

SRCS = main.c
SRCS += drivers/spectrometer.c
SRCS += drivers/circadian.c
SRCS += drivers/flicker.c
SRCS += drivers/i2c_drv.c
SRCS += drivers/timer_drv.c
SRCS += drivers/display.c
SRCS += drivers/sdlog.c
SRCS += drivers/ble.c

OBJS = $(SRCS:.c=.o)

# =====================================================================
#  Build rules
# =====================================================================

.PHONY: all clean flash size debug lst

all: $(TARGET).elf $(TARGET).bin $(TARGET).hex size

$(TARGET).elf: $(OBJS) $(LINKER_SCRIPT)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)
	$(SIZE) $@

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

size: $(TARGET).elf
	$(SIZE) $(TARGET).elf

lst: $(TARGET).elf
	$(OBJDUMP) -d -S $(TARGET).elf > $(TARGET).lst

flash: $(TARGET).hex
	openocd -f interface/stlink.cfg -f target/stm32wbx.cfg \
	  -c "program $(TARGET).hex verify reset exit 0x08000000"

debug: $(TARGET).elf
	openocd -f interface/stlink.cfg -f target/stm32wbx.cfg \
	  -c "init" -c "halt" -c "arm semihosting enable" &
	$(GDB) -ex "target remote localhost:3333" $(TARGET).elf

clean:
	rm -f $(OBJS) $(TARGET).elf $(TARGET).bin $(TARGET).hex \
	      $(TARGET).map $(TARGET).lst