# Makefile — SpeckleFlow firmware build system
#
# Author: jayis1
# Copyright (C) 2026 jayis1. All rights reserved.
# License: GPL-2.0
#
# Build with arm-none-eabi-gcc. Targets STM32H733VIT6.
#
# Usage:
#   make          — build firmware.elf
#   make flash    — flash via OpenOCD
#   make clean    — remove build artifacts
#   make size     — show code size breakdown

# ---- Toolchain -----------------------------------------------------------
CC      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
OBJDUMP = arm-none-eabi-objdump
SIZE    = arm-none-eabi-size
GDB     = arm-none-eabi-gdb

# ---- Target configuration ------------------------------------------------
MCU    = -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard
DEFS   = -DSTM32H733xx -DUSE_FULL_LL_DRIVER=0 -DUSE_HAL_DRIVER=0
INCS   = -I. -Idrivers

# ---- Compiler flags ------------------------------------------------------
CFLAGS = $(MCU) $(DEFS) $(INCS) -O2 -Wall -Wextra -std=c11 \
         -fdata-sections -ffunction-sections -g -gdwarf-4
LDFLAGS = $(MCU) -T linker.ld -Wl,--gc-sections -Wl,-Map=firmware.map \
          -Wl,--cref -specs=nano.specs -specs=nosys.specs

# ---- Source files --------------------------------------------------------
SRCS = main.c \
       drivers/camera.c \
       drivers/fpga.c \
       drivers/laser.c \
       drivers/display.c \
       drivers/ble.c \
       drivers/sdcard.c \
       drivers/imu.c \
       drivers/power.c

OBJS = $(SRCS:.c=.o)

# ---- Build rules ---------------------------------------------------------
.PHONY: all clean flash size dump

all: firmware.elf

firmware.elf: $(OBJS) linker.ld
	$(CC) $(LDFLAGS) -o $@ $(OBJS) -lc -lm
	$(SIZE) $@

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

flash: firmware.elf
	openocd -f interface/stlink.cfg -f target/stm32h7x.cfg \
	  -c "program firmware.elf verify reset exit"

size: firmware.elf
	$(SIZE) -A firmware.elf

dump: firmware.elf
	$(OBJDUMP) -d -S firmware.elf > firmware.lst

clean:
	rm -f $(OBJS) firmware.elf firmware.map firmware.lst firmware.bin

# ---- Dependencies --------------------------------------------------------
main.o: board.h registers.h drivers/camera.h drivers/fpga.h drivers/laser.h \
        drivers/display.h drivers/ble.h drivers/sdcard.h drivers/imu.h \
        drivers/power.h
drivers/camera.o: board.h registers.h drivers/camera.h
drivers/fpga.o: board.h registers.h drivers/fpga.h
drivers/laser.o: board.h registers.h drivers/laser.h
drivers/display.o: board.h registers.h drivers/display.h
drivers/ble.o: board.h registers.h drivers/ble.h
drivers/sdcard.o: board.h registers.h drivers/sdcard.h
drivers/imu.o: board.h registers.h drivers/imu.h
drivers/power.o: board.h registers.h drivers/power.h