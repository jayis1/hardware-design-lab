# Makefile — GrainGuard firmware build
#
# Author: jayis1
# Copyright (C) 2026 jayis1. All rights reserved.
# License: GPL-2.0
#
# Builds the GrainGuard firmware for STM32WL55JCI6 using arm-none-eabi-gcc.
# No external SDK is required — bare-metal with minimal register headers.

# ---- Toolchain ----
CROSS   = arm-none-eabi-
CC      = $(CROSS)gcc
OBJCOPY = $(CROSS)objcopy
SIZE    = $(CROSS)size

# ---- MCU flags ----
MCU      = -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
INCLUDES = -I. -Idrivers
CFLAGS   = $(MCU) $(INCLUDES) -std=c11 -Wall -Wextra -O2 -g \
           -ffunction-sections -fdata-sections \
           -fno-common -fno-builtin
LDFLAGS  = $(MCU) -T linker.ld -Wl,--gc-sections -Wl,-Map=grainguard.map -lm

# ---- Source files ----
SRCDIRS = . drivers
SOURCES = main.c \
          drivers/co2.c \
          drivers/temp.c \
          drivers/humid.c \
          drivers/acoustic.c \
          drivers/emc.c \
          drivers/sri.c \
          drivers/loramesh.c \
          drivers/storage.c \
          drivers/power.c

OBJECTS = $(SOURCES:.c=.o)

# ---- Targets ----
TARGET = grainguard.elf

.PHONY: all clean flash size

all: $(TARGET)

$(TARGET): $(OBJECTS) linker.ld
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS)
	$(OBJCOPY) -O ihex $@ grainguard.hex
	$(SIZE) $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET) grainguard.hex grainguard.map

flash: $(TARGET)
	openocd -f interface/stlink.cfg -f target/stm32wlx.cfg \
	  -c "program grainguard.elf verify reset exit"

size: $(TARGET)
	$(SIZE) $(TARGET)

# ---- Dependencies ----
main.o: board.h registers.h drivers/co2.h drivers/temp.h drivers/humid.h \
        drivers/acoustic.h drivers/emc.h drivers/sri.h drivers/loramesh.h \
        drivers/storage.h drivers/power.h
drivers/co2.o: drivers/co2.h board.h registers.h
drivers/temp.o: drivers/temp.h board.h registers.h
drivers/humid.o: drivers/humid.h board.h registers.h drivers/co2.c
drivers/acoustic.o: drivers/acoustic.h board.h registers.h
drivers/emc.o: drivers/emc.h board.h
drivers/sri.o: drivers/sri.h board.h drivers/emc.h
drivers/loramesh.o: drivers/loramesh.h board.h registers.h
drivers/storage.o: drivers/storage.h board.h registers.h
drivers/power.o: drivers/power.h board.h registers.h

# Author: jayis1
# End of Makefile