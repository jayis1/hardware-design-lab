# WaveForge Firmware Makefile
# Cross-compilation for ARM Cortex-M7 (STM32H743)

# Toolchain
PREFIX  = arm-none-eabi-
CC      = $(PREFIX)gcc
AS      = $(PREFIX)gcc
LD      = $(PREFIX)ld
OBJCOPY = $(PREFIX)objcopy
SIZE    = $(PREFIX)size
GDB     = $(PREFIX)gdb

# Target
TARGET  = waveforge

# Directories
BUILDDIR = build
SRCDIR   = .
DRVDIR   = drivers

# MCU flags
MCU      = -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard
MCU     += -mlittle-endian

# CPU options
CPU_DEFS = -DSTM32H743xx -DUSE_FULL_ASSERT -DHSE_VALUE=8000000

# Include paths
INCLUDES = -I. -I$(DRVDIR)

# Compiler flags
CFLAGS  = $(MCU) $(CPU_DEFS) $(INCLUDES)
CFLAGS += -Wall -Wextra -Wstrict-aliasing=3 -Wno-unused-parameter
CFLAGS += -fdata-sections -ffunction-sections
CFLAGS += -fno-common -fno-builtin -fno-rtti -fno-exceptions
CFLAGS += -O2 -g3
CFLAGS += -std=gnu11

# Linker flags
LDFLAGS  = $(MCU) -specs=nano.specs -specs=nosys.specs
LDFLAGS += -T stm32h743vit6.ld
LDFLAGS += -Wl,--gc-sections -Wl,--print-memory-usage
LDFLAGS += -Wl,-Map=$(BUILDDIR)/$(TARGET).map

# Source files
C_SRCS   = $(SRCDIR)/main.c
C_SRCS  += $(DRVDIR)/i2c.c
C_SRCS  += $(DRVDIR)/spi_flash.c
C_SRCS  += $(DRVDIR)/wm8778.c
C_SRCS  += $(DRVDIR)/tpa6130.c

ASM_SRCS = $(SRCDIR)/startup.s

# Object files
C_OBJS   = $(patsubst %.c,$(BUILDDIR)/%.o,$(C_SRCS))
ASM_OBJS = $(patsubst %.s,$(BUILDDIR)/%.o,$(ASM_SRCS))
OBJS     = $(C_OBJS) $(ASM_OBJS)

# Targets
.PHONY: all clean flash debug

all: $(BUILDDIR)/$(TARGET).bin $(BUILDDIR)/$(TARGET).hex $(BUILDDIR)/$(TARGET).elf

$(BUILDDIR):
	mkdir -p $(BUILDDIR)
	mkdir -p $(BUILDDIR)/$(DRVDIR)

$(BUILDDIR)/%.o: %.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILDDIR)/%.o: %.s | $(BUILDDIR)
	$(AS) $(CFLAGS) -c -o $@ $<

$(BUILDDIR)/$(TARGET).elf: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^
	$(SIZE) $@

$(BUILDDIR)/$(TARGET).bin: $(BUILDDIR)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(BUILDDIR)/$(TARGET).hex: $(BUILDDIR)/$(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

clean:
	rm -rf $(BUILDDIR)

flash: $(BUILDDIR)/$(TARGET).elf
	openocd -f interface/stlink.cfg -f target/stm32h7x.cfg \
		-c "program $< verify reset exit"

debug: $(BUILDDIR)/$(TARGET).elf
	openocd -f interface/stlink.cfg -f target/stm32h7x.cfg &
	$(GDB) $< -ex "target remote :3333" -ex "load" -ex "break main" -ex "continue"

size: $(BUILDDIR)/$(TARGET).elf
	$(SIZE) $<

# Dependencies
-include $(C_OBJS:.o=.d)