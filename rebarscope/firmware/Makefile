# RebarScope firmware — Makefile
# Author: jayis1
# Copyright (C) 2026 jayis1. All rights reserved.
# SPDX-License-Identifier: GPL-3.0
#
# Cross-compile with arm-none-eabi-gcc. This Makefile is a build reference;
# it assumes the STM32L4 CMSIS headers and the vendor startup file are
# available under $(CMSIS_DIR) and linked via the linker script.

MCU      = cortex-m4
CPUFLAGS = -mthumb -mcpu=$(MCU) -mfpu=fpv4-sp-d16 -mfloat-abi=hard
CC       = arm-none-eabi-gcc
OBJCOPY  = arm-none-eabi-objcopy
SIZE     = arm-none-eabi-size

CMSIS_DIR = /opt/arm-cmsis
LDSCRIPT  = linker.ld

CFLAGS = $(CPUFLAGS) -std=c11 -Os -Wall -Wextra -ffunction-sections -fdata-sections \
         -I. -Idrivers -I$(CMSIS_DIR)/Include -DSTM32L4R5xx -DBOARD_AUTHOR=\"jayis1\"
LDFLAGS = $(CPUFLAGS) -T$(LDSCRIPT) -nostartfiles -Wl,--gc-sections -Wl,-Map=rebarscope.map

DRIVER_SRC = drivers/ads1220.c drivers/ad5940.c drivers/eddy.c \
             drivers/encoder.c drivers/imu.c drivers/ble_uart.c \
             drivers/oled.c drivers/fuel_gauge.c drivers/sha256.c \
             drivers/cobs.c drivers/crc16.c

APP_SRC = main.c survey.c

OBJS = $(DRIVER_SRC:.c=.o) $(APP_SRC:.c=.o)

all: rebarscope.elf rebarscope.hex rebarscope.bin

rebarscope.elf: $(OBJS) $(LDSCRIPT)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) -lc -lm -lnosys
	$(SIZE) $@

%.hex: %.elf
	$(OBJCOPY) -O ihex $< $@

%.bin: %.elf
	$(OBJCOPY) -O binary $< $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) rebarscope.elf rebarscope.hex rebarscope.bin rebarscope.map

.PHONY: all clean