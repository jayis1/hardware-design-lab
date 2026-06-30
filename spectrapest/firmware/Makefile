# Makefile — SpectraPest firmware build
#
# Author: jayis1
# Copyright (c) 2026 jayis1. All rights reserved.
#
# Builds the SpectraPest firmware for the STM32H753VIT6 MCU.
# Uses the GCC ARM Embedded toolchain (arm-none-eabi-gcc).

# Toolchain
CC      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE    = arm-none-eabi-size
OBJDUMP = arm-none-eabi-objdump
GDB     = arm-none-eabi-gdb

# Target MCU
MCU     = -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard

# Compiler flags
CFLAGS  = $(MCU) -std=c11 -Wall -Wextra -Os -g3
CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += -fno-common -fno-builtin -fno-exceptions
CFLAGS += -DSTM32H753xx -DUSE_FULL_LL_DRIVER
CFLAGS += -I. -Idrivers

# Linker flags
LDFLAGS = $(MCU) -T link.ld -Wl,--gc-sections -Wl,-Map=firmware.map
LDFLAGS += -Wl,--print-memory-usage

# Source files
SRCS = main.c
SRCS += drivers/imx519.c
SRCS += drivers/acoustic.c
SRCS += drivers/spectral.c
SRCS += drivers/edgeai.c
SRCS += drivers/environment.c
SRCS += drivers/loramesh.c
SRCS += drivers/gnss.c
SRCS += drivers/storage.c
SRCS += drivers/power.c

# Object files
OBJS = $(SRCS:.c=.o)

# Default target
all: firmware.elf firmware.bin firmware.hex

# Compile C sources
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Link
firmware.elf: $(OBJS) link.ld
	$(CC) $(LDFLAGS) $(OBJS) -o $@
	$(SIZE) $@

# Binary
firmware.bin: firmware.elf
	$(OBJCOPY) -O binary $< $@

# Hex
firmware.hex: firmware.elf
	$(OBJCOPY) -O ihex $< $@

# Disassembly
disasm: firmware.elf
	$(OBJDUMP) -d -S firmware.elf > firmware.dis

# Flash via OpenOCD + ST-Link
flash: firmware.elf
	openocd -f interface/stlink.cfg -f target/stm32h7x.cfg \
		-c "program firmware.elf verify reset exit"

# Debug
debug: firmware.elf
	openocd -f interface/stlink.cfg -f target/stm32h7x.cfg &
	$(GDB) firmware.elf -ex "target remote :3333" -ex "load"

# Clean
clean:
	rm -f $(OBJS) firmware.elf firmware.bin firmware.hex firmware.map firmware.dis

# Dependencies
deps:
	$(CC) $(CFLAGS) -MM $(SRCS) > deps.d

-include deps.d

.PHONY: all clean flash debug disasm deps