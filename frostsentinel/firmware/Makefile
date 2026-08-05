/*
 * Makefile — FrostSentinel firmware build
 *
 * Targets the STM32U575ZIT6 with arm-none-eabi-gcc.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

CROSS   = arm-none-eabi-
CC      = $(CROSS)gcc
OBJCOPY = $(CROSS)objcopy
SIZE    = $(CROSS)size

MCU     = -mcpu=cortex-m33 -mthumb -mfpu=auto -mfloat-abi=soft
CFLAGS  = $(MCU) -std=c11 -Wall -Wextra -Os -ffunction-sections -fdata-sections \
          -g3 -I.
LDFLAGS = $(MCU) -T linker.ld -Wl,--gc-sections -Wl,-Map=frostsentinel.map \
          --specs=nano.specs --specs=nosys.specs

SRCS = main.c \
       drivers/skyir.c \
       drivers/psychro.c \
       drivers/leafwet.c \
       drivers/acoustic.c \
       drivers/radio.c \
       drivers/ble.c \
       drivers/flashio.c \
       drivers/power.c \
       drivers/rtc.c \
       drivers/thermo.c \
       drivers/model.c

OBJS = $(SRCS:.c=.o) startup.o

TARGET = frostsentinel.elf
BIN    = frostsentinel.bin

.PHONY: all clean flash

all: $(TARGET) $(BIN)

$(TARGET): $(OBJS) linker.ld
	$(CC) $(LDFLAGS) -o $@ $(OBJS)
	$(SIZE) $@

$(BIN): $(TARGET)
	$(OBJCOPY) -O binary $< $@

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

startup.o: startup.s
	$(CROSS)as $(MCU) -o $@ $<

flash: $(BIN)
	openocd -f interface/stlink.cfg -f target/stm32u5x.cfg \
	  -c "program $(BIN) 0x08000000 verify reset exit"

clean:
	rm -f $(OBJS) $(TARGET) $(BIN) frostsentinel.map