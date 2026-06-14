# SkyPilot Firmware — Makefile
# Cross-compile for STM32H743 (Cortex-M7, arm-none-eabi)

TOOLCHAIN = arm-none-eabi-
CC = $(TOOLCHAIN)gcc
AS = $(TOOLCHAIN)as
LD = $(TOOLCHAIN)ld
OBJCOPY = $(TOOLCHAIN)objcopy
OBJDUMP = $(TOOLCHAIN)objdump
SIZE = $(TOOLCHAIN)size

# CPU flags for Cortex-M7 with FPU
CPU_FLAGS = -mcpu=cortex-m7 -mthumb -mfloat-abi=hard -mfpu=fpv5-d16
CFLAGS = $(CPU_FLAGS) -Wall -Wextra -Werror -O2 -g3 \
         -ffunction-sections -fdata-sections -fno-common \
         -ffreestanding -nostdlib \
         -I. -Idrivers \
         -DSTM32H743xx -DHSE_VALUE=24000000 -DSYSCLK_FREQ=480000000

ASFLAGS = $(CPU_FLAGS) -g3

# Linker script
LDSCRIPT = stm32h743vit6.ld

# Source files
SRCS = main.c \
       drivers/imu_icm42688.c \
       drivers/lte_lara_r6.c

# Object files
OBJS = $(SRCS:.c=.o)

# Output
TARGET = skypilot
BUILDDIR = build

all: $(BUILDDIR)/$(TARGET).elf $(BUILDDIR)/$(TARGET).bin $(BUILDDIR)/$(TARGET).hex

$(BUILDDIR):
	mkdir -p $(BUILDDIR)
	mkdir -p $(BUILDDIR)/drivers

$(BUILDDIR)/%.o: %.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/%.o: %.s | $(BUILDDIR)
	$(AS) $(ASFLAGS) $< -o $@

$(BUILDDIR)/$(TARGET).elf: $(OBJS) $(LDSCRIPT) | $(BUILDDIR)
	$(CC) $(CPU_FLAGS) -T$(LDSCRIPT) -Wl,--gc-sections \
	       -Wl,-Map=$(BUILDDIR)/$(TARGET).map \
	       -Wl,--print-memory-usage \
	       -o $@ $(OBJS) -lgcc

$(BUILDDIR)/$(TARGET).bin: $(BUILDDIR)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(BUILDDIR)/$(TARGET).hex: $(BUILDDIR)/$(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

size: $(BUILDDIR)/$(TARGET).elf
	$(SIZE) $<

map: $(BUILDDIR)/$(TARGET).map

# Flash via OpenOCD (ST-Link)
flash: $(BUILDDIR)/$(TARGET).elf
	openocd -f interface/stlink.cfg -f target/stm32h7x.cfg \
	        -c "program $< verify reset exit"

# Flash via DFU (USB-C)
dfu: $(BUILDDIR)/$(TARGET).bin
	dfu-util -a 0 --dfuse-address 0x08000000:leave -D $<

# Flash via UART bootloader
flash-uart: $(BUILDDIR)/$(TARGET).bin
	stm32flash -w $< -v -b 115200 /dev/ttyUSB0

clean:
	rm -rf $(BUILDDIR)

.PHONY: all clean flash dfu flash-uart size map