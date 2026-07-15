# =============================================================================
# Makefile — ThermoGlyph firmware build
#
# Target: nRF5340 (Cortex-M33, dual-core)
# Toolchain: GNU Arm Embedded (arm-none-eabi-)
# SDK: nRF Connect SDK (Zephyr-based) — referenced but not required for stubs
#
# Author: jayis1
# Copyright (c) 2026 jayis1
# License: GPL-3.0
# =============================================================================

# --- Toolchain ---
CC       = arm-none-eabi-gcc
AS       = arm-none-eabi-gcc
LD       = arm-none-eabi-ld
OBJCOPY  = arm-none-eabi-objcopy
SIZE     = arm-none-eabi-size

# --- Target CPU ---
ARCH     = -mcpu=cortex-m33 -mthumb -mfpu=auto -mfloat-abi=hard

# --- Optimization ---
OPT      = -Os

# --- Include paths ---
INCLUDES = -I. \
           -I$(NRF_CONNECT_SDK_DIR)/include \
           -I$(NRF_CONNECT_SDK_DIR)/modules/hal/nrf5340/include

# --- Compiler flags ---
CFLAGS   = $(ARCH) $(OPT) $(INCLUDES) \
           -std=c11 -Wall -Wextra -Wno-unused-parameter \
           -ffunction-sections -fdata-sections \
           -fno-builtin -fomit-frame-pointer \
           -DBOARD_THERMOGLYPH -DNRF5340_XXAA \
           -DNRF53_APPLICATION_UNIT \
           -DBOARD_AUTHOR=\"jayis1\"

# --- Linker flags ---
LDFLAGS  = $(ARCH) -nostdlib -nostartfiles \
           -Wl,--gc-sections -Wl,--build-id=none \
           -T linker.ld

# --- Source files ---
C_SOURCES = \
	main.c \
	glyph_engine.c \
	thermal_pid.c \
	imu_gesture.c \
	ble_service.c \
	lora_link.c \
	power_mgmt.c

ASM_SOURCES = \
	startup_nrf5340.s

# --- Object files ---
OBJECTS = $(C_SOURCES:.c=.o) $(ASM_SOURCES:.s=.o)

# --- Target ---
TARGET = thermoglyph.elf
HEX    = thermoglyph.hex
BIN    = thermoglyph.bin

# --- Build rules ---
.PHONY: all clean flash size

all: $(TARGET) $(HEX) $(BIN) size

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS) -lc -lm

$(HEX): $(TARGET)
	$(OBJCOPY) -O ihex $< $@

$(BIN): $(TARGET)
	$(OBJCOPY) -O binary $< $@

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.s
	$(AS) $(ARCH) -x assembler-with-cpp -c -o $@ $<

size: $(TARGET)
	$(SIZE) $(TARGET)

flash: $(HEX)
	openocd -f interface/cmsis-dap.cfg \
		-f target/nrf52.cfg \
		-c "program $(HEX) verify reset exit 0x10000"

clean:
	rm -f $(OBJECTS) $(TARGET) $(HEX) $(BIN)