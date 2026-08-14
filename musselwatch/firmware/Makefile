# ============================================================
# MusselWatch firmware Makefile
# Target: STM32L432KC (Cortex-M4, 80 MHz)
# Author: jayis1
# Copyright (c) 2026 jayis1. All rights reserved.
# License: GPL-2.0
# ============================================================

# ---- Toolchain ----
PREFIX  = arm-none-eabi-
CC      = $(PREFIX)gcc
OBJCOPY = $(PREFIX)objcopy
OBJDUMP = $(PREFIX)objdump
SIZE    = $(PREFIX)size

# ---- CPU / FPU flags ----
ARCH_FLAGS = -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard

# ---- Include paths ----
INCLUDES = -I. -Idrivers

# ---- Compiler flags ----
CFLAGS  = $(ARCH_FLAGS) $(INCLUDES) -std=c11 -Wall -Wextra -Wpedantic \
          -Os -g3 -ffunction-sections -fdata-sections \
          -fno-common -fno-builtin -ffreestanding \
          -DSTM32L432xx -DBOARD_MUSSELWATCH

LDFLAGS = $(ARCH_FLAGS) -T linker.ld -Wl,--gc-sections -Wl,-Map=musselwatch.map \
          -nostdlib -Wl,--print-memory-usage

# ---- Source files ----
SRCS = main.c \
       drivers/adc.c \
       drivers/mux.c \
       drivers/sx1262.c \
       drivers/onewire.c \
       drivers/i2c_pmic.c \
       drivers/gape.c

OBJS = $(SRCS:.c=.o)

# ---- Target ----
TARGET = musselwatch

.PHONY: all clean flash dump size

all: $(TARGET).elf $(TARGET).bin $(TARGET).hex

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET).elf: $(OBJS) startup.o linker.ld
	$(CC) $(LDFLAGS) -o $@ $(OBJS) startup.o -lc -lm

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

# ---- Flash via OpenOCD / ST-Link ----
flash: $(TARGET).bin
	openocd -f interface/stlink.cfg -f target/stm32l4x.cfg \
	  -c "program $(TARGET).bin verify reset exit 0x08000000"

# ---- Disassembly ----
dump: $(TARGET).elf
	$(OBJDUMP) -dS $(TARGET).elf > $(TARGET).lst

# ---- Size summary ----
size: $(TARGET).elf
	$(SIZE) $(TARGET).elf

clean:
	rm -f $(OBJS) startup.o $(TARGET).elf $(TARGET).bin $(TARGET).hex \
	      $(TARGET).map $(TARGET).lst