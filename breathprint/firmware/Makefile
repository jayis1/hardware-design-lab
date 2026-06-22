# Makefile — BreathPrint firmware build
# BreathPrint — Portable Breath VOC Analyzer
#
# Author: jayis1
# Copyright (c) 2026 jayis1
# SPDX-License-Identifier: MIT
#
# Build: make
# Clean: make clean
# Flash: make flash (requires OpenOCD + ST-Link)

# ========================================================================
# Toolchain
# ========================================================================

CROSS   = arm-none-eabi-
CC      = $(CROSS)gcc
LD      = $(CROSS)gcc
OBJCOPY = $(CROSS)objcopy
OBJDUMP = $(CROSS)objdump
SIZE    = $(CROSS)size

# ========================================================================
# Target
# ========================================================================

MCU     = cortex-m7
FPU     = fpv5-sp-d16
FLOAT_ABI = hard

TARGET  = breathprint

# ========================================================================
# Paths
# ========================================================================

BUILD_DIR = build
SRC_DIR   = .
DRV_DIR   = drivers

# ========================================================================
# Source files
# ========================================================================

C_SOURCES = \
    $(SRC_DIR)/main.c \
    $(DRV_DIR)/i2c_drv.c \
    $(DRV_DIR)/adc_drv.c \
    $(DRV_DIR)/sensor_array.c \
    $(DRV_DIR)/ble.c \
    $(DRV_DIR)/sdlog.c \
    $(DRV_DIR)/display.c \
    $(DRV_DIR)/ml_infer.c \
    $(DRV_DIR)/breath_state.c \
    $(DRV_DIR)/power.c \
    $(DRV_DIR)/flash_drv.c

# ========================================================================
# Include paths
# ========================================================================

C_INCLUDES = \
    -I$(SRC_DIR) \
    -I$(DRV_DIR)

# ========================================================================
# Compiler flags
# ========================================================================

CFLAGS = -mcpu=$(MCU) -mfpu=$(FPU) -mfloat-abi=$(FLOAT_ABI) -mthumb
CFLAGS += $(C_INCLUDES)
CFLAGS += -std=c11 -Wall -Wextra -Wno-unused-parameter
CFLAGS += -Os -g3 -ffunction-sections -fdata-sections
CFLAGS += -fno-common -fno-exceptions -fomit-frame-pointer
CFLAGS += -DSTM32H733 -DUSE_FULL_LL_DRIVER
CFLAGS += --specs=nano.specs --specs=nosys.specs

# ========================================================================
# Linker flags
# ========================================================================

LDSCRIPT = linker.ld
LDFLAGS = -mcpu=$(MCU) -mfpu=$(FPU) -mfloat-abi=$(FLOAT_ABI) -mthumb
LDFLAGS += -T$(LDSCRIPT)
LDFLAGS += -Wl,--gc-sections -Wl,--print-memory-usage
LDFLAGS += --specs=nano.specs --specs=nosys.specs
LDFLAGS += -Wl,-Map=$(BUILD_DIR)/$(TARGET).map

# ========================================================================
# Objects
# ========================================================================

OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(C_SOURCES))

# ========================================================================
# Default target
# ========================================================================

all: $(BUILD_DIR) $(TARGET).elf $(TARGET).bin $(TARGET).hex

# ========================================================================
# Build rules
# ========================================================================

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
	mkdir -p $(BUILD_DIR)/drivers

$(BUILD_DIR)/%.o: %.c
	@echo "  CC  $<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(TARGET).elf: $(OBJECTS)
	@echo "  LD  $@"
	@$(LD) $(LDFLAGS) $(OBJECTS) -o $(BUILD_DIR)/$(TARGET).elf
	@$(SIZE) $(BUILD_DIR)/$(TARGET).elf

$(TARGET).bin: $(TARGET).elf
	@echo "  OBJCOPY $@"
	@$(OBJCOPY) -O binary $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).bin

$(TARGET).hex: $(TARGET).elf
	@echo "  OBJCOPY $@"
	@$(OBJCOPY) -O ihex $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex

# ========================================================================
# Flash (OpenOCD + ST-Link)
# ========================================================================

flash: $(TARGET).elf
	openocd -f interface/stlink.cfg -f target/stm32h7x.cfg \
		-c "program $(BUILD_DIR)/$(TARGET).elf verify reset exit"

# ========================================================================
# Debug
# ========================================================================

debug: $(TARGET).elf
	gdb-multiarch $(BUILD_DIR)/$(TARGET).elf \
		-ex "target remote :3333" \
		-ex "load" \
		-ex "monitor reset halt"

# ========================================================================
# Analysis
# ========================================================================

disasm: $(TARGET).elf
	$(OBJDUMP) -d $(BUILD_DIR)/$(TARGET).elf > $(BUILD_DIR)/$(TARGET).dis

size: $(TARGET).elf
	$(SIZE) -A $(BUILD_DIR)/$(TARGET).elf

# ========================================================================
# Clean
# ========================================================================

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean flash debug disasm size