# Makefile — SoundLoom firmware
# Author: jayis1
# Copyright (c) 2026 jayis1. All rights reserved.
#
# SoundLoom: Bioacoustic Soil Health Monitor
# Target: STM32H733VIH6 (Cortex-M7, 480 MHz)
# Toolchain: arm-none-eabi-gcc

# ---- Toolchain ----
CC       = arm-none-eabi-gcc
OBJCOPY  = arm-none-eabi-objcopy
OBJDUMP  = arm-none-eabi-objdump
SIZE     = arm-none-eabi-size

# ---- MCU flags ----
MCU      = -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard
MCU     += -mfp16-format=ieee

# ---- Include paths ----
INCLUDES = -I. \
           -I$(HAL_DIR)/Inc \
           -I$(CMSIS_DIR)/Include \
           -I$(CMSIS_DSP_DIR)/Include

# ---- C flags ----
CFLAGS   = $(MCU) $(INCLUDES) -std=c11 -Wall -Wextra -Wno-unused-parameter
CFLAGS  += -O2 -ffunction-sections -fdata-sections -fno-common
CFLAGS  += -DSTM32H733xx -DUSE_HAL_DRIVER -DARM_MATH_CM7
CFLAGS  += -g3 -gdwarf-2

# ---- Linker flags ----
LDFLAGS  = $(MCU) -T$(LDSCRIPT) -Wl,--gc-sections -Wl,-Map=$(BUILD)/soundloom.map
LDFLAGS += -lm -lc --specs=nano.specs --specs=nosys.specs

# ---- Directories ----
BUILD    = build
HAL_DIR  = Drivers/STM32H7xx_HAL_Driver
CMSIS_DIR = Drivers/CMSIS/Device/ST/STM32H7xx/Include
CMSIS_DSP_DIR = Drivers/CMSIS/DSP
LDSCRIPT = STM32H733VIHx_FLASH.ld

# ---- Source files ----
SOURCES = main.c acq.c dsp.c classifier.c beamformer.c env_sensors.c \
          lorawan.c svi.c \
          $(HAL_DIR)/Src/stm32h7xx_hal.c \
          $(HAL_DIR)/Src/stm32h7xx_hal_spi.c \
          $(HAL_DIR)/Src/stm32h7xx_hal_dma.c \
          $(HAL_DIR)/Src/stm32h7xx_hal_uart.c \
          $(HAL_DIR)/Src/stm32h7xx_hal_i2c.c \
          $(HAL_DIR)/Src/stm32h7xx_hal_adc.c \
          $(HAL_DIR)/Src/stm32h7xx_hal_gpio.c \
          $(HAL_DIR)/Src/stm32h7xx_hal_rcc.c \
          $(HAL_DIR)/Src/stm32h7xx_hal_rtc.c \
          $(HAL_DIR)/Src/stm32h7xx_hal_pwr.c \
          $(HAL_DIR)/Src/stm32h7xx_hal_flash.c \
          $(HAL_DIR)/Src/stm32h7xx_hal_sd.c \
          $(HAL_DIR)/Src/stm32h7xx_hal_tim.c \
          $(HAL_DIR)/Src/stm32h7xx_hal_cortex.c \
          system_stm32h7xx.c \
          stm32h7xx_it.c

OBJECTS = $(patsubst %.c,$(BUILD)/%.o,$(SOURCES))

# ---- Targets ----

.PHONY: all clean flash dump size

all: $(BUILD)/soundloom.elf $(BUILD)/soundloom.bin $(BUILD)/soundloom.hex

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/%.o: %.c | $(BUILD)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/soundloom.elf: $(OBJECTS) $(LDSCRIPT) | $(BUILD)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS)
	$(SIZE) $@

$(BUILD)/soundloom.bin: $(BUILD)/soundloom.elf
	$(OBJCOPY) -O binary $< $@

$(BUILD)/soundloom.hex: $(BUILD)/soundloom.elf
	$(OBJCOPY) -O ihex $< $@

flash: $(BUILD)/soundloom.bin
	openocd -f interface/stlink.cfg -f target/stm32h7x.cfg \
		-c "program $(BUILD)/soundloom.bin verify reset exit 0x08000000"

dump: $(BUILD)/soundloom.elf
	$(OBJDUMP) -d -S $< > $(BUILD)/soundloom.lst

size: $(BUILD)/soundloom.elf
	$(SIZE) $<

clean:
	rm -rf $(BUILD)

# ---- Dependencies ----
main.o: board.h registers.h acq.h dsp.h classifier.h beamformer.h \
         env_sensors.h lorawan.h svi.h
acq.o: board.h registers.h acq.h
dsp.o: board.h dsp.h
classifier.o: board.h classifier.h
beamformer.o: board.h beamformer.h
env_sensors.o: board.h registers.h env_sensors.h
lorawan.o: board.h registers.h lorawan.h
svi.o: board.h svi.h env_sensors.h classifier.h