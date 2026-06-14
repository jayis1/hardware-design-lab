# NeuroLink Firmware Makefile
# Cross-compile for ARM Cortex-M7 (STM32H743)

# Toolchain
PREFIX  = arm-none-eabi-
CC      = $(PREFIX)gcc
AS      = $(PREFIX)gcc
LD      = $(PREFIX)ld
OBJCOPY = $(PREFIX)objcopy
OBJDUMP = $(PREFIX)objdump
SIZE    = $(PREFIX)size

# CPU flags
CPU_FLAGS  = -mcpu=cortex-m7 -mthumb -mfloat-abi=hard -mfpu=fpv5-d16
OPT_FLAGS  = -O2 -ffunction-sections -fdata-sections -fno-common
WARN_FLAGS = -Wall -Wextra -Wstrict-prototypes -Wshadow -Wundef
DEFS       = -DSTM32H743xx -DUSE_HAL_DRIVER -DHSE_VALUE=27000000

CFLAGS  = $(CPU_FLAGS) $(OPT_FLAGS) $(WARN_FLAGS) $(DEFS) \
          -I. -Idrivers

# Linker
LDSCRIPT = stm32h743zit6.ld
LDFLAGS = -T$(LDSCRIPT) -Wl,--gc-sections -Wl,--print-memory-usage \
          -Wl,-Map=$(TARGET).map --specs=nano.specs --specs=nosys.specs

# Target
TARGET = neurolink

# Source files
SRCS = main.c \
       drivers/ads1299.c \
       drivers/bq25895.c \
       drivers/lsm6dsox.c \
       drivers/ice40_spi.c

OBJS = $(SRCS:.c=.o)

# Build rules
.PHONY: all clean flash debug

all: $(TARGET).elf $(TARGET).bin $(TARGET).hex

$(TARGET).elf: $(OBJS)
	$(CC) $(CPU_FLAGS) $(LDFLAGS) -o $@ $^
	$(SIZE) $@

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.s
	$(AS) $(CPU_FLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET).elf $(TARGET).bin $(TARGET).hex $(TARGET).map

flash: $(TARGET).hex
	openocd -f interface/stlink.cfg -f target/stm32h7x.cfg \
		-c "program $(TARGET).hex verify reset exit"

debug: $(TARGET).elf
	arm-none-eabi-gdb $(TARGET).elf \
		-ex "target remote :3333" \
		-ex "monitor reset halt" \
		-ex "load" \
		-ex "monitor reset halt"

# Dependencies
-include $(OBJS:.o=.d)