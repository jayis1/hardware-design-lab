# HexaScope Firmware Makefile
# Cross-compilation for ARM Cortex-M4 (STM32G474)

# Toolchain
PREFIX  = arm-none-eabi-
CC      = $(PREFIX)gcc
AS      = $(PREFIX)as
LD      = $(PREFIX)ld
OBJCOPY = $(PREFIX)objcopy
OBJDUMP = $(PREFIX)objdump
SIZE    = $(PREFIX)size

# Target MCU
MCU      = cortex-m4
CPUFLAGS = -mcpu=$(MCU) -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16
CFLAGS  = $(CPUFLAGS) -Wall -Wextra -Wstrict-prototypes -O2 -g3 \
          -ffunction-sections -fdata-sections -fno-common \
          -I. -Idrivers
ASFLAGS = $(CPUFLAGS) -Wall -g3
LDFLAGS = -T link.ld -nostartfiles -Wl,--gc-sections -Wl,-Map=hexascope.map

# Source files
CSRCS   = main.c \
          board.c \
          startup.c
DRVR_SRCS = drivers/pmic_da9063.c \
            drivers/dac60508.c \
            drivers/spi_flash.c \
            drivers/fpga_config.c \
            drivers/usb_pd.c \
            drivers/esp32_wifi.c

OBJS    = $(CSRCS:.c=.o) $(DRVR_SRCS:.c=.o)

# Output
TARGET  = hexascope
HEX     = $(TARGET).hex
BIN     = $(TARGET).bin
ELF     = $(TARGET).elf

# Default target
all: $(BIN) $(HEX)

$(ELF): $(OBJS) link.ld
	$(CC) $(LDFLAGS) -o $@ $(OBJS) -lc -lgcc -lm

$(BIN): $(ELF)
	$(OBJCOPY) -O binary $< $@

$(HEX): $(ELF)
	$(OBJCOPY) -O ihex $< $@

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.S
	$(AS) $(ASFLAGS) -o $@ $<

clean:
	rm -f $(OBJS) $(ELF) $(BIN) $(HEX) $(TARGET).map

flash: $(BIN)
	openocd -f interface/stlink.cfg -f target/stm32g4x.cfg \
		-c "program $(BIN) verify reset exit 0x08000000"

.PHONY: all clean flash