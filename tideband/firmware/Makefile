# TideBand Firmware Makefile
# Author: jayis1
# Copyright © 2026 jayis1. All rights reserved.
# License: GPL-2.0
#
# Build: make
# Clean: make clean
# Flash: make flash (requires OpenOCD + ST-Link)

# ---- Toolchain ----
CC      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
OBJDUMP = arm-none-eabi-objdump
SIZE    = arm-none-eabi-size

# ---- Target MCU: STM32H733VGT6 ----
MCU = -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard

# ---- Compiler flags ----
CFLAGS = $(MCU) -std=c11 -Wall -Wextra -Wno-unused-parameter \
         -O2 -g -ffunction-sections -fdata-sections \
         -I. -Idrivers \
         -DSTM32H733xx -DUSE_FULL_LL_DRIVER \
         -DARM_MATH_CM7 -D__FPU_PRESENT=1

# ---- Linker flags ----
LDFLAGS = $(MCU) -TSTM32H733VGTx_FLASH.ld \
         -Wl,--gc-sections -Wl,-Map=tideband.map \
         --specs=nano.specs --specs=nosys.specs \
         -lc -lm -lnosys

# ---- CMSIS-DSP library (optional, for optimized FFT) ----
# Uncomment if CMSIS-DSP is available in your toolchain
# CMSIS_DSP_LIB = -larm_cortexM7lfsp_math
# CFLAGS += -I$(CMSIS_DIR)/Include

# ---- Source files ----
SRCS = main.c \
       drivers/doppler.c \
       drivers/attitude.c \
       drivers/depth.c \
       drivers/storage.c \
       drivers/display.c \
       drivers/ble_link.c \
       drivers/haptic.c \
       drivers/power.c

OBJS = $(SRCS:.c=.o)

# ---- Startup and system files (provided by STM32CubeH7) ----
STARTUP = startup_stm32h733xx.s
SYSTEM  = system_stm32h7xx.c

# ---- Build targets ----
TARGET = tideband

all: $(TARGET).elf $(TARGET).hex $(TARGET).bin

$(TARGET).elf: $(STARTUP:.s=.o) $(SYSTEM:.c=.o) $(OBJS) STM32H733VGTx_FLASH.ld
	$(CC) $(LDFLAGS) -o $@ $^ $(CMSIS_DSP_LIB)
	$(SIZE) $@

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

# ---- Pattern rules ----
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.s
	$(CC) $(CFLAGS) -x assembler-with-cpp -c $< -o $@

# ---- Dependencies ----
main.o: main.c board.h registers.h drivers/doppler.h drivers/attitude.h \
        drivers/depth.h drivers/storage.h drivers/display.h \
        drivers/ble_link.h drivers/haptic.h drivers/power.h

drivers/doppler.o: drivers/doppler.c drivers/doppler.h board.h registers.h
drivers/attitude.o: drivers/attitude.c drivers/attitude.h board.h registers.h
drivers/depth.o: drivers/depth.c drivers/depth.h board.h registers.h
drivers/storage.o: drivers/storage.c drivers/storage.h board.h registers.h
drivers/display.o: drivers/display.c drivers/display.h board.h registers.h \
                   drivers/doppler.h drivers/depth.h drivers/attitude.h
drivers/ble_link.o: drivers/ble_link.c drivers/ble_link.h board.h registers.h \
                    drivers/doppler.h drivers/depth.h drivers/attitude.h
drivers/haptic.o: drivers/haptic.c drivers/haptic.h board.h registers.h
drivers/power.o: drivers/power.c drivers/power.h board.h registers.h

# ---- Flash target (OpenOCD) ----
flash: $(TARGET).elf
	openocd -f interface/stlink.cfg -f target/stm32h7x.cfg \
		-c "program $< verify reset exit"

# ---- Debug target (GDB) ----
debug: $(TARGET).elf
	arm-none-eabi-gdb -ex "target remote localhost:3333" -ex "load" $<

# ---- Clean ----
clean:
	rm -f $(OBJS) $(STARTUP:.s=.o) $(SYSTEM:.c=.o) \
	      $(TARGET).elf $(TARGET).hex $(TARGET).bin $(TARGET).map

.PHONY: all clean flash debug