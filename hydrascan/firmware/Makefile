# Makefile — HydraScan firmware build
# Author: jayis1
# Copyright (C) 2026 jayis1
#
# Cross-compiles the HydraScan firmware for the STM32H733VG using the
# arm-none-eabi-gcc toolchain. No vendor HAL is required — all drivers
# are register-level (see registers.h / board.h).
#
#   make          → build hydrascan.elf, .hex, .bin
#   make flash     → openocd via ST-Link V3
#   make size      → print memory section sizes
#   make clean
#
# Toolchain prefix override:  make CROSS=arm-none-eabi-

CROSS   ?= arm-none-eabi-
CC      := $(CROSS)gcc
OBJCOPY := $(CROSS)objcopy
SIZE    := $(CROSS)size

# ---- MCU / flags ----------------------------------------------------
MCU     := -mcpu=cortex-m7 -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb
CFLAGS  := $(MCU) -O2 -g3 -ffunction-sections -fdata-sections \
           -Wall -Wextra -std=c11 \
           -I. -Idrivers
LDFLAGS := $(MCU) -Tlinker.ld -Wl,--gc-sections -Wl,-Map=hydrascan.map \
           -specs=nosys.specs -specs=nano.specs -lm

# ---- Sources --------------------------------------------------------
SRCS := main.c classifier.c startup.c \
        drivers/optical.c drivers/eis.c drivers/thermal.c \
        drivers/display.c drivers/ble.c drivers/flash_lib.c

OBJS := $(SRCS:.c=.o)
TGT  := hydrascan

.PHONY: all flash size clean

all: $(TGT).elf $(TGT).hex $(TGT).bin

$(TGT).elf: $(OBJS) linker.ld
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

$(TGT).hex: $(TGT).elf
	$(OBJCOPY) -O ihex $< $@

$(TGT).bin: $(TGT).elf
	$(OBJCOPY) -O binary $< $@

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

flash: $(TGT).hex
	openocd -f interface/stlink.cfg -f target/stm32h7x.cfg \
	  -c "program $(TGT).hex verify reset exit 0x08000000"

size: $(TGT).elf
	$(SIZE) $(TGT).elf

clean:
	rm -f $(OBJS) $(TGT).elf $(TGT).hex $(TGT).bin $(TGT).map