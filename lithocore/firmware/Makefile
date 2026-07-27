# LithoCore Firmware Makefile
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

# MCU flags
MCU      = -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
CFLAGS   = $(MCU) -std=c11 -Wall -Wextra -Os -g3 -ffunction-sections -fdata-sections
CFLAGS  += -I. -Idrivers
CFLAGS  += -DSTM32G474xx -DUSE_FULL_LL_DRIVER
CFLAGS  += -fno-common -fno-exceptions

# Linker
LDFLAGS  = $(MCU) -T linker.ld -Wl,--gc-sections -Wl,-Map=lithocore.map
LDFLAGS  += -specs=nano.specs -specs=nosys.specs
LDLIBS   = -lm

# Source files
SRCS = main.c
DRIVERS = drivers/ads1256.c drivers/dds.c drivers/lockin.c drivers/eis_sweep.c \
          drivers/randles.c drivers/cnls.c drivers/soh.c drivers/dcir.c \
          drivers/coulomb.c drivers/safety.c drivers/power.c drivers/ble.c \
          drivers/usb.c drivers/storage.c
ASM_SRCS = startup.s

# Object files
OBJS = $(SRCS:.c=.o) $(DRIVERS:.c=.o) $(ASM_SRCS:.s=.o)

# Targets
.PHONY: all clean flash debug

all: lithocore.elf lithocore.bin lithocore.hex

lithocore.elf: $(OBJS) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)
	$(SIZE) $@

lithocore.bin: lithocore.elf
	$(OBJCOPY) -O binary $< $@

lithocore.hex: lithocore.elf
	$(OBJCOPY) -O ihex $< $@

# Compile rules
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.s
	$(AS) $(MCU) -c -o $@ $<

# Flash via OpenOCD
flash: lithocore.elf
	$(OPENOCD) -f interface/stlink.cfg -f target/stm32g4x.cfg \
		-c "program lithocore.elf verify reset exit"

# Debug via OpenOCD + GDB
debug: lithocore.elf
	$(OPENOCD) -f interface/stlink.cfg -f target/stm32g4x.cfg &
	$(CROSS)gdb lithocore.elf -ex "target remote :3333"

# Clean
clean:
	rm -f $(OBJS) lithocore.elf lithocore.bin lithocore.hex lithocore.map

# Dependencies
main.o: board.h registers.h drivers/ads1256.h drivers/dds.h drivers/lockin.h \
        drivers/eis_sweep.h drivers/randles.h drivers/cnls.h drivers/soh.h \
        drivers/dcir.h drivers/coulomb.h drivers/safety.h drivers/power.h \
        drivers/ble.h drivers/usb.h drivers/storage.h
drivers/ads1256.o: drivers/ads1256.h board.h registers.h
drivers/dds.o: drivers/dds.h board.h registers.h
drivers/lockin.o: drivers/lockin.h drivers/ads1256.h drivers/dds.h board.h registers.h
drivers/eis_sweep.o: drivers/eis_sweep.h drivers/lockin.h drivers/dds.h \
                     drivers/ads1256.h drivers/safety.h board.h
drivers/randles.o: drivers/randles.h drivers/lockin.h
drivers/cnls.o: drivers/cnls.h drivers/randles.h drivers/eis_sweep.h
drivers/soh.o: drivers/soh.h drivers/randles.h drivers/eis_sweep.h board.h
drivers/dcir.o: drivers/dcir.h drivers/safety.h board.h registers.h
drivers/coulomb.o: drivers/coulomb.h drivers/safety.h drivers/dcir.h board.h registers.h
drivers/safety.o: drivers/safety.h board.h registers.h
drivers/power.o: drivers/power.h board.h registers.h
drivers/ble.o: drivers/ble.h drivers/soh.h board.h registers.h
drivers/usb.o: drivers/usb.h drivers/soh.h board.h registers.h
drivers/storage.o: drivers/storage.h drivers/soh.h drivers/ble.h board.h registers.h