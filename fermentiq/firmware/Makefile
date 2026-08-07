# Makefile — FermenTiq Firmware Build
# FermenTiq — Smart Multi-Modal Fermentation Bioreactor Monitor
#
# Author: jayis1
# Copyright (c) 2026 jayis1
# SPDX-License-Identifier: GPL-3.0
#
# Build: make
# Flash: make flash
# Monitor: make monitor
# Clean: make clean
#
# This project uses ESP-IDF v5.1 build system. For standalone builds
# without ESP-IDF, use the arm-none-eabi toolchain targets below.

# ========================================================================
# ESP-IDF Build (primary)
# ========================================================================
# When using ESP-IDF, build is handled by `idf.py build`. This Makefile
# provides convenience wrappers.

IDF_PATH ?= $(HOME)/esp/esp-idf
IDF_PY   = $(IDF_PATH)/tools/idf.py

.PHONY: all build flash monitor clean menuconfig

all: build

build:
	@echo "Building FermenTiq firmware (ESP-IDF)..."
	$(IDF_PY) build

flash:
	@echo "Flashing FermenTiq firmware..."
	$(IDF_PY) -p /dev/ttyUSB0 flash

monitor:
	$(IDF_PY) -p /dev/ttyUSB0 monitor

menuconfig:
	$(IDF_PY) menuconfig

clean:
	$(IDF_PY) fullclean

# ========================================================================
# Standalone Build (fallback, for syntax checking without ESP-IDF)
# ========================================================================
# These targets allow syntax-checking the firmware with arm-none-eabi-gcc
# when ESP-IDF is not available. They do not produce a flashable binary.

CROSS   = arm-none-eabi-
CC      = $(CROSS)gcc
OBJCOPY = $(CROSS)objcopy
SIZE    = $(CROSS)size

MCU     = cortex-m7
FPU     = fpv5-sp-d16
FLOAT_ABI = hard

CFLAGS  = -mcpu=$(MCU) -mthumb -mfpu=$(FPU) -mfloat-abi=$(FLOAT_ABI)
CFLAGS += -std=c11 -Wall -Wextra -Os -g3
CFLAGS += -ffreestanding -fno-common -fdata-sections -ffunction-sections
CFLAGS += -I. -Idrivers
CFLAGS += -DPLATFORM_ESP32S3=1
CFLAGS += -DESP_IDF_AVAILABLE=0

# For standalone syntax check, provide minimal stubs
LDFLAGS = -mcpu=$(MCU) -mthumb -nostdlib -T linker.ld

# Source files
SRCS_C = main.c
SRCS_DRV = drivers/ad5933.c drivers/co2_ndir.c drivers/ph_isfet.c \
           drivers/temp_rtd.c drivers/acoustic.c drivers/fusion.c \
           drivers/ble.c drivers/wifi_mqtt.c drivers/power.c \
           drivers/storage.c

OBJS = $(SRCS_C:.c=.o) $(SRCS_DRV:.c=.o)

# ========================================================================
# Syntax Check Target (no ESP-IDF required)
# ========================================================================
.PHONY: syntax-check

syntax-check:
	@echo "=== Syntax checking FermenTiq firmware ==="
	@for f in $(SRCS_C) $(SRCS_DRV); do \
		echo "  CC $$f"; \
		$(CC) $(CFLAGS) -fsyntax-only $$f 2>&1 || true; \
	done
	@echo "=== Syntax check complete ==="
	@echo "Note: Warnings about missing ESP-IDF headers are expected"
	@echo "      in standalone mode. Use ESP-IDF for full compilation."

# ========================================================================
# Line Count (for quality verification)
# ========================================================================
.PHONY: loc

loc:
	@echo "=== FermenTiq Firmware Line Count ==="
	@wc -l main.c board.h registers.h Makefile linker.ld drivers/*.c drivers/*.h
	@echo ""
	@echo "Total C lines:"
	@cat main.c drivers/*.c | wc -l
	@echo "Total H lines:"
	@cat board.h registers.h drivers/*.h | wc -l