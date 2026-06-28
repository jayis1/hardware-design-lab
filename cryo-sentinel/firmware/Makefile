# Makefile — Cryo-Sentinel firmware build.
# Author: jayis1
# License: MIT
#
# This is a reference Makefile for the nRF Connect SDK (NCS) / Zephyr port.
# In a real NCS workspace, west handles the build; this file is provided so
# the source tree is self-documenting and so a desktop smoke-compile with
# mock platform shims works without Zephyr installed.

# ---- Toolchain -------------------------------------------------------
CC      ?= arm-none-eabi-gcc
OBJCOPY ?= arm-none-eabi-objcopy
SIZE    ?= arm-none-eabi-size

# ---- Targets ---------------------------------------------------------
# Target build uses Zephyr in real life; for desktop smoke-compile use:
#   make DESKTOP=1
# which defines CS_DESKTOP and links against mock_*.c stubs.

DESKTOP ?= 0
ifeq ($(DESKTOP),1)
CC       = gcc
CFLAGS  += -DCS_DESKTOP -D_POSIX_C_SOURCE=200809L -lm
LDFLAGS += -lm
endif

# ---- Sources ---------------------------------------------------------
SRCS = main.c cs_sensor.c cs_alarm.c cs_log.c cs_radio.c cs_power.c \
       cs_calibration.c

OBJS = $(SRCS:.c=.o)

# ---- Flags -----------------------------------------------------------
CFLAGS += -std=c99 -Wall -Wextra -Wno-unused-parameter -Os -g3 \
          -ffunction-sections -fdata-sections \
          -I.

# Cortex-M33 tuning (ignored on desktop).
CFLAGS += -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16
LDFLAGS += -Wl,--gc-sections -Wl,-Map=cryo-sentinel.map

# ---- Rules -----------------------------------------------------------
all: cryo-sentinel.elf

cryo-sentinel.elf: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)
	$(SIZE) $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) cryo-sentinel.elf cryo-sentinel.map

.PHONY: all clean