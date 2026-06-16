# PhotonWeave Firmware Makefile
# Cross-compile for ARM Cortex-M7 (STM32H743VIT6)
# Target: STM32H743 system controller firmware

# Toolchain
CC      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
OBJDUMP = arm-none-eabi-objdump
SIZE    = arm-none-eabi-size
GDB     = arm-none-eabi-gdb

# MCU configuration
MCU      = cortex-m7
FPU      = fpv5-sp-d16
FLOAT    = hard
CPUFLAGS = -mcpu=$(MCU) -mthumb -mfpu=$(FPU) -mfloat-abi=$(FLOAT)

# Device
PART     = STM32H743VITx
DEFINES  = -D$(PART) -DUSE_HAL_DRIVER -DHSE_VALUE=25000000

# Directories
BUILD_DIR  = build
SRC_DIR    = .
DRV_DIR    = drivers
HAL_DIR    = STM32CubeH7/Drivers
CMSIS_DIR  = $(HAL_DIR)/CMSIS

# Source files
C_SOURCES  = \
    main.c \
    system_clock.c \
    gpio_init.c \
    si5345_config.c \
    fpga_program.c \
    board_eeprom.c \
    tmp117_sensor.c \
    thermal_manager.c \
    usb_command.c \
    $(DRV_DIR)/ddr4_mig_driver.c \
    $(DRV_DIR)/cgh_pipeline.c \
    $(DRV_DIR)/hdmi_output.c \
    $(DRV_DIR)/qsfp_ingest.c \
    $(DRV_DIR)/dma_engine.c \
    $(DRV_DIR)/error_handler.c \
    $(DRV_DIR)/watchdog.c

# HAL sources (minimal required set)
HAL_SOURCES = \
    $(HAL_DIR)/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal.c \
    $(HAL_DIR)/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_cortex.c \
    $(HAL_DIR)/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rcc.c \
    $(HAL_DIR)/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rcc_ex.c \
    $(HAL_DIR)/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_gpio.c \
    $(HAL_DIR)/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_i2c.c \
    $(HAL_DIR)/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_i2c_ex.c \
    $(HAL_DIR)/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_spi.c \
    $(HAL_DIR)/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_spi_ex.c \
    $(HAL_DIR)/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_uart.c \
    $(HAL_DIR)/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_uart_ex.c \
    $(HAL_DIR)/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_tim.c \
    $(HAL_DIR)/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_tim_ex.c \
    $(HAL_DIR)/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pwr.c \
    $(HAL_DIR)/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pwr_ex.c \
    $(HAL_DIR)/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_flash.c \
    $(HAL_DIR)/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_flash_ex.c \
    $(HAL_DIR)/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dma.c \
    $(HAL_DIR)/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_exti.c

# Startup and system files
STARTUP_S  = startup_stm32h743xx.s
SYSTEM_C   = system_stm32h7xx.c

# Include paths
INCLUDES   = \
    -I. \
    -Idrivers \
    -I$(CMSIS_DIR)/Include \
    -I$(CMSIS_DIR)/Device/ST/STM32H7xx/Include \
    -I$(HAL_DIR)/STM32H7xx_HAL_Driver/Inc

# Compiler flags
CFLAGS     = $(CPUFLAGS) $(DEFINES) $(INCLUDES)
CFLAGS    += -Os -Wall -Wextra -Werror -Wno-unused-parameter
CFLAGS    += -ffunction-sections -fdata-sections
CFLAGS    += -fno-common -fno-strict-aliasing
CFLAGS    += -std=gnu11 -g3 -gdwarf-2
CFLAGS    += -fstack-usage -fno-builtin

# Linker flags
LDSCRIPT   = STM32H743VITx_FLASH.ld
LDFLAGS    = $(CPUFLAGS) -T $(LDSCRIPT)
LDFLAGS   += -Wl,--gc-sections
LDFLAGS   += -Wl,-Map=$(BUILD_DIR)/photonweave_stm32.map
LDFLAGS   += -specs=nano.specs -specs=nosys.specs
LDFLAGS   += -Wl,--print-memory-usage
LDFLAGS   += -Wl,--cref

# Object lists
C_OBJS     = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
HAL_OBJS   = $(addprefix $(BUILD_DIR)/,$(notdir $(HAL_SOURCES:.c=.o)))
ASM_OBJS   = $(BUILD_DIR)/startup_stm32h743xx.o
SYS_OBJS   = $(BUILD_DIR)/system_stm32h7xx.o

ALL_OBJS   = $(C_OBJS) $(HAL_OBJS) $(ASM_OBJS) $(SYS_OBJS)

# Target names
ELF        = $(BUILD_DIR)/photonweave_stm32.elf
BIN        = $(BUILD_DIR)/photonweave_stm32.bin
HEX        = $(BUILD_DIR)/photonweave_stm32.hex
LST        = $(BUILD_DIR)/photonweave_stm32.lst
SIZE_FILE  = $(BUILD_DIR)/photonweave_stm32.size

# Default target
.PHONY: all clean flash debug size

all: $(BIN) $(HEX) $(LST) size

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Compile C sources from SRC_DIR
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile C sources from DRV_DIR
$(BUILD_DIR)/%.o: $(DRV_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile HAL sources
$(BUILD_DIR)/%.o: $(HAL_DIR)/STM32H7xx_HAL_Driver/Src/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Assemble startup file
$(BUILD_DIR)/startup_stm32h743xx.o: $(STARTUP_S) | $(BUILD_DIR)
	$(CC) $(CPUFLAGS) -c $< -o $@

# Compile system file
$(BUILD_DIR)/system_stm32h7xx.o: $(SYSTEM_C) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Link
$(ELF): $(ALL_OBJS)
	$(CC) $(LDFLAGS) $^ -o $@

# Binary
$(BIN): $(ELF)
	$(OBJCOPY) -O binary $< $@

# Intel HEX
$(HEX): $(ELF)
	$(OBJCOPY) -O ihex $< $@

# Disassembly listing
$(LST): $(ELF)
	$(OBJDUMP) -d -S $< > $@

# Size analysis
size: $(ELF)
	$(SIZE) --format=berkeley $< | tee $(SIZE_FILE)

# Clean
clean:
	rm -rf $(BUILD_DIR)

# Flash via ST-Link
flash: $(BIN)
	st-flash --reset write $< 0x08000000

# Flash via OpenOCD
flash-openocd: $(BIN)
	openocd -f interface/stlink.cfg -f target/stm32h7x.cfg \
		-c "program $< 0x08000000 verify reset exit"

# Debug via GDB
debug: $(ELF)
	$(GDB) -ex "target extended-remote :3333" \
	       -ex "monitor reset halt" \
	       -ex "load" \
	       -ex "monitor reset init" \
	       $<

# Static analysis
cppcheck:
	cppcheck --enable=all --inconclusive --std=c11 \
	         --suppress=missingIncludeSystem \
	         $(INCLUDES) $(C_SOURCES)

# Format code
format:
	clang-format -i $(C_SOURCES) $(DRV_DIR)/*.c $(DRV_DIR)/*.h *.h

# Generate compile_commands.json for LSP
bear: clean
	bear -- make all
