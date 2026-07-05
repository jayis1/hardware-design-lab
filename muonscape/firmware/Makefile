# Makefile — MûonScape firmware
# Author: jayis1
# Copyright (c) 2026 jayis1. All rights reserved.
# License: MIT
#
# MûonScape: Portable Cosmic-Ray Muon Tomography Imager
# Target: STM32H723VIT6 (Cortex-M7, 550 MHz, 1 MB Flash)
# Toolchain: arm-none-eabi-gcc
#
# Usage:
#   make            — build muonscape.elf
#   make flash       — flash via OpenOCD / ST-Link
#   make clean       — remove build artifacts

# ---- Toolchain ----
CC       = arm-none-eabi-gcc
OBJCOPY  = arm-none-eabi-objcopy
OBJDUMP  = arm-none-eabi-objdump
SIZE     = arm-none-eabi-size
OPENOCD  = openocd

# ---- MCU flags ----
MCU      = -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard
MCU     += -mfp16-format=ieee

# ---- Include paths ----
INCLUDES = -I. \
           -Idrivers \
           -I$(HAL_DIR)/Inc \
           -I$(CMSIS_DIR)/Include \
           -I$(CMSIS_DSP_DIR)/Include

# ---- C flags ----
CFLAGS   = $(MCU) $(INCLUDES) -std=c11 -Wall -Wextra -Wno-unused-parameter
CFLAGS  += -O2 -ffunction-sections -fdata-sections -fno-common
CFLAGS  += -DSTM32H723xx -DUSE_HAL_DRIVER -DARM_MATH_CM7
CFLAGS  += -g3 -gdwarf-2

# ---- Linker flags ----
LDFLAGS  = $(MCU) -T$(LDSCRIPT) -Wl,--gc-sections -Wl,-Map=$(BUILD)/muonscape.map
LDFLAGS += -lm -lc --specs=nano.specs --specs=nosys.specs

# ---- Directories ----
BUILD    = build
HAL_DIR  = Drivers/STM32H7xx_HAL_Driver
CMSIS_DIR = Drivers/CMSIS/Device/ST/STM32H7xx/Include
CMSIS_DSP_DIR = Drivers/CMSIS/DSP
LDSCRIPT = STM32H723VITx_FLASH.ld

# ---- Sources ----
SRCS = main.c
DRIVER_SRCS = drivers/i2c.c drivers/fpga.c drivers/sipm.c drivers/asic.c \
              drivers/track.c drivers/fbp.c drivers/event.c \
              drivers/wifi.c drivers/ble.c drivers/gps.c \
              drivers/sensors.c drivers/power.c drivers/storage.c \
              drivers/command.c drivers/tdc.c
ASM_SRCS = startup.s

OBJS = $(patsubst %.c,$(BUILD)/%.o,$(SRCS)) \
       $(patsubst %.c,$(BUILD)/%.o,$(DRIVER_SRCS)) \
       $(patsubst %.s,$(BUILD)/%.o,$(ASM_SRCS))

# ---- Targets ----
TARGET = muonscape

.PHONY: all flash clean size

all: $(BUILD)/$(TARGET).elf $(BUILD)/$(TARGET).hex $(BUILD)/$(TARGET).bin size

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: %.s
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/$(TARGET).elf: $(OBJS) $(LDSCRIPT)
	$(CC) $(LDFLAGS) $(OBJS) -o $@

$(BUILD)/$(TARGET).hex: $(BUILD)/$(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

$(BUILD)/$(TARGET).bin: $(BUILD)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

size: $(BUILD)/$(TARGET).elf
	$(SIZE) $<

flash: $(BUILD)/$(TARGET).hex
	$(OPENOCD) -f interface/stlink.cfg -f target/stm32h7x.cfg \
	    -c "program $< verify reset exit"

clean:
	rm -rf $(BUILD)