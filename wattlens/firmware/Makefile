# =============================================================================
# Makefile — WattLens firmware build
# WattLens — 3-Phase Power Quality Analyzer & NILM
#
# Author: jayis1
# Copyright (c) 2026 jayis1
# SPDX-License-Identifier: MIT
#
# Build: make          → firmware.elf
#        make flash    → flash via OpenOCD / ST-Link
#        make clean
# =============================================================================

PROJECT    := wattlens
TARGET     := $(PROJECT).elf
HEX        := $(PROJECT).hex
BIN        := $(PROJECT).bin

# --- Toolchain ---
CC          := arm-none-eabi-gcc
CXX         := arm-none-eabi-g++
OBJCOPY     := arm-none-eabi-objcopy
SIZE        := arm-none-eabi-size
OPENOCD     := openocd

# --- MCU flags ---
MCU         := -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard
MCU_DEFS    := -DSTM32H743xx -DUSE_FULL_LL_DRIVER -DARM_MATH_CM7 \
               -D__FPU_PRESENT=1 -DHSE_VALUE=16000000UL

# --- Include paths ---
INCLUDES    := -I. -Idrivers -I$(CMSIS_DIR) -I$(CMSIS_DSP_DIR)

# --- Compiler flags ---
CFLAGS      := $(MCU) $(MCU_DEFS) $(INCLUDES) \
               -Os -g3 -Wall -Wextra -Wno-unused-parameter \
               -ffunction-sections -fdata-sections \
               -fno-common -fno-exceptions -std=gnu11

CXXFLAGS    := $(CFLAGS) -std=gnu++17 -fno-rtti

LDFLAGS     := $(MCU) -T linker.ld -Wl,--gc-sections -Wl,-Map=$(PROJECT).map \
               --specs=nano.specs --specs=nosys.specs -lc -lm

# --- Source files ---
SRC_DIR     := . drivers
C_SOURCES   := $(foreach d,$(SRC_DIR),$(wildcard $(d)/*.c))

# --- Object files ---
OBJ_DIR     := build
OBJS        := $(patsubst %.c,$(OBJ_DIR)/%.o,$(C_SOURCES))

# --- CMSIS paths (set to your local install) ---
CMSIS_DIR     ?= /usr/local/CMSIS/Include
CMSIS_DSP_DIR ?= /usr/local/CMSIS/DSP/Include

# =============================================================================
# Rules
# =============================================================================

.PHONY: all flash clean size

all: $(TARGET) $(HEX) $(BIN) size

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)
	mkdir -p $(OBJ_DIR)/drivers

$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	@echo "  CC  $<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS) linker.ld
	@echo "  LD  $@"
	@$(CC) $(LDFLAGS) $(OBJS) -o $@

$(HEX): $(TARGET)
	@echo "  OBJCOPY $@"
	@$(OBJCOPY) -O ihex $< $@

$(BIN): $(TARGET)
	@echo "  OBJCOPY $@"
	@$(OBJCOPY) -O binary $< $@

size: $(TARGET)
	@$(SIZE) $(TARGET)

flash: $(BIN)
	@echo "  FLASH $<"
	@$(OPENOCD) -f interface/stlink.cfg -f target/stm32h7x.cfg \
	  -c "program $(BIN) verify reset exit 0x08000000"

clean:
	rm -rf $(OBJ_DIR) $(TARGET) $(HEX) $(BIN) $(PROJECT).map

# =============================================================================
# Dependency tracking
# =============================================================================
DEPS := $(OBJS:.o=.d)
-include $(DEPS)

$(OBJ_DIR)/%.d: %.c | $(OBJ_DIR)
	@$(CC) $(CFLAGS) -MM -MT $@ -MF $@ $<

# Author: jayis1
# SPDX-License-Identifier: MIT