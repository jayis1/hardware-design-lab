# ============================================================================
#  Soilchord firmware — Makefile
#  Author: jayis1
#  SPDX-License-Identifier: MIT
# ----------------------------------------------------------------------------
#  Builds soilchord.elf / .hex / .bin for the STM32L432KC reference board
#  using arm-none-eabi-gcc. Links against CMSIS-DSP (replace the vendored
#  naive DFT in dsp.c with the real library for production).
# ============================================================================
TARGET     := soilchord
CROSS      ?= arm-none-eabi-
CC         := $(CROSS)gcc
OBJCOPY    := $(CROSS)objcopy
SIZE       := $(CROSS)size

MCU        := -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
OPT        := -Os -g3 -ffunction-sections -fdata-sections
WARN       := -Wall -Wextra -Wno-unused-parameter -Wno-unused-but-set-variable
DEFINES    := -DSTM32L432xx -DUSE_FULL_LL_DRIVER
INCS       := -I. -ICMSIS/Include -Istm32l4xx_hal/Inc

CFLAGS     := $(MCU) $(OPT) $(WARN) $(DEFINES) $(INCS) -std=c11
LDFLAGS    := $(MCU) -TSTM32L432KCUx_FLASH.ld -Wl,--gc-sections -Wl,-Map=$(TARGET).map
LDLIBS     := -lc -lm -lnosys

SRCS       := main.c dsp.c actuator.c piezo.c lora.c temp.c power.c flash_log.c
OBJS       := $(SRCS:.c=.o)

.PHONY: all flash size clean

all: $(TARGET).elf $(TARGET).hex $(TARGET).bin size

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET).elf: $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $@

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

size: $(TARGET).elf
	$(SIZE) $(TARGET).elf

flash: $(TARGET).hex
	openocd -f interface/stlink.cfg -f target/stm32l4x.cfg \
		-c "program $(TARGET).hex verify reset exit"

clean:
	rm -f $(OBJS) $(TARGET).elf $(TARGET).hex $(TARGET).bin $(TARGET).map