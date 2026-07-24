# Makefile for AlloyScan firmware
# Author: jayis1
# Copyright (c) 2026 jayis1. All rights reserved.
# SPDX-License-Identifier: GPL-2.0
#
# Build system for the AlloyScan STM32G474 firmware.
# Requires arm-none-eabi-gcc toolchain.

# ---- Toolchain ----
CC       = arm-none-eabi-gcc
OBJCOPY  = arm-none-eabi-objcopy
OBJDUMP  = arm-none-eabi-objdump
SIZE     = arm-none-eabi-size

# ---- Target configuration ----
MCU      = cortex-m4
FPU      = -mfpu=fpv4-sp-d16 -mfloat-abi=hard
ARCH     = -mcpu=$(MCU) -mthumb $(FPU)

# ---- Compiler flags ----
CFLAGS   = $(ARCH) -std=c99 -Wall -Wextra -Os -g3
CFLAGS  += -ffunction-sections -fdata-sections -fno-common
CFLAGS  += -I. -Idrivers
CFLAGS  += -DSTM32G474 -DUSE_FULL_LL_DRIVER
CFLAGS  += --specs=nano.specs --specs=nosys.specs

# ---- Linker flags ----
LDFLAGS  = $(ARCH) -T STM32G474CEUx_FLASH.ld
LDFLAGS += -Wl,--gc-sections -Wl,-Map=$(TARGET).map
LDFLAGS += --specs=nano.specs --specs=nosys.specs
LDFLAGS += -lm  /* math library for sinf, sqrtf, atan2f */

# ---- Target ----
TARGET   = alloyscan

# ---- Source files ----
SRCS = main.c \
       alloy_db.c \
       classifier.c \
       calibration.c \
       lockin.c \
       ui.c \
       drivers/dac_drv.c \
       drivers/adc_drv.c \
       drivers/oled_drv.c \
       drivers/flash_drv.c \
       drivers/ble_drv.c \
       drivers/tof_drv.c \
       drivers/button.c \
       drivers/battery.c

OBJS = $(SRCS:.c=.o)

# ---- Build rules ----
.PHONY: all clean flash dump

all: $(TARGET).elf $(TARGET).bin $(TARGET).hex

$(TARGET).elf: $(OBJS) STM32G474CEUx_FLASH.ld
	$(CC) $(LDFLAGS) -o $@ $(OBJS)
	$(SIZE) $@

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.s
	$(CC) $(CFLAGS) -c $< -o $@

flash: $(TARGET).bin
	st-flash write $(TARGET).bin 0x08000000

dump: $(TARGET).elf
	$(OBJDUMP) -D $(TARGET).elf > $(TARGET).asm

clean:
	rm -f $(OBJS) $(TARGET).elf $(TARGET).bin $(TARGET).hex \
	      $(TARGET).map $(TARGET).asm

# ---- Dependencies ----
main.o: board.h registers.h alloy_db.h classifier.h calibration.h lockin.h ui.h \
        drivers/dac_drv.h drivers/adc_drv.h drivers/oled_drv.h drivers/flash_drv.h \
        drivers/ble_drv.h drivers/tof_drv.h drivers/button.h drivers/battery.h

classifier.o: classifier.h alloy_db.h board.h
alloy_db.o: alloy_db.h
lockin.o: lockin.h registers.h board.h
ui.o: ui.h drivers/oled_drv.h alloy_db.h board.h
calibration.o: calibration.h classifier.h ui.h drivers/dac_drv.h drivers/adc_drv.h \
               drivers/tof_drv.h drivers/button.h lockin.h board.h

drivers/dac_drv.o: drivers/dac_drv.h registers.h board.h lockin.h
drivers/adc_drv.o: drivers/adc_drv.h registers.h board.h
drivers/oled_drv.o: drivers/oled_drv.h registers.h board.h
drivers/flash_drv.o: drivers/flash_drv.h registers.h board.h
drivers/ble_drv.o: drivers/ble_drv.h registers.h board.h
drivers/tof_drv.o: drivers/tof_drv.h registers.h board.h
drivers/button.o: drivers/button.h registers.h board.h
drivers/battery.o: drivers/battery.h registers.h board.h