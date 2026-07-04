# Makefile — GlyphFlow firmware build
# Author: jayis1
# Copyright (c) 2026 jayis1 — MIT License
#
# Builds the GlyphFlow bare-metal firmware with arm-none-eabi-gcc.
# No external HAL; direct register access. Produces ELF + bin + hex.

# ---- Toolchain ----
CC      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE    = arm-none-eabi-size

# ---- Target MCU ----
MCU     = -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
MCU_LK  = -TSTM32L432KCUX_FLASH.ld

# ---- Flags ----
CFLAGS  = $(MCU) -Os -g -std=c11 -Wall -Wextra -Wno-unused-parameter
CFLAGS += -ffunction-sections -fdata-sections -fno-common -fno-builtin
CFLAGS += -I. -Idrivers
CFLAGS += -D__ARM_ARCH_7EM__

LDFLAGS = $(MCU) $(MCU_LK) -nostartfiles
LDFLAGS += -Wl,--gc-sections -Wl,-Map=glyphflow.map -lc -lgcc -lm

# ---- Sources ----
SRCS = main.c
SRCS += drivers/imu.c drivers/trajectory.c drivers/tflite.c
SRCS += drivers/ble.c drivers/haptic.c drivers/fuel.c
SRCS += drivers/touch.c drivers/i2c.c

OBJS = $(SRCS:.c=.o)

# ---- Build rules ----
.PHONY: all clean flash size

all: glyphflow.elf glyphflow.bin glyphflow.hex size

glyphflow.elf: $(OBJS) startup.s STM32L432KCUX_FLASH.ld
	$(CC) $(LDFLAGS) -o $@ $(OBJS) startup.s

glyphflow.bin: glyphflow.elf
	$(OBJCOPY) -O binary $< $@

glyphflow.hex: glyphflow.elf
	$(OBJCOPY) -O ihex $< $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

size: glyphflow.elf
	$(SIZE) glyphflow.elf

flash: glyphflow.bin
	openocd -f interface/stlink.cfg -f target/stm32l4x.cfg \
		-c "program glyphflow.bin verify reset exit 0x08000000"

clean:
	rm -f $(OBJS) glyphflow.elf glyphflow.bin glyphflow.hex glyphflow.map