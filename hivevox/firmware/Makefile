# Makefile — HiveVox firmware build
# Author: jayis1
# Copyright (c) 2026 jayis1 — MIT License
#
# Builds the HiveVox bare-metal firmware with arm-none-eabi-gcc.
# No external HAL; direct register access. Produces ELF + bin + hex.

# ---- Toolchain ----
CC      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE    = arm-none-eabi-size

# ---- Target MCU ----
MCU     = -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
MCU_LK  = -TSTM32L432KCUX_FLASH.ld

# ---- Flags ----
CFLAGS  = $(MCU) -Os -g -std=c11 -Wall -Wextra -Wno-unused-parameter
CFLAGS += -ffunction-sections -fdata-sections -fno-common -fno-builtin
CFLAGS += -I. -Idrivers

LDFLAGS = $(MCU) $(MCU_LK) -nostartfiles
LDFLAGS += -Wl,--gc-sections -Wl,-Map=hivevox.map -lc -lgcc

# ---- Sources ----
SRCS = main.c
SRCS += drivers/i2c.c drivers/onewire.c drivers/hx711.c
SRCS += drivers/sx1276.c drivers/acoustic.c

OBJS = $(SRCS:.c=.o)

# ---- Build rules ----
.PHONY: all clean flash size

all: hivevox.elf hivevox.bin hivevox.hex size

hivevox.elf: $(OBJS) startup.s STM32L432KCUX_FLASH.ld
	$(CC) $(LDFLAGS) -o $@ $(OBJS) startup.s

hivevox.bin: hivevox.elf
	$(OBJCOPY) -O binary $< $@

hivevox.hex: hivevox.elf
	$(OBJCOPY) -O ihex $< $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

size: hivevox.elf
	$(SIZE) hivevox.elf

flash: hivevox.bin
	openocd -f interface/stlink.cfg -f target/stm32l4x.cfg \
		-c "program hivevox.bin verify reset exit 0x08000000"

clean:
	rm -f $(OBJS) hivevox.elf hivevox.bin hivevox.hex hivevox.map