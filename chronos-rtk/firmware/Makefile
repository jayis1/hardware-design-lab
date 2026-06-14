# Chronos-RTK Firmware Makefile
# Cross-compile for STM32G474RET6 (ARM Cortex-M4F)

# Toolchain
PREFIX  = arm-none-eabi-
CC      = $(PREFIX)gcc
AS      = $(PREFIX)as
LD      = $(PREFIX)ld
OBJCOPY = $(PREFIX)objcopy
OBJDUMP = $(PREFIX)objdump
SIZE    = $(PREFIX)size

# Project
TARGET  = chronos-rtk
BUILDDIR = build

# MCU configuration
MCU      = cortex-m4
FPU      = fpv4-sp-d16
DEVICES  = STM32G474xx

# Compiler flags
CFLAGS  = -mcpu=$(MCU) -mthumb -mfloat-abi=hard -mfpu=$(FPU)
CFLAGS += -Wall -Wextra -Werror -std=gnu11 -O2
CFLAGS += -ffunction-sections -fdata-sections -fno-common
CFLAGS += -D$(DEVICES) -DUSE_FULL_ASSERT
CFLAGS += -I.

# Linker flags
LDFLAGS  = -mcpu=$(MCU) -mthumb -mfloat-abi=hard -mfpu=$(FPU)
LDFLAGS += -Wl,--gc-sections -Wl,--defsym=_stack=0x20020000
LDFLAGS += -Tstm32g474ret6.ld
LDFLAGS += -Wl,-Map=$(BUILDDIR)/$(TARGET).map,--cref

# Source files
SRCS_C  = main.c \
          drivers/sx1262.c \
          drivers/w25q128.c \
          drivers/ssd1306.c

SRCS_S  = startup_stm32g474xx.s

# Object files
OBJS    = $(patsubst %.c,$(BUILDDIR)/%.o,$(SRCS_C)) \
          $(patsubst %.s,$(BUILDDIR)/%.o,$(SRCS_S))

# Targets
.PHONY: all clean flash debug

all: $(BUILDDIR)/$(TARGET).elf $(BUILDDIR)/$(TARGET).bin $(BUILDDIR)/$(TARGET).hex
	$(SIZE) $(BUILDDIR)/$(TARGET).elf

$(BUILDDIR)/%.o: %.c | $(BUILDDIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILDDIR)/%.o: %.s | $(BUILDDIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILDDIR)/$(TARGET).elf: $(OBJS) stm32g474ret6.ld | $(BUILDDIR)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) -lgcc -lc -lnosys

$(BUILDDIR)/$(TARGET).bin: $(BUILDDIR)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(BUILDDIR)/$(TARGET).hex: $(BUILDDIR)/$(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

$(BUILDDIR):
	mkdir -p $(BUILDDIR)
	mkdir -p $(BUILDDIR)/drivers

clean:
	rm -rf $(BUILDDIR)

flash: $(BUILDDIR)/$(TARGET).elf
	openocd -f interface/stlink.cfg -f target/stm32g4x.cfg \
		-c "program $< verify reset exit"

debug: $(BUILDDIR)/$(TARGET).elf
	openocd -f interface/stlink.cfg -f target/stm32g4x.cfg &
	$(PREFIX)gdb $< -ex "target remote :3333"

dfu: $(BUILDDIR)/$(TARGET).bin
	dfu-util -d 0483:df11 -a 0 -s 0x08000000:leave -D $<