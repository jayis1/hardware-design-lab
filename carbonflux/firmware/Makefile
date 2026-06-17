# CarbonFlux Firmware Makefile
# Author: jayis1
# Copyright (c) 2026 jayis1. All rights reserved.
# License: GPL-2.0
#
# Toolchain: arm-none-eabi-gcc 12.x
# Target:    STM32U5A9ZG (Cortex-M33)
# Build:     make          → Release build (optimized for size)
#            make DEBUG=1  → Debug build (no optimization, debug symbols)
#            make clean    → Remove build artifacts
#            make flash    → Flash via ST-Link (st-flash)
#            make dfu      → Flash via USB DFU (dfu-util)

# --- Toolchain ---
CROSS_COMPILE ?= arm-none-eabi-
CC   := $(CROSS_COMPILE)gcc
LD   := $(CROSS_COMPILE)gcc
AR   := $(CROSS_COMPILE)ar
AS   := $(CROSS_COMPILE)as
OBJCOPY := $(CROSS_COMPILE)objcopy
OBJDUMP := $(CROSS_COMPILE)objdump
SIZE  := $(CROSS_COMPILE)size

# --- Target ---
MCU      := cortex-m33
TARGET   := carbonflux
FLASH_BASE := 0x08008000  # Application start (after 16 kB bootloader)

# --- Directories ---
BUILD_DIR  := build
DRV_DIR    := drivers
INC_DIRS   := . $(DRV_DIR)
SRC_DIRS   := . $(DRV_DIR)

# --- Sources ---
SRCS := main.c \
        $(DRV_DIR)/scd41.c \
        $(DRV_DIR)/dps310.c \
        $(DRV_DIR)/ds18b20.c \
        $(DRV_DIR)/sx1262.c \
        $(DRV_DIR)/w25q128.c \
        $(DRV_DIR)/rv8803.c \
        $(DRV_DIR)/servo.c \
        $(DRV_DIR)/adc.c \
        $(DRV_DIR)/flux_engine.c \
        $(DRV_DIR)/power.c

# Startup + linker
STARTUP_S ?= startup_stm32u5a9.s
LINKER_SCRIPT ?= linker.ld

OBJS := $(SRCS:%.c=$(BUILD_DIR)/%.o)
OBJS += $(BUILD_DIR)/$(STARTUP_S:%.s=%.o)

# --- Include paths ---
INCLUDES := $(addprefix -I,$(INC_DIRS))

# --- Compiler flags ---
CFLAGS := -mcpu=$(MCU) -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16
CFLAGS += -Wall -Wextra -Wdouble-promotion -Wno-unused-parameter
CFLAGS += -ffunction-sections -fdata-sections -fstack-usage
CFLAGS += -DSTM32U5A9Zx -DUSE_HAL_DRIVER
CFLAGS += $(INCLUDES)

ifdef DEBUG
CFLAGS += -Og -g3 -DDEBUG
else
CFLAGS += -Os -flto -g2
endif

# --- Linker flags ---
LDFLAGS := -mcpu=$(MCU) -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16
LDFLAGS += -specs=nano.specs -specs=nosys.specs
LDFLAGS += -Wl,--gc-sections -Wl,-Map=$(BUILD_DIR)/$(TARGET).map
LDFLAGS += -Wl,--script=$(LINKER_SCRIPT)
LDFLAGS += -Wl,--defsym=__flash_base=$(FLASH_BASE)

# --- Build rules ---
.PHONY: all clean flash dfu size

all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).bin $(BUILD_DIR)/$(TARGET).hex

# Compile C sources
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Assemble startup
$(BUILD_DIR)/startup_stm32u5a9.o: $(STARTUP_S)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Link
$(BUILD_DIR)/$(TARGET).elf: $(OBJS) $(LINKER_SCRIPT)
	$(LD) $(LDFLAGS) $(OBJS) -o $@
	$(SIZE) $@

# Binary output
$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(BUILD_DIR)/$(TARGET).hex: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

# --- Utilities ---
size: $(BUILD_DIR)/$(TARGET).elf
	$(SIZE) $<
	@echo "=== Section Sizes ==="
	$(OBJDUMP) -h $(BUILD_DIR)/$(TARGET).elf

flash: $(BUILD_DIR)/$(TARGET).bin
	st-flash --reset write $(BUILD_DIR)/$(TARGET).bin $(FLASH_BASE)

dfu: $(BUILD_DIR)/$(TARGET).bin
	dfu-util -a 0 -d 0483:df11 -s $(FLASH_BASE):leave -D $(BUILD_DIR)/$(TARGET).bin

clean:
	rm -rf $(BUILD_DIR)

# --- Dependencies ---
-include $(OBJS:.o=.d)

# --- Information ---
info:
	@echo "CarbonFlux Firmware Build"
	@echo "  MCU:            STM32U5A9ZG (Cortex-M33)"
	@echo "  Clock:          160 MHz"
	@echo "  Flash base:     0x08008000"
	@echo "  Compiler:       $(shell $(CC) --version | head -1)"
	@echo "  Flags:          $(CFLAGS)"