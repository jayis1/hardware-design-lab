# FlangePrint deterministic firmware build
# Author: jayis1
# Copyright (c) 2026 jayis1
# SPDX-License-Identifier: MIT
TARGET := flangeprint
CC ?= cc
CFLAGS := -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror -DFP_HOST_SIM=1
LDLIBS := -lm
SOURCES := main.c drivers.c
OBJECTS := $(SOURCES:.c=.o)
.PHONY: all clean test run check target
all: $(TARGET)
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) $(LDLIBS) -o $@
%.o: %.c board.h registers.h
	$(CC) $(CFLAGS) -c $< -o $@
test: $(TARGET)
	./$(TARGET) --self-test
run: $(TARGET)
	./$(TARGET) --demo
check: clean all test run
# Target integration expects Nordic nRF5 SDK 17.1.0 startup/linker files.
target:
	arm-none-eabi-gcc -std=c11 -Os -Wall -Wextra -Werror -mcpu=cortex-m4 -mthumb \
		-DNRF52840_XXAA -I$(NRF5_SDK)/modules/nrfx -I. -c $(SOURCES)
clean:
	rm -f $(OBJECTS) $(TARGET)
