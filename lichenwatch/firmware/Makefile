# Makefile — Lichenwatch firmware
# Author: jayis1
# License: MIT
#
# Build with arm-none-eabi-gcc. In CI (no toolchain), `make compile-check`
# runs a syntax/structure check with the host compiler to validate the
# code compiles cleanly as C (driver logic is host-buildable).

# ---- Toolchain (real firmware build) ----
CC      ?= arm-none-eabi-gcc
OBJCOPY ?= arm-none-eabi-objcopy
SIZE    ?= arm-none-eabi-size

# Host compiler for the compile-check target.
HOST_CC ?= gcc

CFLAGS_COMMON = -std=c11 -Wall -Wextra -Wno-unused-parameter \
                -Wno-unused-but-set-variable -Wno-unused-variable \
                -Os -g3 -ffunction-sections -fdata-sections \
                -I. -Idrivers

CFLAGS_ARM = $(CFLAGS_COMMON) -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 \
             -mfloat-abi=hard -DARM_CM4 -ffreestanding -nostdlib

# ---- Sources ----
SRC = main.c \
       drivers/pam_drv.c \
       drivers/spectral_drv.c \
       drivers/env_drv.c \
       drivers/wetness_drv.c \
       drivers/audio_drv.c \
       drivers/flash_drv.c \
       drivers/lora_drv.c \
       drivers/ble_svc.c \
       drivers/power.c \
       drivers/inference.c \
       drivers/model.c

OBJ = $(SRC:.c=.o)

# ---- Targets ----
.PHONY: all clean compile-check

all: lichenwatch.elf

lichenwatch.elf: $(OBJ)
	$(CC) $(CFLAGS_ARM) -T link.ld -Wl,--gc-sections -o $@ $(OBJ) -lc -lm
	$(SIZE) $@

%.o: %.c
	$(CC) $(CFLAGS_ARM) -c -o $@ $<

# ---- Host compile-check (CI / no toolchain) ----
# Validates that all C compiles cleanly. Freestanding headers only; the
# shim headers provide the FreeRTOS and register surface so the host GCC
# can type-check the entire tree.
compile-check:
	$(HOST_CC) $(CFLAGS_COMMON) -DHOST_BUILD -c -o /dev/null main.c
	$(HOST_CC) $(CFLAGS_COMMON) -DHOST_BUILD -c -o /dev/null drivers/pam_drv.c
	$(HOST_CC) $(CFLAGS_COMMON) -DHOST_BUILD -c -o /dev/null drivers/spectral_drv.c
	$(HOST_CC) $(CFLAGS_COMMON) -DHOST_BUILD -c -o /dev/null drivers/env_drv.c
	$(HOST_CC) $(CFLAGS_COMMON) -DHOST_BUILD -c -o /dev/null drivers/wetness_drv.c
	$(HOST_CC) $(CFLAGS_COMMON) -DHOST_BUILD -c -o /dev/null drivers/audio_drv.c
	$(HOST_CC) $(CFLAGS_COMMON) -DHOST_BUILD -c -o /dev/null drivers/flash_drv.c
	$(HOST_CC) $(CFLAGS_COMMON) -DHOST_BUILD -c -o /dev/null drivers/lora_drv.c
	$(HOST_CC) $(CFLAGS_COMMON) -DHOST_BUILD -c -o /dev/null drivers/ble_svc.c
	$(HOST_CC) $(CFLAGS_COMMON) -DHOST_BUILD -c -o /dev/null drivers/power.c
	$(HOST_CC) $(CFLAGS_COMMON) -DHOST_BUILD -c -o /dev/null drivers/inference.c
	$(HOST_CC) $(CFLAGS_COMMON) -DHOST_BUILD -c -o /dev/null drivers/model.c
	@echo "compile-check: all files compiled cleanly"

clean:
	rm -f $(OBJ) lichenwatch.elf lichenwatch.hex lichenwatch.bin