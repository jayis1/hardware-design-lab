# AeroCast firmware Makefile
# Author: jayis1
# Copyright (C) 2026 jayis1
#
# Build with arm-none-eabi-gcc (no CMSIS dependency — the project ships
# its own minimal register helpers in registers.h). The startup file
# and linker script live in the platform/ directory (not included in
# this review bundle but present in the full release).

CROSS   ?= arm-none-eabi-
CC       = $(CROSS)gcc
OBJCOPY  = $(CROSS)objcopy
SIZE     = $(CROSS)size

MCU      = -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard
CFLAGS   = $(MCU) -std=c11 -Os -Wall -Wextra -ffunction-sections -fdata-sections \
           -g3 -I. -Idrivers \
           -DFW_VERSION_MAJOR=1 -DFW_VERSION_MINOR=0 -DFW_VERSION_PATCH=0 \
           -DFW_AUTHOR=\\\"jayis1\\\" -DCOPYRIGHT=\\\"jayis1\\\"
LDFLAGS  = $(MCU) -T platform/stm32h7a3.ld --specs=nano.specs --specs=nosys.specs \
           -Wl,--gc-sections -Wl,-Map=aerocast.map

SRCS = main.c \
       drivers/optics.c drivers/flow.c drivers/afe.c \
       drivers/classify.c drivers/sensors.c drivers/comms.c \
       drivers/display.c drivers/storage.c

OBJS = $(SRCS:.c=.o)
TARGET = aerocast.elf

.PHONY: all clean size

all: $(TARGET) aerocast.bin aerocast.hex size

$(TARGET): $(OBJS) platform/startup_stm32h7a3.o platform/system_stm32h7a3.o
	$(CC) $(LDFLAGS) -o $@ $^ -lm

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

aerocast.bin: $(TARGET)
	$(OBJCOPY) -O binary $< $@

aerocast.hex: $(TARGET)
	$(OBJCOPY) -O ihex $< $@

size: $(TARGET)
	$(SIZE) $(TARGET)

clean:
	rm -f $(OBJS) platform/*.o $(TARGET) aerocast.bin aerocast.hex aerocast.map