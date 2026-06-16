# Terramesh Geotechnical Node — Firmware Makefile
# Author: jayis1
# Copyright © 2026 jayis1
# License: GPL-2.0
#
# Builds firmware for STM32U5A5ZJT6Q (Cortex-M33)
# Requires: arm-none-eabi-gcc, openocd, stlink

# Toolchain
PREFIX    := arm-none-eabi-
CC        := $(PREFIX)gcc
LD        := $(PREFIX)gcc
OBJCOPY   := $(PREFIX)objcopy
OBJDUMP   := $(PREFIX)objdump
SIZE      := $(PREFIX)size
GDB       := $(PREFIX)gdb

# Target MCU
MCU       := STM32U5A5ZJT6Q
CPU       := cortex-m33
FPU       := fpv5-sp-d16
FLOAT_ABI := hard

# Directories
SRC_DIR   := .
DRV_DIR   := drivers
BUILD_DIR := build

# Source files
SRCS := \
	main.c \
	drivers/sx1262.c \
	drivers/scl3300.c \
	drivers/ads1120.c \
	drivers/bme688.c

# Object files
OBJS := $(SRCS:%.c=$(BUILD_DIR)/%.o)

# Linker script
LDSCRIPT := stm32u5a5zjt6q.ld

# Compiler flags
CFLAGS := \
	-mcpu=$(CPU) \
	-mfpu=$(FPU) \
	-mfloat-abi=$(FLOAT_ABI) \
	-mthumb \
	-ffreestanding \
	-fno-common \
	-fdata-sections \
	-ffunction-sections \
	-Wall \
	-Wextra \
	-Werror=implicit-function-declaration \
	-Werror=return-type \
	-Werror=unused-variable \
	-Werror=unused-function \
	-Wno-unused-parameter \
	-O2 \
	-g \
	-D$(MCU) \
	-DUSE_HAL_DRIVER \
	-DSTM32U5A5xx \
	-I. \
	-Idrivers

# Linker flags
LDFLAGS := \
	-mcpu=$(CPU) \
	-mfpu=$(FPU) \
	-mfloat-abi=$(FLOAT_ABI) \
	-mthumb \
	-ffreestanding \
	-nostartfiles \
	-Wl,--gc-sections \
	-Wl,-Map=$(BUILD_DIR)/terramesh.map \
	-T$(LDSCRIPT)

# Default target
all: $(BUILD_DIR)/terramesh.elf $(BUILD_DIR)/terramesh.bin $(BUILD_DIR)/terramesh.hex

# Compile C source
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

# Link
$(BUILD_DIR)/terramesh.elf: $(OBJS) $(LDSCRIPT)
	$(LD) $(LDFLAGS) -o $@ $(OBJS)
	$(SIZE) $@

# Generate binary
$(BUILD_DIR)/terramesh.bin: $(BUILD_DIR)/terramesh.elf
	$(OBJCOPY) -O binary $< $@

# Generate hex
$(BUILD_DIR)/terramesh.hex: $(BUILD_DIR)/terramesh.elf
	$(OBJCOPY) -O ihex $< $@

# Generate disassembly
$(BUILD_DIR)/terramesh.lst: $(BUILD_DIR)/terramesh.elf
	$(OBJDUMP) -S $< > $@

# Flash via ST-Link
flash: $(BUILD_DIR)/terramesh.bin
	openocd -f interface/stlink.cfg \
		-f target/stm32u5x.cfg \
		-c "program $(BUILD_DIR)/terramesh.bin 0x08000000 verify reset exit"

# Flash via DFU (USB DFU mode)
dfu: $(BUILD_DIR)/terramesh.bin
	dfu-util -a 0 -s 0x08000000:leave -D $(BUILD_DIR)/terramesh.bin

# Debug with GDB
debug: $(BUILD_DIR)/terramesh.elf
	openocd -f interface/stlink.cfg \
		-f target/stm32u5x.cfg \
		-c "gdb_port 3333" &
	$(GDB) -ex "target remote localhost:3333" \
		-ex "monitor reset halt" \
		-ex "load" \
		-ex "continue" \
		$(BUILD_DIR)/terramesh.elf

# Clean
clean:
	rm -rf $(BUILD_DIR)

# Size report
size: $(BUILD_DIR)/terramesh.elf
	$(SIZE) $<

# Dependencies
$(BUILD_DIR)/main.o: board.h registers.h
$(BUILD_DIR)/drivers/sx1262.o: board.h registers.h
$(BUILD_DIR)/drivers/scl3300.o: board.h registers.h
$(BUILD_DIR)/drivers/ads1120.o: board.h registers.h
$(BUILD_DIR)/drivers/bme688.o: board.h registers.h

.PHONY: all clean flash dfu debug size
