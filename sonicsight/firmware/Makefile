###############################################################################
# @file    Makefile
# @brief   SonicSight — Firmware Build System.
#          Cross-compilation for STM32H743ZI (Cortex-M7, single-precision FPU).
# @author  jayis1
# @copyright © 2026 jayis1. All rights reserved.
# @license GPL-2.0
#
# Requirements:
#   - GNU ARM Embedded Toolchain (gcc-arm-none-eabi >= 10.3-2021.10)
#     https://developer.arm.com/tools-and-software/open-source-software/
#     developer-tools/gnu-toolchain/gnu-rm
#   - OpenOCD >= 0.11 (for flash via ST-Link)
#   - GNU Make >= 4.3
#
# Usage:
#   make              — Build firmware ELF + HEX + BIN
#   make clean        — Remove build artifacts
#   make flash        — Flash via ST-Link (OpenOCD)
#   make dfu          — Flash via USB DFU (DFU-util)
#   make size         — Show section sizes
#   make all          — Default target (build)
###############################################################################

# ========================================================================== #
#  Project Settings                                                          #
# ========================================================================== #

TARGET       := sonicsight
PROJECT_DIR  := .
BUILD_DIR    := build
OUTPUT_DIR   := $(BUILD_DIR)/out

# ========================================================================== #
#  Toolchain                                                                 #
# ========================================================================== #

CC           := arm-none-eabi-gcc
CXX          := arm-none-eabi-g++
AS           := arm-none-eabi-as
LD           := arm-none-eabi-gcc
OBJCOPY      := arm-none-eabi-objcopy
OBJDUMP      := arm-none-eabi-objdump
SIZE         := arm-none-eabi-size
GDB          := arm-none-eabi-gdb

# ========================================================================== #
#  MCU Configuration                                                         #
# ========================================================================== #

MCU_SERIES   := stm32h7xx
MCU_FAMILY   := STM32H743xx
CPU          := -mcpu=cortex-m7
FPU          := -mfpu=fpv5-d16 -mfloat-abi=hard
MCU_FLAGS    := $(CPU) -mthumb $(FPU) -D$(MCU_FAMILY)

# ========================================================================== #
#  Source Files                                                              #
# ========================================================================== #

C_SOURCES := \
    main.c \
    acquisition.c \
    crosscorr.c \
    tomography.c \
    calibration.c \
    ble.c \
    display.c \
    sdlog.c

# CMSIS-DSP library source (simplified — in production, link prebuilt lib)
C_SOURCES += \
    $(STM_CUBE_DIR)/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal.c \
    $(STM_CUBE_DIR)/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_cortex.c \
    $(STM_CUBE_DIR)/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rcc.c \
    $(STM_CUBE_DIR)/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_gpio.c \
    $(STM_CUBE_DIR)/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_spi.c \
    $(STM_CUBE_DIR)/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_i2c.c \
    $(STM_CUBE_DIR)/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_uart.c \
    $(STM_CUBE_DIR)/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dcmi.c \
    $(STM_CUBE_DIR)/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dma.c \
    $(STM_CUBE_DIR)/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_sdram.c \
    $(STM_CUBE_DIR)/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_tim.c \
    $(STM_CUBE_DIR)/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pwr.c \
    $(STM_CUBE_DIR)/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pwr_ex.c \
    $(STM_CUBE_DIR)/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_flash.c \
    $(STM_CUBE_DIR)/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rcc_ex.c

C_SOURCES := $(addprefix $(PROJECT_DIR)/, $(C_SOURCES))

# Assembly startup file
ASM_SOURCES := startup_stm32h743xx.s

# ========================================================================== #
#  Include Paths                                                             #
# ========================================================================== #

C_INCLUDES := \
    -I$(PROJECT_DIR) \
    -I$(STM_CUBE_DIR)/Drivers/CMSIS/Device/ST/STM32H7xx/Include \
    -I$(STM_CUBE_DIR)/Drivers/CMSIS/Include \
    -I$(STM_CUBE_DIR)/Drivers/STM32H7xx_HAL_Driver/Inc \
    -I$(STM_CUBE_DIR)/Drivers/STM32H7xx_HAL_Driver/Inc/Legacy \
    -I$(STM_CUBE_DIR)/Middlewares/ST/ARM/DSP/Inc

# ========================================================================== #
#  Linker Script                                                             #
# ========================================================================== #

LDSCRIPT := stm32h743zi_flash.ld

# ========================================================================== #
#  Compiler & Linker Flags                                                   #
# ========================================================================== #

# C flags
CFLAGS := $(MCU_FLAGS) \
    -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare \
    -O2 -ffunction-sections -fdata-sections \
    -g -gdwarf-3 \
    -DUSE_HAL_DRIVER -D$(MCU_FAMILY) \
    -std=gnu11

# Linker flags
LDFLAGS := $(MCU_FLAGS) \
    -T$(LDSCRIPT) \
    -Wl,--gc-sections -Wl,--no-warn-rwx-segments \
    -Wl,-Map=$(OUTPUT_DIR)/$(TARGET).map,--cref \
    -specs=nosys.specs -specs=nano.specs \
    -lc -lm -lnosys \
    -u _printf_float

# ========================================================================== #
#  Object Files                                                              #
# ========================================================================== #

C_OBJS := $(patsubst $(PROJECT_DIR)/%.c, $(BUILD_DIR)/%.o, $(C_SOURCES))
ASM_OBJS := $(patsubst %.s, $(BUILD_DIR)/%.o, $(ASM_SOURCES))
ALL_OBJS := $(C_OBJS) $(ASM_OBJS)

# ========================================================================== #
#  Rules                                                                     #
# ========================================================================== #

.PHONY: all clean flash dfu size directories

all: directories $(OUTPUT_DIR)/$(TARGET).elf $(OUTPUT_DIR)/$(TARGET).hex \
     $(OUTPUT_DIR)/$(TARGET).bin

directories:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(OUTPUT_DIR)
	@mkdir -p $(sort $(dir $(C_OBJS) $(ASM_OBJS)))

# Link
$(OUTPUT_DIR)/$(TARGET).elf: $(ALL_OBJS)
	@echo "  LD    $@"
	$(LD) $(LDFLAGS) $^ -o $@
	$(SIZE) $@

# HEX
$(OUTPUT_DIR)/$(TARGET).hex: $(OUTPUT_DIR)/$(TARGET).elf
	@echo "  OBJCOPY $@"
	$(OBJCOPY) -O ihex $< $@

# BIN
$(OUTPUT_DIR)/$(TARGET).bin: $(OUTPUT_DIR)/$(TARGET).elf
	@echo "  OBJCOPY $@"
	$(OBJCOPY) -O binary $< $@

# Compile C sources
$(BUILD_DIR)/%.o: $(PROJECT_DIR)/%.c
	@echo "  CC    $<"
	$(CC) $(CFLAGS) $(C_INCLUDES) -c $< -o $@

# Compile assembly
$(BUILD_DIR)/%.o: %.s
	@echo "  AS    $<"
	$(AS) $(MCU_FLAGS) -c $< -o $@

# ========================================================================== #
#  Utilities                                                                 #
# ========================================================================== #

clean:
	@echo "  CLEAN"
	rm -rf $(BUILD_DIR)

size: $(OUTPUT_DIR)/$(TARGET).elf
	$(SIZE) --format=berkeley $(OUTPUT_DIR)/$(TARGET).elf

flash: $(OUTPUT_DIR)/$(TARGET).bin
	@echo "  FLASH via ST-Link"
	openocd -f interface/stlink.cfg \
	        -f target/stm32h7x.cfg \
	        -c "program $(OUTPUT_DIR)/$(TARGET).bin 0x08000000 verify reset exit"

dfu: $(OUTPUT_DIR)/$(TARGET).bin
	@echo "  FLASH via USB DFU"
	dfu-util -d 0483:DF11 -a 0 -s 0x08000000:leave -D $(OUTPUT_DIR)/$(TARGET).bin

gdb: $(OUTPUT_DIR)/$(TARGET).elf
	$(GDB) -ex "target remote localhost:3333" $<

# Display compiler version
toolchain-check:
	@echo "Toolchain:"
	$(CC) --version | head -1

# ========================================================================== #
#  Dependencies                                                              #
# ========================================================================== #

# Automatic dependency generation
DEP_FLAGS = -MMD -MP -MF $(@:.o=.d)
CFLAGS += $(DEP_FLAGS)

-include $(ALL_OBJS:.o=.d)