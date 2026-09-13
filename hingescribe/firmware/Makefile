# HingeScribe deterministic firmware build
# Author: jayis1
# Copyright (c) 2026 jayis1
# SPDX-License-Identifier: MIT
TARGET := hingescribe
CC ?= cc
CFLAGS := -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror -DHS_HOST_SIM=1
LDLIBS := -lm
SOURCES := main.c sensors.c analytics.c protocol.c platform.c
OBJECTS := $(SOURCES:.c=.o)

.PHONY: all clean test run check target
all: $(TARGET)
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) $(LDLIBS) -o $@
%.o: %.c board.h registers.h hingescribe.h
	$(CC) $(CFLAGS) -c $< -o $@
run: $(TARGET)
	./$(TARGET)
test: $(TARGET)
	./$(TARGET) --self-test
check: clean all test
# Target build expects Nordic nRF5 SDK 17.1.0 startup/linker objects.
target:
	arm-none-eabi-gcc -std=c11 -Os -Wall -Wextra -Werror -mcpu=cortex-m4 -mthumb \
		-DNRF52840_XXAA -I$(NRF5_SDK)/modules/nrfx -I. -c $(SOURCES)
clean:
	rm -f $(OBJECTS) $(TARGET)
