# CyberGuard Firmware Makefile
# Cross-compiles for ARM Cortex-M33 (STM32L562)

# Toolchain
PREFIX  = arm-none-eabi-
CC      = $(PREFIX)gcc
AS      = $(PREFIX)gcc
LD      = $(PREFIX)ld
OBJCOPY = $(PREFIX)objcopy
OBJDUMP = $(PREFIX)objdump
SIZE    = $(PREFIX)size

# Target
TARGET = cyberguard

# CPU flags (Cortex-M33 with FPU)
CPU_FLAGS = -mcpu=cortex-m33 -mthumb -mfpu=fpv5-sp-d16 -mfloat-abi=hard

# Compiler flags
CFLAGS  = $(CPU_FLAGS) -Wall -Wextra -Werror -std=gnu11 -O2 -g3
CFLAGS += -ffunction-sections -fdata-sections -fno-common -fno-builtin-printf
CFLAGS += -I. -Idrivers
CFLAGS += -DSTM32L562xx -DUSE_HAL_DRIVER -DHSE_VALUE=0 -DHSI_VALUE=16000000

# Linker flags
LDFLAGS = $(CPU_FLAGS) -T cyberguard.ld -specs=nano.specs -specs=nosys.specs
LDFLAGS += -Wl,--gc-sections -Wl,--print-memory-usage
LDFLAGS += -Wl,-Map=$(TARGET).map,--cref

# Source files
SRCS = main.c
SRCS += drivers/i2c_se.c
SRCS += drivers/spi_nfc.c
SRCS += drivers/uart_fp.c
SRCS += drivers/uart_ble.c
SRCS += drivers/spi_flash.c
SRCS += drivers/pmic.c
SRCS += drivers/usb_ctap2.c

# Object files
OBJS = $(SRCS:.c=.o)
OBJS += startup.o

# Default target
all: $(TARGET).bin $(TARGET).hex $(TARGET).elf

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.s
	$(AS) $(CPU_FLAGS) -c -o $@ $<

$(TARGET).elf: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

size: $(TARGET).elf
	$(SIZE) $(TARGET).elf

disasm: $(TARGET).elf
	$(OBJDUMP) -d -S $(TARGET).elf > $(TARGET).lst

flash: $(TARGET).bin
	openocd -f interface/cmsis-dap.cfg -f target/stm32l5x.cfg \
		-c "program $(TARGET).bin verify reset exit 0x08000000"

clean:
	rm -f $(OBJS) $(TARGET).elf $(TARGET).bin $(TARGET).hex $(TARGET).map $(TARGET).lst

.PHONY: all clean flash size disasm