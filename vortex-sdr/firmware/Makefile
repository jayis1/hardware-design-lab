# Vortex-SDR Firmware Makefile
# Cross-compile for STM32H743ZIT6 (ARM Cortex-M7 @ 480 MHz)

# Toolchain
PREFIX  = arm-none-eabi-
CC      = $(PREFIX)gcc
AS      = $(PREFIX)as
LD      = $(PREFIX)ld
OBJCOPY = $(PREFIX)objcopy
OBJDUMP = $(PREFIX)objdump
SIZE    = $(PREFIX)size

# Project
TARGET  = vortex-sdr
BUILDDIR = build

# MCU configuration
MCU      = cortex-m7
FPU      = fpv5-sp-d16
DEVICES  = STM32H743xx

# Compiler flags
CFLAGS  = -mcpu=$(MCU) -mthumb -mfloat-abi=hard -mfpu=$(FPU)
CFLAGS += -Wall -Wextra -Werror -std=gnu11 -O2
CFLAGS += -ffunction-sections -fdata-sections -fno-common
CFLAGS += -D$(DEVICES) -DUSE_FULL_ASSERT -DHSE_VALUE=8000000
CFLAGS += -I.

# Linker flags
LDFLAGS  = -mcpu=$(MCU) -mthumb -mfloat-abi=hard -mfpu=$(FPU)
LDFLAGS += -Wl,--gc-sections -Wl,--defsym=_stack=0x20020000
LDFLAGS += -Tstm32h743zit6.ld
LDFLAGS += -Wl,-Map=$(BUILDDIR)/$(TARGET).map,--cref

# Source files
SRCS_C  = main.c \
          drivers/adf4351.c \
          drivers/ad9645.c \
          drivers/ili9341.c

SRCS_S  = startup_stm32h743xx.s

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

$(BUILDDIR)/$(TARGET).elf: $(OBJS) stm32h743zit6.ld | $(BUILDDIR)
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
	openocd -f interface/stlink.cfg -f target/stm32h7x.cfg \
		-c "program $< verify reset exit"

debug: $(BUILDDIR)/$(TARGET).elf
	openocd -f interface/stlink.cfg -f target/stm32h7x.cfg &
	gdb-multiarch -ex "target remote :3333" -ex "load" $(BUILDDIR)/$(TARGET).elf

.PHONY: size
size: $(BUILDDIR)/$(TARGET).elf
	$(SIZE) $<