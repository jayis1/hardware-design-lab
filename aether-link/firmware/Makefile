# Makefile for Aether-Link MicroBlaze Firmware
# Target: Xilinx MicroBlaze v11.0 (little-endian, 32-bit)
# Toolchain: Xilinx Vitis / mb-gcc
# Builds bare-metal firmware for FPGA management processor

# Toolchain prefix (adjust for your Vitis installation)
CROSS_COMPILE ?= mb-

CC      = $(CROSS_COMPILE)gcc
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump
SIZE    = $(CROSS_COMPILE)size

# MicroBlaze v11.0 flags
ARCH_FLAGS  = -mcpu=v11.0 -mno-xl-soft-mul -mlittle-endian -mhard-float
OPT_FLAGS   = -O2 -fno-strict-aliasing -fno-common
WARN_FLAGS  = -Wall -Wextra -Werror -Wshadow -Wpointer-arith \
              -Wcast-align -Wstrict-prototypes -Wmissing-prototypes
DBG_FLAGS   = -g3 -gdwarf-4

CFLAGS  = $(ARCH_FLAGS) $(OPT_FLAGS) $(WARN_FLAGS) $(DBG_FLAGS) \
          -ffunction-sections -fdata-sections \
          -I. -Idrivers -Iinclude

LDFLAGS = $(ARCH_FLAGS) -Wl,--gc-sections -Wl,-T,linker_script.ld \
          -nostartfiles -nostdlib -ffreestanding

# Source files
SRC = main.c \
      drivers/toe_driver.c \
      drivers/nvmeof_pdu.c \
      drivers/gpio_init.c \
      drivers/clock_config.c \
      drivers/i2c_driver.c \
      drivers/spi_flash.c \
      drivers/fan_control.c \
      drivers/power_monitor.c \
      drivers/error_handler.c

OBJ = $(SRC:.c=.o)
DEP = $(SRC:.c=.d)

TARGET      = aether_fw
ELF         = $(TARGET).elf
BIN         = $(TARGET).bin
HEX         = $(TARGET).hex
LST         = $(TARGET).lst
MAP         = $(TARGET).map
COE         = $(TARGET).coe
SIZE_REPORT = $(TARGET).size

# Default target
.PHONY: all
all: $(ELF) $(BIN) $(HEX) $(LST) $(SIZE_REPORT)

# Link
$(ELF): $(OBJ) linker_script.ld
	@echo "  LD      $@"
	$(CC) $(LDFLAGS) $(OBJ) -o $@ -Wl,-Map=$(MAP)

# Compile
%.o: %.c
	@echo "  CC      $<"
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

# Binary (for BRAM initialization)
$(BIN): $(ELF)
	@echo "  OBJCOPY $@"
	$(OBJCOPY) -O binary $< $@

# Intel HEX
$(HEX): $(ELF)
	@echo "  OBJCOPY $@"
	$(OBJCOPY) -O ihex $< $@

# Disassembly listing
$(LST): $(ELF)
	@echo "  OBJDUMP $@"
	$(OBJDUMP) -d -S $< > $@

# Size report
$(SIZE_REPORT): $(ELF)
	@echo "  SIZE    $@"
	$(SIZE) --format=berkeley $< | tee $@

# COE file for Vivado BRAM initialization
$(COE): $(BIN)
	@echo "  COE     $@"
	python3 bin_to_coe.py $< $@

# Clean
.PHONY: clean
clean:
	rm -f $(OBJ) $(DEP) $(ELF) $(BIN) $(HEX) $(LST) $(MAP) $(COE) $(SIZE_REPORT)

# Distclean
.PHONY: distclean
distclean: clean
	rm -f *~ drivers/*~ include/*~

# Include dependency files
-include $(DEP)

# Help
.PHONY: help
help:
	@echo "Aether-Link MicroBlaze Firmware Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all          - Build ELF, BIN, HEX, LST, and size report"
	@echo "  $(ELF)       - Build ELF executable"
	@echo "  $(BIN)       - Build raw binary for BRAM"
	@echo "  $(HEX)       - Build Intel HEX"
	@echo "  $(LST)       - Build disassembly listing"
	@echo "  $(COE)       - Build Vivado COE file for BRAM init"
	@echo "  clean        - Remove build artifacts"
	@echo "  distclean    - Remove build artifacts and editor backups"
	@echo ""
	@echo "Variables:"
	@echo "  CROSS_COMPILE - Toolchain prefix (default: mb-)"
	@echo ""
	@echo "Example:"
	@echo "  make CROSS_COMPILE=mb- all"
