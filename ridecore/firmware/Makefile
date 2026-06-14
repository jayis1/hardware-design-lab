# ============================================================================
# RideCore Firmware Makefile
# Cross-compile for ARM Cortex-M4 (STM32G474)
# ============================================================================

# Toolchain
PREFIX  = arm-none-eabi-
CC      = $(PREFIX)gcc
OBJCOPY = $(PREFIX)objcopy
OBJDUMP = $(PREFIX)objdump
SIZE    = $(PREFIX)size

# Target
TARGET  = ridecore

# Build directory
BUILDDIR = build

# MCU specifics
MCU      = -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16
DEFS     = -DSTM32G474xx -DUSE_HAL_DRIVER -DHSE_VALUE=8000000
INCLUDES = -I. -Idrivers

# Compiler flags
CFLAGS   = $(MCU) $(DEFS) $(INCLUDES) -Wall -Wextra -Werror -Wno-unused-parameter \
           -O2 -ffunction-sections -fdata-sections -fno-common -fmessage-length=0 \
           -std=gnu11 -ggdb3

LDFLAGS  = $(MCU) -specs=nano.specs -specs=nosys.specs \
           -Tstm32g474ceux_flash.ld \
           -Wl,--gc-sections -Wl,--print-memory-usage \
           -Wl,-Map=$(BUILDDIR)/$(TARGET).map,--cref

# Source files
C_SRCS   = main.c \
           clock_config.c \
           gpio_init.c \
           foc.c \
           drivers/pmic.c \
           drivers/canfd.c \
           drivers/spi_flash.c \
           drivers/ble_uart.c

OBJS     = $(patsubst %.c,$(BUILDDIR)/%.o,$(C_SRCS))

# Default target
all: $(BUILDDIR)/$(TARGET).elf $(BUILDDIR)/$(TARGET).bin $(BUILDDIR)/$(TARGET).hex
	$(SIZE) $(BUILDDIR)/$(TARGET).elf

$(BUILDDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/$(TARGET).elf: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ -lm

$(BUILDDIR)/$(TARGET).bin: $(BUILDDIR)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(BUILDDIR)/$(TARGET).hex: $(BUILDDIR)/$(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

clean:
	rm -rf $(BUILDDIR)

flash: $(BUILDDIR)/$(TARGET).elf
	openocd -f interface/stlink.cfg -f target/stm32g4x.cfg -c "program $< verify reset exit"

dfu: $(BUILDDIR)/$(TARGET).bin
	dfu-util -a 0 -s 0x08000000:leave -D $<

.PHONY: all clean flash dfu