# Makefile — Glaciator-Motes firmware
# Author: jayis1
# Copyright (C) 2026 jayis1. All rights reserved.
# License: GPL-2.0

# ---- Toolchain ----
CC       = arm-none-eabi-gcc
OBJCOPY  = arm-none-eabi-objcopy
SIZE     = arm-none-eabi-size

# ---- Target MCU: STM32WL55JC ----
CPU      = -mcpu=cortex-m4
FPU      = -mfpu=fpv4-sp-d16 -mfloat-abi=hard
MCU      = $(CPU) -mthumb $(FPU)

# ---- Flags ----
CFLAGS   = $(MCU) -std=c11 -Wall -Wextra -Wno-unused-parameter \
           -Os -ffunction-sections -fdata-sections \
           -I. -I./stm32wl55_hal \
           -DSTM32WL55xx -DUSE_FULL_LL_DRIVER \
           -DAUTHOR='"jayis1"' -DLICENSE='"GPL-2.0"'

LDFLAGS  = $(MCU) -T linker.ld -Wl,--gc-sections -Wl,-Map=build/firmware.map \
           -nostartfiles -lc -lnosys

# ---- Sources ----
SRCS_C = main.c adc_driver.c mems_driver.c seismology.c lora_mesh.c \
         gps_discipline.c power.c sdlog.c lz4.c \
         stm32wl55_hal/system_stm32wl55.c

SRCS_S = stm32wl55_hal/startup_stm32wl55.s

OBJS = $(patsubst %.c, build/%.o, $(SRCS_C)) \
       $(patsubst %.s, build/%.o, $(SRCS_S))

# ---- Targets ----
.PHONY: all clean flash size

all: build/glaciator-motes.elf build/glaciator-motes.bin size

build:
	@mkdir -p build

build/%.o: %.c | build
	$(CC) $(CFLAGS) -c $< -o $@

build/%.o: %.s | build
	$(CC) $(CFLAGS) -c $< -o $@

build/glaciator-motes.elf: $(OBJS) linker.ld | build
	$(CC) $(LDFLAGS) $(OBJS) -o $@

build/glaciator-motes.bin: build/glaciator-motes.elf
	$(OBJCOPY) -O binary $< $@

flash: build/glaciator-motes.bin
	openocd -f interface/stlink.cfg -f target/stm32wlx.cfg \
	  -c "program build/glaciator-motes.bin 0x08000000 verify reset exit"

size: build/glaciator-motes.elf
	$(SIZE) $<

clean:
	rm -rf build

# ---- Author / license metadata ----
AUTHOR := jayis1
LICENSE := GPL-2.0