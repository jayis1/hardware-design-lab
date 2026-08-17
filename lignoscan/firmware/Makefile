# ============================================================
# LignoScan — Portable Acoustic Tomography Scanner
# Firmware Makefile
#
# Author: jayis1
# Copyright (C) 2026 jayis1. All rights reserved.
# License: GPL-2.0
# ============================================================

# ---- Toolchain ----
CC      = arm-none-eabi-gcc
LD      = arm-none-eabi-ld
OBJCOPY = arm-none-eabi-objcopy
OBJDUMP = arm-none-eabi-objdump
SIZE    = arm-none-eabi-size

# ---- Target MCU: STM32H733VIT6 ----
CPU     = -mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb
CFLAGS  = $(CPU) -std=c11 -O2 -Wall -Wextra -g3
CFLAGS += -DSTM32H733xx -DARM_MATH_CM7
CFLAGS += -fdata-sections -ffunction-sections
CFLAGS += -I. -Idrivers

# ---- Linker flags ----
LDFLAGS = $(CPU) -T linker.ld -Wl,--gc-sections -Wl,-Map=lignoscan.map
LDLIBS  = -lm -lc

# ---- Source files ----
SRCS = main.c
SRCS += drivers/tdc.c
SRCS += drivers/mux.c
SRCS += drivers/hv.c
SRCS += drivers/afe.c
SRCS += drivers/tomography.c
SRCS += drivers/ble.c
SRCS += drivers/gps.c
SRCS += drivers/sdlog.c
SRCS += drivers/display.c
SRCS += drivers/power.c

OBJS = $(SRCS:.c=.o)

# ---- Build targets ----
.PHONY: all clean flash

all: lignoscan.elf lignoscan.bin lignoscan.hex

lignoscan.elf: $(OBJS) linker.ld
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)
	$(SIZE) $@

lignoscan.bin: lignoscan.elf
	$(OBJCOPY) -O binary $< $@

lignoscan.hex: lignoscan.elf
	$(OBJCOPY) -O ihex $< $@

# ---- Compile rules ----
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# ---- Flash via ST-Link ----
flash: lignoscan.bin
	st-flash write lignoscan.bin 0x08000000

# ---- Disassembly ----
disasm: lignoscan.elf
	$(OBJDUMP) -d -S lignoscan.elf > lignoscan.disasm

# ---- Clean ---- */
clean:
	rm -f $(OBJS) lignoscan.elf lignoscan.bin lignoscan.hex \
	      lignoscan.map lignoscan.disasm

# ---- Dependencies ---- */
main.o: board.h registers.h drivers/tdc.h drivers/mux.h drivers/hv.h \
        drivers/afe.h drivers/tomography.h drivers/ble.h drivers/gps.h \
        drivers/sdlog.h drivers/display.h drivers/power.h

drivers/tdc.o: drivers/tdc.h board.h registers.h
drivers/mux.o: drivers/mux.h board.h
drivers/hv.o: drivers/hv.h board.h
drivers/afe.o: drivers/afe.h board.h
drivers/tomography.o: drivers/tomography.h board.h
drivers/ble.o: drivers/ble.h board.h
drivers/gps.o: drivers/gps.h board.h
drivers/sdlog.o: drivers/sdlog.h board.h
drivers/display.o: drivers/display.h board.h
drivers/power.o: drivers/power.h board.h

# EOF — Makefile
# Author: jayis1
# Copyright (C) 2026 jayis1. All rights reserved.