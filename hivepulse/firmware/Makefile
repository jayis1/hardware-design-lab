# Makefile — HivePulse firmware build system
#
# Target: STM32H733VGT6 (Cortex-M7 @ 280 MHz)
# Toolchain: arm-none-eabi-gcc
# Author: jayis1
# Copyright (c) 2026 jayis1. All rights reserved.
# License: GPL-3.0

# ---- Toolchain ----
CC      = arm-none-eabi-gcc
AS      = arm-none-eabi-gcc
LD      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
OBJDUMP = arm-none-eabi-objdump
SIZE    = arm-none-eabi-size

# ---- Directories ----
BUILD_DIR = build
SRC_DIR   = .

# ---- MCU Flags ----
MCU = -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard

# ---- C Flags ----
CFLAGS = $(MCU) -std=c11 -Wall -Wextra -Wno-unused-parameter \
         -O2 -ffunction-sections -fdata-sections -fno-common \
         -ffreestanding -fno-builtin \
         -DSTM32H733 -DSTM32H7_SERIES -DDEBUG_ENABLE \
         -I$(SRC_DIR)

# ---- Assembly Flags ----
ASFLAGS = $(MCU) -x assembler-with-cpp

# ---- Linker Flags ----
LDFLAGS = $(MCU) -T linker.ld \
         -Wl,--gc-sections -Wl,--print-memory-usage \
         -nostartfiles -nostdlib \
         -lc -lm -lnosys

# ---- Source Files ----
C_SOURCES = \
    main.c \
    audio.c \
    dsp.c \
    ml_model.c \
    sensors.c \
    bee_counter.c \
    lora.c \
    ble.c \
    power.c

ASM_SOURCES = \
    startup_stm32h733.s

# ---- Object Files ----
OBJECTS = $(addprefix $(BUILD_DIR)/, $(C_SOURCES:.c=.o))
OBJECTS += $(addprefix $(BUILD_DIR)/, $(ASM_SOURCES:.s=.o))

# ---- Default Target ----
all: $(BUILD_DIR)/hivepulse.elf $(BUILD_DIR)/hivepulse.bin $(BUILD_DIR)/hivepulse.hex

# ---- Compile C Sources ----
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# ---- Assemble ----
$(BUILD_DIR)/%.o: %.s
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) -c $< -o $@

# ---- Link ----
$(BUILD_DIR)/hivepulse.elf: $(OBJECTS)
	$(LD) $(LDFLAGS) $(OBJECTS) -o $@
	$(SIZE) $@

# ---- Binary Outputs ----
$(BUILD_DIR)/hivepulse.bin: $(BUILD_DIR)/hivepulse.elf
	$(OBJCOPY) -O binary $< $@

$(BUILD_DIR)/hivepulse.hex: $(BUILD_DIR)/hivepulse.elf
	$(OBJCOPY) -O ihex $< $@

# ---- Disassembly (for debugging) ----
disasm: $(BUILD_DIR)/hivepulse.elf
	$(OBJDUMP) -d $< > $(BUILD_DIR)/hivepulse.lst

# ---- Flash via OpenOCD ----
flash: $(BUILD_DIR)/hivepulse.elf
	openocd -f interface/stlink.cfg -f target/stm32h7x.cfg \
		-c "program $< verify reset exit"

# ---- Flash via ST-Link ----
stflash: $(BUILD_DIR)/hivepulse.bin
	st-flash write $< 0x08000000

# ---- Debug (GDB + OpenOCD) ----
debug: $(BUILD_DIR)/hivepulse.elf
	openocd -f interface/stlink.cfg -f target/stm32h7x.cfg &
	arm-none-eabi-gdb $< -ex "target remote :3333"

# ---- Clean ----
clean:
	rm -rf $(BUILD_DIR)

# ---- Phony Targets ----
.PHONY: all clean flash stflash debug disasm