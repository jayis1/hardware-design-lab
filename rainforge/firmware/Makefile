#
# Makefile — RainForge firmware build
# Author: jayis1
# Copyright (C) 2026 jayis1
#
# Builds the RainForge firmware for the ESP32-C3 using the ESP-IDF
# standalone toolchain (riscv32-esp-elf-gcc). In a typical project
# this would be driven by `idf.py build`, but this Makefile provides
# a portable, self-contained build for environments without the full
# ESP-IDF framework.
#
# Usage:
#   make            # build rainforge.elf
#   make flash      # flash via esptool.py
#   make clean
#   make size       # print size summary
#

# ---- Toolchain ----
CC      = riscv32-esp-elf-gcc
OBJCOPY = riscv32-esp-elf-objcopy
SIZE    = riscv32-esp-elf-size

# ---- Target MCU: ESP32-C3 (RISC-V single-core, 160 MHz) ----
ARCH_FLAGS = -march=rv32imc -mabi=ilp32 -mdiv -mno-save-restore
CPU_FLAGS  = -DCPU_FREQ_HZ=160000000UL

# ---- Include paths ----
INCLUDES = -I. -Idrivers

# ---- Compiler flags ----
CFLAGS = $(ARCH_FLAGS) $(CPU_FLAGS) $(INCLUDES) \
         -Os -g -std=c11 -Wall -Wextra -Wno-unused-parameter \
         -ffunction-sections -fdata-sections \
         -fno-builtin -fno-common \
         -DESP32_C3=1

LDFLAGS = $(ARCH_FLAGS) -nostartfiles -Wl,--gc-sections -Wl,-Map=rainforge.map

# ---- Source files ----
SRCDIRS = . drivers
SRCS    = $(foreach d,$(SRCDIRS),$(wildcard $(d)/*.c))
OBJS    = $(SRCS:.c=.o)
DEPS    = $(OBJS:.o=.d)

# ---- Output ----
TARGET  = rainforge.elf
BIN     = rainforge.bin

# ---- ESP-IDF paths (override with env if needed) ----
IDF_PATH  ?= $(HOME)/esp/esp-idf
ESPTOOL   ?= $(IDF_PATH)/components/esptool_py/esptool/esptool.py
FLASH_PORT ?= /dev/ttyUSB0
FLASH_BAUD ?= 460800

# ---- Build rules ----
.PHONY: all clean flash size

all: $(TARGET) $(BIN)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) -lm
	$(SIZE) $@

$(BIN): $(TARGET)
	$(OBJCOPY) -O binary $< $@

%.o: %.c
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

flash: $(BIN)
	python3 $(ESPTOOL) --chip esp32c3 --port $(FLASH_PORT) --baud $(FLASH_BAUD) \
		write_flash 0x0 $(BIN)

size: $(TARGET)
	$(SIZE) --format=berkeley $(TARGET)

clean:
	rm -f $(OBJS) $(DEPS) $(TARGET) $(BIN) rainforge.map

-include $(DEPS)