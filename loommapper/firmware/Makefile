# LoomMapper deterministic host firmware build
# Author: jayis1
# Copyright (c) 2026 jayis1
# SPDX-License-Identifier: MIT
TARGET := loommapper
CC ?= cc
CFLAGS := -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror -DLM_HOST_SIM=1
SOURCES := main.c drivers.c topology.c protocol.c diagnostics.c
OBJECTS := $(SOURCES:.c=.o)
.PHONY: all clean test run check target
all: $(TARGET)
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@
%.o: %.c board.h registers.h loommapper.h
	$(CC) $(CFLAGS) -c $< -o $@
test: $(TARGET)
	./$(TARGET) --self-test
run: $(TARGET)
	./$(TARGET)
check: clean all test run
# Target build requires Nordic nRF Connect SDK 2.7 and its generated headers.
target:
	arm-none-eabi-gcc -std=c11 -Os -Wall -Wextra -Werror -mcpu=cortex-m33 -mthumb -DNRF5340_XXAA_APPLICATION -I$(NCS)/zephyr/include -I. -c $(SOURCES)
clean:
	rm -f $(OBJECTS) $(TARGET)
