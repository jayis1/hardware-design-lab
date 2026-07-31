# Synthand Firmware Makefile
# Author: jayis1
# Copyright (C) 2026 jayis1. All rights reserved.
# License: GPL-3.0

# Toolchain
CROSS   = arm-none-eabi-
CC      = $(CROSS)gcc
AS      = $(CROSS)gcc
LD      = $(CROSS)gcc
OBJCOPY = $(CROSS)objcopy
SIZE    = $(CROSS)size
OPENOCD = openocd

# MCU flags — Cortex-M33 with FPU and DSP
MCU      = -mcpu=cortex-m33 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
CFLAGS   = $(MCU) -std=c11 -Wall -Wextra -Os -g3 -ffunction-sections -fdata-sections
CFLAGS  += -I. -Idrivers
CFLAGS  += -DNRF5340_XXAA -DNRF5340_APPLICATION
CFLAGS  += -fno-common -fno-exceptions

# Linker
LDFLAGS  = $(MCU) -T linker.ld -Wl,--gc-sections -Wl,-Map=synthand.map
LDFLAGS  += -specs=nano.specs -specs=nosys.specs
LDLIBS   = -lm

# Source files
SRCS = main.c
DRIVERS = drivers/imu.c drivers/emg.c drivers/signal.c drivers/gesture.c \
          drivers/tcn_model.c drivers/haptic.c drivers/ble_midi.c \
          drivers/osc.c drivers/power.c drivers/storage.c drivers/usb.c
ASM_SRCS = startup.s

# Object files
OBJS = $(SRCS:.c=.o) $(DRIVERS:.c=.o) $(ASM_SRCS:.s=.o)

# Targets
.PHONY: all clean flash debug

all: synthand.elf synthand.hex synthand.bin

synthand.elf: $(OBJS) linker.ld
	$(LD) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $@
	$(SIZE) $@

synthand.hex: synthand.elf
	$(OBJCOPY) -O ihex $< $@

synthand.bin: synthand.elf
	$(OBJCOPY) -O binary $< $@

# Compile rules
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.s
	$(AS) $(MCU) -x assembler-with-cpp -c $< -o $@

# Flash via OpenOCD (nRF5340 app core)
flash: synthand.hex
	$(OPENOCD) -f interface/cmsis-dap.cfg -f target/nrf52.cfg \
		-c "init; reset init; halt; nrf53_app_loadfile synthand.hex; \
		reset run; exit"

# Debug via OpenOCD + GDB
debug: synthand.elf
	$(OPENOCD) -f interface/cmsis-dap.cfg -f target/nrf52.cfg & \
	sleep 1 && $(CROSS)gdb synthand.elf -ex "target remote :3333"

# Dependencies
main.o: main.c board.h registers.h drivers/imu.h drivers/emg.h \
	drivers/signal.h drivers/gesture.h drivers/tcn_model.h drivers/haptic.h \
	drivers/ble_midi.h drivers/osc.h drivers/power.h drivers/storage.h \
	drivers/usb.h

drivers/imu.o: drivers/imu.c drivers/imu.h board.h registers.h
drivers/emg.o: drivers/emg.c drivers/emg.h board.h registers.h
drivers/signal.o: drivers/signal.c drivers/signal.h board.h
drivers/gesture.o: drivers/gesture.c drivers/gesture.h drivers/tcn_model.h board.h
drivers/tcn_model.o: drivers/tcn_model.c drivers/tcn_model.h board.h
drivers/haptic.o: drivers/haptic.c drivers/haptic.h board.h registers.h
drivers/ble_midi.o: drivers/ble_midi.c drivers/ble_midi.h board.h registers.h
drivers/osc.o: drivers/osc.c drivers/osc.h board.h
drivers/power.o: drivers/power.c drivers/power.h board.h registers.h
drivers/storage.o: drivers/storage.c drivers/storage.h board.h registers.h
drivers/usb.o: drivers/usb.c drivers/usb.h board.h registers.h

clean:
	rm -f $(OBJS) synthand.elf synthand.hex synthand.bin synthand.map