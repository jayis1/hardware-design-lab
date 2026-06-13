# EAG-7000 M4F Firmware — Makefile
# Cross-compilation for ARM Cortex-M4F (arm-none-eabi)
#
# Targets:
#   make          — Build firmware ELF
#   make clean    — Remove build artifacts
#   make flash    — Package as remoteproc binary (.bin)
#   make size     — Show firmware memory usage

# Toolchain
CROSS_COMPILE ?= arm-none-eabi-
CC = $(CROSS_COMPILE)gcc
AS = $(CROSS_COMPILE)as
LD = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy
SIZE = $(CROSS_COMPILE)size

# Target
TARGET = eag7000-m4f

# Build directory
BUILDDIR = build

# Sources
SRCS_C = main.c \
         drivers/spi.c \
         drivers/uart.c \
         drivers/i2c.c \
         drivers/gpio.c \
         drivers/mu.c \
         drivers/mcp2518fd.c

SRCS_S = startup.s

# Objects
OBJS = $(patsubst %.c,$(BUILDDIR)/%.o,$(SRCS_C)) \
       $(patsubst %.s,$(BUILDDIR)/%.o,$(SRCS_S))

# Includes
INCDIRS = . drivers

# MCU flags (Cortex-M4F with hardware FPU)
MCU_FLAGS = -mcpu=cortex-m4 \
            -mthumb \
            -mfpu=fpv4-sp-d16 \
            -mfloat-abi=hard

# Compiler flags
CFLAGS = $(MCU_FLAGS) \
         -Wall -Wextra -Werror \
         -O2 -g3 \
         -ffreestanding -nostdlib \
         -fno-builtin -fno-common \
         -ffunction-sections -fdata-sections \
         $(addprefix -I,$(INCDIRS)) \
         -DEAG7000_M4F=1

# Linker flags
LDFLAGS = $(MCU_FLAGS) \
          -nostartfiles \
          -Wl,--gc-sections \
          -Wl,--print-memory-usage \
          -T eag7000-m4f.ld \
          -Wl,-Map=$(BUILDDIR)/$(TARGET).map

# Output files
ELF = $(BUILDDIR)/$(TARGET).elf
BIN = $(BUILDDIR)/$(TARGET).bin
HEX = $(BUILDDIR)/$(TARGET).hex

# Default target
.PHONY: all
all: $(ELF) $(BIN)

# Create build directories
$(BUILDDIR):
	mkdir -p $(BUILDDIR) $(BUILDDIR)/drivers

# Compile C sources
$(BUILDDIR)/%.o: %.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Assemble startup file
$(BUILDDIR)/%.o: %.s | $(BUILDDIR)
	$(AS) $(MCU_FLAGS) $< -o $@

# Link
$(ELF): $(OBJS) eag7000-m4f.ld | $(BUILDDIR)
	$(CC) $(LDFLAGS) $(OBJS) -o $@ -lgcc

# Generate binary for remoteproc
$(BIN): $(ELF)
	$(OBJCOPY) -O binary $< $@

# Generate hex file
$(HEX): $(ELF)
	$(OBJCOPY) -O ihex $< $@

# Show size
.PHONY: size
size: $(ELF)
	$(SIZE) -A $<

# Clean
.PHONY: clean
clean:
	rm -rf $(BUILDDIR)

# Flash (copy .bin to Linux remotepproc path)
.PHONY: flash
flash: $(BIN)
	@echo "Copy $(BIN) to /lib/firmware/ for remoteproc loading"
	@echo "Then: echo $(TARGET).elf > /sys/class/remoteproc/remoteproc0/firmware"
	@echo "      echo start > /sys/class/remoteproc/remoteproc0/state"