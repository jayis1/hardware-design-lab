# Makefile — Chronos Pulser Firmware
# Cross-compile for RISC-V RV32IM (VexRiscv soft CPU on ECP5 FPGA)
# Target: Lattice ECP5-45F with VexRiscv soft CPU

# Toolchain
CROSS_COMPILE ?= riscv64-unknown-elf-
CC            := $(CROSS_COMPILE)gcc
AS            := $(CROSS_COMPILE)as
LD            := $(CROSS_COMPILE)ld
OBJCOPY       := $(CROSS_COMPILE)objcopy
OBJDUMP       := $(CROSS_COMPILE)objdump
SIZE          := $(CROSS_COMPILE)size

# Architecture and ABI
ARCH_FLAGS    := -march=rv32im -mabi=ilp32 -mcmodel=medany

# Optimization: size-focused for embedded
OPT_FLAGS     := -Os -ffunction-sections -fdata-sections \
                 -fno-builtin -fno-stack-protector \
                 -ffreestanding -nostdlib

# Warning flags
WARN_FLAGS    := -Wall -Wextra -Werror -Wno-unused-parameter \
                 -Wno-missing-field-initializers

# Include paths
INCLUDES      := -I. -Idrivers

# Defines
DEFINES       := -DF_CPU=100000000UL \
                 -DCHRONOS_PULSER_REV=1 \
                 -DCHRONOS_PULSER_VARIANT=0 \
                 -DDEVICE_ID=0x4350

CFLAGS        := $(ARCH_FLAGS) $(OPT_FLAGS) $(WARN_FLAGS) $(INCLUDES) $(DEFINES)
ASFLAGS       := $(ARCH_FLAGS) $(INCLUDES) $(DEFINES)

# Linker flags
LDFLAGS       := $(ARCH_FLAGS) -Wl,--gc-sections \
                 -Wl,-Map=build/firmware.map \
                 -T linker/firmware.ld \
                 -nostdlib -nostartfiles

# Source files
C_SRCS := \
    main.c \
    drivers/clock_config.c \
    drivers/gpio_init.c \
    drivers/adc_driver.c \
    drivers/tdc_driver.c \
    drivers/pulse_driver.c \
    drivers/vga_driver.c \
    drivers/i2c_driver.c \
    drivers/spi_flash.c \
    drivers/usb_protocol.c \
    drivers/temp_monitor.c \
    drivers/error_handler.c \
    drivers/calibration.c \
    drivers/led_controller.c

ASM_SRCS := \
    startup.S \
    vectors.S

# Object files
C_OBJS   := $(C_SRCS:.c=.o)
ASM_OBJS := $(ASM_SRCS:.S=.o)
OBJS     := $(addprefix build/,$(C_OBJS) $(ASM_OBJS))

# Output files
ELF       := build/firmware.elf
BIN       := build/firmware.bin
HEX       := build/firmware.hex
LST       := build/firmware.lst
MAP       := build/firmware.map

# Build directories
BUILD_DIRS := build build/drivers build/linker

.PHONY: all clean flash size dirs

all: dirs $(BIN) $(HEX) $(LST)

dirs:
	@mkdir -p $(BUILD_DIRS)

$(ELF): $(OBJS) linker/firmware.ld
	$(CC) $(LDFLAGS) -o $@ $(OBJS) -lgcc

$(BIN): $(ELF)
	$(OBJCOPY) -O binary $< $@

$(HEX): $(ELF)
	$(OBJCOPY) -O ihex $< $@

$(LST): $(ELF)
	$(OBJDUMP) -d -S $< > $@

build/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

build/%.o: %.S
	@mkdir -p $(dir $@)
	$(CC) $(ASFLAGS) -c -o $@ $<

# Linker script
linker/firmware.ld:
	@mkdir -p linker
	@echo "/* Chronos Pulser firmware linker script */" > $@
	@echo "MEMORY {" >> $@
	@echo "  BRAM (rwx)  : ORIGIN = 0x10000000, LENGTH = 8K" >> $@
	@echo "  DDR3 (rwx)  : ORIGIN = 0x00000000, LENGTH = 256M" >> $@
	@echo "}" >> $@
	@echo "SECTIONS {" >> $@
	@echo "  .text : { *(.text*) } > DDR3" >> $@
	@echo "  .rodata : { *(.rodata*) } > DDR3" >> $@
	@echo "  .data : { *(.data*) } > DDR3" >> $@
	@echo "  .bss : { *(.bss*) } > DDR3" >> $@
	@echo "  .stack (NOLOAD) : { . = ALIGN(16); _stack_top = .; . += 16K; _stack_bottom = .; } > DDR3" >> $@
	@echo "  .heap (NOLOAD) : { . = ALIGN(16); _heap_start = .; . += 64K; _heap_end = .; } > DDR3" >> $@
	@echo "}" >> $@

size: $(ELF)
	$(SIZE) $@

# Flash firmware to SPI flash via ecpprog
flash: $(BIN)
	@echo "Flashing firmware to SPI flash at offset 0x140000..."
	ecpprog -o 0x140000 -w $(BIN)
	@echo "Flash complete."

clean:
	rm -rf build/ linker/

# Help
help:
	@echo "Chronos Pulser Firmware Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all     — Build firmware binary, hex, and listing"
	@echo "  size    — Print firmware size"
	@echo "  flash   — Flash firmware to SPI flash via ecpprog"
	@echo "  clean   — Remove build artifacts"
	@echo ""
	@echo "Variables:"
	@echo "  CROSS_COMPILE — Toolchain prefix (default: riscv64-unknown-elf-)"
	@echo ""
	@echo "Output files:"
	@echo "  build/firmware.bin — Raw binary (for flashing)"
	@echo "  build/firmware.hex — Intel HEX format"
	@echo "  build/firmware.lst — Disassembly listing"
	@echo "  build/firmware.map — Linker map"
