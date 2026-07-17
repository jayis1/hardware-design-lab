# ----------------------------------------------------------------------------
# Makefile — Stridophone firmware for STM32H743VIT6
#
# Author : jayis1
# License: MIT
#
# Build:  make          -> build/stridophone.elf
#         make flash    -> openocd flash
#         make clean
#
# Toolchain: arm-none-eabi-gcc (GNU Arm Embedded 10.3+)
# ----------------------------------------------------------------------------
TARGET   = stridophone
BUILD    = build

CC       = arm-none-eabi-gcc
OBJCOPY  = arm-none-eabi-objcopy
SIZE     = arm-none-eabi-size

CPU      = -mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb
WARN     = -Wall -Wextra -Wno-unused-parameter -Wno-unused-function
OPT      = -Os -ffunction-sections -fdata-sections -g3
DEFS     = -DSTM32H743xx -DUSE_FULL_LL_DRIVER=0 -DARM_MATH_CM7
CFLAGS   = $(CPU) $(WARN) $(OPT) $(DEFS) -I. -Idrivers -Idsp -Iclassify -Iapp
LDFLAGS  = $(CPU) -nostartfiles -Wl,--gc-sections -Wl,-Map=$(BUILD)/$(TARGET).map

# ---- Linker script (minimal H743) -------------------------------------------
# We provide a minimal linker script inline via a generated file.
LDSCRIPT = linker.ld

# ---- Sources ----------------------------------------------------------------
SRCS = main.c \
       drivers/i2s_mic.c drivers/adxl355.c drivers/sht45.c \
       drivers/sd.c drivers/esp_coex.c drivers/power.c \
       dsp/fft.c dsp/beamform.c dsp/mfcc.c dsp/features.c \
       classify/forest.c classify/forest_weights.c \
       app/events.c app/net.c \
       startup.c

OBJS = $(patsubst %.c,$(BUILD)/%.o,$(SRCS))

# ---- Rules ------------------------------------------------------------------
all: $(BUILD)/$(TARGET).elf $(BUILD)/$(TARGET).hex $(BUILD)/$(TARGET).bin

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/$(TARGET).elf: $(OBJS) $(LDSCRIPT)
	@mkdir -p $(BUILD)
	$(CC) $(LDFLAGS) -T$(LDSCRIPT) $(OBJS) -o $@ -lc -lm
	$(SIZE) $@

$(BUILD)/$(TARGET).hex: $(BUILD)/$(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

$(BUILD)/$(TARGET).bin: $(BUILD)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

flash: $(BUILD)/$(TARGET).elf
	openocd -f interface/stlink.cfg -f target/stm32h7x.cfg \
	    -c "program $< verify reset exit"

clean:
	rm -rf $(BUILD)

.PHONY: all flash clean