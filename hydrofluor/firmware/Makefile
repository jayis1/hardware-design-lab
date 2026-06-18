#
# Makefile — HydroFluor firmware build
# Author: jayis1
# License: MIT
#
# Builds the HydroFluor firmware for STM32L4R9VIT6 using the GNU Arm
# bare-metal toolchain (arm-none-eabi-gcc). The full STM32CubeL4 HAL is
# NOT used; this firmware uses a minimal register-level driver approach.
# The fatfs library is expected to be vendored under middleware/fatfs/.
#
# Usage:
#   make           # build hydrofluor.elf
#   make flash     # flash via OpenOCD + ST-Link
#   make clean
#

TARGET    := hydrofluor
ARCH      := arm-none-eabi
CC        := $(ARCH)-gcc
OBJCOPY   := $(ARCH)-objcopy
SIZE      := $(ARCH)-size
OPENOCD   := openocd

CPU       := -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
CFLAGS    := $(CPU) -std=c11 -Wall -Wextra -Os -g3 -ffunction-sections -fdata-sections
CFLAGS    += -I. -Idrivers -I../middleware/fatfs
CFLAGS    += -DSTM32L4R9xx -DUSE_FULL_LL_DRIVER

LDFLAGS   := $(CPU) -Tstm32l4r9.ld -Wl,--gc-sections -Wl,-Map=$(TARGET).map -nostartfiles
LDLIBS    := -lm

# Vendor startup + system files (provide SystemInit + vector table)
STARTUP   := startup_stm32l4r9xx.s
SYSTEM    := system_stm32l4r9.c

SRCS      := main.c $(SYSTEM) \
            drivers/led_excitation.c \
            drivers/photodiode.c \
            drivers/flowcell.c \
            drivers/fluorometry.c \
            drivers/storage.c \
            drivers/ble_telemetry.c \
            drivers/lora_link.c \
            drivers/depth.c \
            drivers/temp.c \
            ../middleware/fatfs/ff.c \
            ../middleware/fatfs/diskio.c

OBJS      := $(STARTUP:.s=.o) $(SRCS:.c=.o)

.PHONY: all clean flash

all: $(TARGET).elf $(TARGET).hex $(TARGET).bin

$(STARTUP:.s=.o): $(STARTUP)
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET).elf: $(OBJS) stm32l4r9.ld
	$(CC) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $@
	$(SIZE) $@

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

flash: $(TARGET).elf
	$(OPENOCD) -f interface/stlink.cfg -f target/stm32l4x.cfg \
	  -c "program $(TARGET).elf verify reset exit"

clean:
	rm -f $(OBJS) $(TARGET).elf $(TARGET).hex $(TARGET).bin $(TARGET).map