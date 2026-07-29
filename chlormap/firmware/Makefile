# Makefile — ChloroMap firmware
# Author: jayis1
# Copyright (C) 2026 jayis1. All rights reserved.
# License: GPL-2.0

# ---- Toolchain ----
CC       = arm-none-eabi-gcc
OBJCOPY  = arm-none-eabi-objcopy
SIZE     = arm-none-eabi-size

# ---- Target MCU: STM32L432KCU6 ----
CPU      = -mcpu=cortex-m4
FPU      = -mfpu=fpv4-sp-d16 -mfloat-abi=hard
MCU      = $(CPU) -mthumb $(FPU)

# ---- Flags ----
CFLAGS   = $(MCU) -std=c11 -Wall -Wextra -Wno-unused-parameter \
           -Os -ffunction-sections -fdata-sections \
           -I. -I./drivers -I./stm32l432_hal \
           -DSTM32L432xx -DUSE_FULL_LL_DRIVER \
           -DAUTHOR='"jayis1"' -DLICENSE='"GPL-2.0"'

LDFLAGS  = $(MCU) -T linker.ld -Wl,--gc-sections -Wl,-Map=build/chlormap.map \
           -nostartfiles -lc -lnosys

# ---- Sources ----
SRCS_C = main.c \
         drivers/adc.c drivers/spectrometer.c drivers/indices.c \
         drivers/calib.c drivers/ble.c drivers/gps.c \
         drivers/display.c drivers/storage.c drivers/power.c \
         drivers/usb_cdc.c \
         stm32l432_hal/system_stm32l4.c

SRCS_S = stm32l432_hal/startup_stm32l432.s

OBJS = $(patsubst %.c, build/%.o, $(SRCS_C)) \
       $(patsubst %.s, build/%.o, $(SRCS_S))

# ---- Targets ----
.PHONY: all clean flash size

all: build/chlormap.elf build/chlormap.bin size

build:
	@mkdir -p build/drivers

build/%.o: %.c | build
	$(CC) $(CFLAGS) -c $< -o $@

build/%.o: %.s | build
	$(CC) $(CFLAGS) -c $< -o $@

build/chlormap.elf: $(OBJS) linker.ld | build
	$(CC) $(LDFLAGS) $(OBJS) -o $@

build/chlormap.bin: build/chlormap.elf
	$(OBJCOPY) -O binary $< $@

flash: build/chlormap.bin
	openocd -f interface/stlink.cfg -f target/stm32l4x.cfg \
	  -c "program build/chlormap.bin 0x08000000 verify reset exit"

size: build/chlormap.elf
	$(SIZE) $<

clean:
	rm -rf build

# ---- Author / license metadata ----
AUTHOR := jayis1
LICENSE := GPL-2.0