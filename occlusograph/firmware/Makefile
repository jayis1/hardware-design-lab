# Makefile — Occlusograph firmware build
#
# Author: jayis1
# Copyright © 2026 jayis1. All rights reserved.
# License: GPL-3.0
#
# This Makefile wraps the Zephyr build system for the nRF5340 target.
# On a real nRF Connect SDK install, you would run:
#
#   west build -b nrf5340dk_nrf5340_cpuapp
#   west flash
#
# This Makefile provides a convenience wrapper and a host-compile target
# for unit-testing the firmware logic (fusion, classifier, flash_log)
# without Zephyr, using stub implementations of the hal_* functions.

# ---- Target build (nRF5340 via Zephyr) ----
ZEPHYR_BOARD ?= nrf5340dk_nrf5340_cpuapp
WEST ?= west

.PHONY: target flash

target:
	$(WEST) build -b $(ZEPHYR_BOARD) firmware -- -DCONF_FILE=main.c
	@echo "Target build complete. Use 'make flash' to program."

flash:
	$(WEST) flash

# ---- Host build (unit test / lint) ----
# Compiles all .c files with stub hal_*.c implementations on the host.
# This validates syntax and logic without needing Zephyr or hardware.

CC      ?= gcc
CFLAGS  := -std=c11 -Wall -Wextra -Wno-unused-parameter -O2 -g \
           -I. -DHOST_BUILD
LDFLAGS := -lm

SRCS := main.c sensors.c fusion.c classify.c ble.c flash_log.c power.c
OBJS := $(SRCS:.c=.o)

# Host test harness (stubs for hal_* functions + a tiny unit-test main).
HOST_SRCS := host_stubs.c
HOST_OBJS := $(HOST_SRCS:.c=.o)

.PHONY: host clean lint

host: host_test

host_test: $(OBJS) $(HOST_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Host unit-test binary built: ./host_test"

lint:
	@if command -v cppcheck >/dev/null 2>&1; then \
		cppcheck --enable=all --suppress=missingIncludeSystem \
		  --suppress=unusedFunction -I. $(SRCS); \
	else echo "cppcheck not installed; skipping"; fi

clean:
	rm -f $(OBJS) $(HOST_OBJS) host_test
	@if [ -d build ]; then $(WEST) build -t pristine 2>/dev/null || rm -rf build; fi

# ---- Dependencies ----
main.o:        main.c board.h registers.h sensors.h fusion.h classify.h ble.h flash_log.h power.h
sensors.o:     sensors.c sensors.h board.h registers.h
fusion.o:      fusion.c fusion.h board.h sensors.h
classify.o:    classify.c classify.h board.h fusion.h
ble.o:         ble.c ble.h board.h classify.h registers.h
flash_log.o:   flash_log.c flash_log.h board.h registers.h classify.h
power.o:       power.c power.h board.h registers.h