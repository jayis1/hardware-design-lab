# Makefile — Nebula Matrix STM32H7 Firmware
#
# Cross-compile for ARM Cortex-M7 (STM32H743VIT6)
# Toolchain: arm-none-eabi-gcc (GCC ARM Embedded)
# HAL: STM32H7xx HAL Driver Library
# RTOS: FreeRTOS
# Network: LWIP
# USB: TinyUSB
# Filesystem: FatFS

# =========================================================================
# Toolchain
# =========================================================================

TOOLCHAIN_PREFIX ?= arm-none-eabi-
CC      = $(TOOLCHAIN_PREFIX)gcc
CXX     = $(TOOLCHAIN_PREFIX)g++
AS      = $(TOOLCHAIN_PREFIX)gcc -x assembler-with-cpp
LD      = $(TOOLCHAIN_PREFIX)ld
OBJCOPY = $(TOOLCHAIN_PREFIX)objcopy
OBJDUMP = $(TOOLCHAIN_PREFIX)objdump
SIZE    = $(TOOLCHAIN_PREFIX)size
GDB     = $(TOOLCHAIN_PREFIX)gdb

# =========================================================================
# Project Configuration
# =========================================================================

TARGET       = nebula_matrix
BUILD_DIR    = build
DEVICE       = STM32H743VIT6
CPU          = cortex-m7
FPU          = fpv5-d16
FLOAT_ABI    = hard

# =========================================================================
# Source Files
# =========================================================================

# Core sources
C_SOURCES = \
    main.c \
    system_stm32h7xx.c \
    startup_stm32h743xx.s

# Driver sources
C_SOURCES += \
    drivers/hub75e_spi_driver.c \
    drivers/fpga_uart_driver.c \
    drivers/tmp117_driver.c \
    drivers/si5351_driver.c \
    drivers/ina219_driver.c \
    drivers/pmic_driver.c \
    drivers/w25q128_driver.c

# HAL sources (STM32H7xx_HAL_Driver)
HAL_DIR = STM32H7xx_HAL_Driver
C_SOURCES += \
    $(HAL_DIR)/Src/stm32h7xx_hal.c \
    $(HAL_DIR)/Src/stm32h7xx_hal_cortex.c \
    $(HAL_DIR)/Src/stm32h7xx_hal_rcc.c \
    $(HAL_DIR)/Src/stm32h7xx_hal_rcc_ex.c \
    $(HAL_DIR)/Src/stm32h7xx_hal_gpio.c \
    $(HAL_DIR)/Src/stm32h7xx_hal_dma.c \
    $(HAL_DIR)/Src/stm32h7xx_hal_dma_ex.c \
    $(HAL_DIR)/Src/stm32h7xx_hal_spi.c \
    $(HAL_DIR)/Src/stm32h7xx_hal_spi_ex.c \
    $(HAL_DIR)/Src/stm32h7xx_hal_uart.c \
    $(HAL_DIR)/Src/stm32h7xx_hal_uart_ex.c \
    $(HAL_DIR)/Src/stm32h7xx_hal_i2c.c \
    $(HAL_DIR)/Src/stm32h7xx_hal_i2c_ex.c \
    $(HAL_DIR)/Src/stm32h7xx_hal_tim.c \
    $(HAL_DIR)/Src/stm32h7xx_hal_tim_ex.c \
    $(HAL_DIR)/Src/stm32h7xx_hal_adc.c \
    $(HAL_DIR)/Src/stm32h7xx_hal_adc_ex.c \
    $(HAL_DIR)/Src/stm32h7xx_hal_eth.c \
    $(HAL_DIR)/Src/stm32h7xx_hal_sd.c \
    $(HAL_DIR)/Src/stm32h7xx_hal_sd_ex.c \
    $(HAL_DIR)/Src/stm32h7xx_hal_pwr.c \
    $(HAL_DIR)/Src/stm32h7xx_hal_pwr_ex.c \
    $(HAL_DIR)/Src/stm32h7xx_hal_flash.c \
    $(HAL_DIR)/Src/stm32h7xx_hal_flash_ex.c \
    $(HAL_DIR)/Src/stm32h7xx_hal_hsem.c \
    $(HAL_DIR)/Src/stm32h7xx_hal_mdma.c \
    $(HAL_DIR)/Src/stm32h7xx_ll_sdmmc.c

# FreeRTOS sources
FREERTOS_DIR = FreeRTOS
C_SOURCES += \
    $(FREERTOS_DIR)/croutine.c \
    $(FREERTOS_DIR)/event_groups.c \
    $(FREERTOS_DIR)/list.c \
    $(FREERTOS_DIR)/queue.c \
    $(FREERTOS_DIR)/stream_buffer.c \
    $(FREERTOS_DIR)/tasks.c \
    $(FREERTOS_DIR)/timers.c \
    $(FREERTOS_DIR)/portable/GCC/ARM_CM4F/port.c \
    $(FREERTOS_DIR)/portable/MemMang/heap_4.c

# LWIP sources
LWIP_DIR = lwip
C_SOURCES += \
    $(LWIP_DIR)/src/core/init.c \
    $(LWIP_DIR)/src/core/def.c \
    $(LWIP_DIR)/src/core/dns.c \
    $(LWIP_DIR)/src/core/inet_chksum.c \
    $(LWIP_DIR)/src/core/ip.c \
    $(LWIP_DIR)/src/core/mem.c \
    $(LWIP_DIR)/src/core/memp.c \
    $(LWIP_DIR)/src/core/netif.c \
    $(LWIP_DIR)/src/core/pbuf.c \
    $(LWIP_DIR)/src/core/raw.c \
    $(LWIP_DIR)/src/core/stats.c \
    $(LWIP_DIR)/src/core/sys.c \
    $(LWIP_DIR)/src/core/tcp.c \
    $(LWIP_DIR)/src/core/tcp_in.c \
    $(LWIP_DIR)/src/core/tcp_out.c \
    $(LWIP_DIR)/src/core/timeouts.c \
    $(LWIP_DIR)/src/core/udp.c \
    $(LWIP_DIR)/src/core/ipv4/autoip.c \
    $(LWIP_DIR)/src/core/ipv4/dhcp.c \
    $(LWIP_DIR)/src/core/ipv4/etharp.c \
    $(LWIP_DIR)/src/core/ipv4/icmp.c \
    $(LWIP_DIR)/src/core/ipv4/igmp.c \
    $(LWIP_DIR)/src/core/ipv4/ip4.c \
    $(LWIP_DIR)/src/core/ipv4/ip4_addr.c \
    $(LWIP_DIR)/src/core/ipv4/ip4_frag.c \
    $(LWIP_DIR)/src/netif/ethernet.c \
    $(LWIP_DIR)/src/api/api_lib.c \
    $(LWIP_DIR)/src/api/api_msg.c \
    $(LWIP_DIR)/src/api/err.c \
    $(LWIP_DIR)/src/api/if_api.c \
    $(LWIP_DIR)/src/api/netbuf.c \
    $(LWIP_DIR)/src/api/netdb.c \
    $(LWIP_DIR)/src/api/netifapi.c \
    $(LWIP_DIR)/src/api/sockets.c \
    $(LWIP_DIR)/src/api/tcpip.c

# FatFS sources
FATFS_DIR = FatFS
C_SOURCES += \
    $(FATFS_DIR)/ff.c \
    $(FATFS_DIR)/ffunicode.c \
    $(FATFS_DIR)/ffsystem.c

# TinyUSB sources
TINYUSB_DIR = tinyusb
C_SOURCES += \
    $(TINYUSB_DIR)/src/tusb.c \
    $(TINYUSB_DIR)/src/common/tusb_fifo.c \
    $(TINYUSB_DIR)/src/device/usbd.c \
    $(TINYUSB_DIR)/src/device/usbd_control.c \
    $(TINYUSB_DIR)/src/class/cdc/cdc_rndis.c \
    $(TINYUSB_DIR)/src/portable/st/stm32_fsdev/dcd_stm32_fsdev.c

# Mongoose HTTP server
C_SOURCES += mongoose/mongoose.c

# Art-Net / sACN receiver
C_SOURCES += artnet_receiver.c

# =========================================================================
# Include Paths
# =========================================================================

INCLUDES = \
    -I. \
    -Idrivers \
    -I$(HAL_DIR)/Inc \
    -I$(FREERTOS_DIR)/include \
    -I$(FREERTOS_DIR)/portable/GCC/ARM_CM4F \
    -I$(LWIP_DIR)/src/include \
    -I$(LWIP_DIR)/src/include/ipv4 \
    -I$(LWIP_DIR)/src/include/netif \
    -I$(FATFS_DIR) \
    -I$(TINYUSB_DIR)/src \
    -I$(TINYUSB_DIR)/src/portable/st/stm32_fsdev \
    -Imongoose \
    -ICMSIS/Core/Include \
    -ICMSIS/Device/ST/STM32H7xx/Include

# =========================================================================
# Compiler Flags
# =========================================================================

CPU_FLAGS = -mcpu=$(CPU) -mthumb -mfpu=$(FPU) -mfloat-abi=$(FLOAT_ABI)

CFLAGS = $(CPU_FLAGS) $(INCLUDES) \
    -D$(DEVICE) \
    -DUSE_HAL_DRIVER \
    -DUSE_FULL_LL_DRIVER \
    -DHSE_VALUE=$(HSE_FREQUENCY_HZ) \
    -DHSE_BYPASS \
    -Wall -Wextra -Werror \
    -fdata-sections -ffunction-sections \
    -fno-strict-aliasing \
    -Os -g -gdwarf-2 \
    -std=gnu11

ASFLAGS = $(CPU_FLAGS) -g -gdwarf-2

LDFLAGS = $(CPU_FLAGS) \
    -T STM32H743VITx_FLASH.ld \
    -Wl,--gc-sections \
    -Wl,-Map=$(BUILD_DIR)/$(TARGET).map \
    -Wl,--cref \
    -Wl,--print-memory-usage \
    -specs=nano.specs \
    -specs=nosys.specs \
    -u _printf_float \
    -lm -lc -lnosys

# =========================================================================
# Object Files
# =========================================================================

C_OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
ASM_OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))

vpath %.c $(sort $(dir $(C_SOURCES)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

OBJECTS = $(C_OBJECTS) $(ASM_OBJECTS)

# =========================================================================
# Build Rules
# =========================================================================

.PHONY: all clean flash debug size

all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).bin $(BUILD_DIR)/$(TARGET).hex

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR)
	@echo "  CC    $<"
	@$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	@echo "  AS    $<"
	@$(AS) -c $(ASFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) Makefile
	@echo "  LD    $@"
	@$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	@echo ""
	@$(SIZE) $@

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf
	@echo "  BIN   $@"
	@$(OBJCOPY) -O binary $< $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf
	@echo "  HEX   $@"
	@$(OBJCOPY) -O ihex $< $@

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# =========================================================================
# Clean
# =========================================================================

clean:
	@echo "  CLEAN"
	@rm -rf $(BUILD_DIR)

# =========================================================================
# Flash (via OpenOCD + ST-Link)
# =========================================================================

OPENOCD ?= openocd
OPENOCD_CFG ?= interface/stlink.cfg
OPENOCD_TARGET ?= target/stm32h7x.cfg

flash: $(BUILD_DIR)/$(TARGET).bin
	@echo "  FLASH $<"
	$(OPENOCD) -f $(OPENOCD_CFG) -f $(OPENOCD_TARGET) \
	    -c "init" \
	    -c "reset halt" \
	    -c "flash write_image erase $< 0x08000000" \
	    -c "reset run" \
	    -c "shutdown"

# =========================================================================
# Debug (via OpenOCD + GDB)
# =========================================================================

debug: $(BUILD_DIR)/$(TARGET).elf
	@echo "Starting OpenOCD..."
	$(OPENOCD) -f $(OPENOCD_CFG) -f $(OPENOCD_TARGET) &
	@sleep 2
	@echo "Starting GDB..."
	$(GDB) -ex "target extended-remote :3333" \
	       -ex "monitor reset halt" \
	       -ex "load" \
	       -ex "monitor reset init" \
	       $<

# =========================================================================
# Size Analysis
# =========================================================================

size: $(BUILD_DIR)/$(TARGET).elf
	$(SIZE) --format=berkeley $<
	@echo ""
	@echo "Section sizes:"
	$(OBJDUMP) -h $< | grep -E '\.text|\.data|\.bss|\.rodata'

# =========================================================================
# Dependencies
# =========================================================================

DEPS = $(C_OBJECTS:.o=.d)
-include $(DEPS)

$(BUILD_DIR)/%.d: %.c | $(BUILD_DIR)
	@$(CC) $(CFLAGS) -MM -MT $(@:.d=.o) $< > $@
