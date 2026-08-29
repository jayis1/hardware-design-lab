# DrainVeil firmware Makefile
# Author: jayis1
# Copyright (C) 2026 jayis1. All rights reserved.

CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -pedantic -I. -Idrivers
LDFLAGS ?= -lm
TARGET = drainveil_demo
SOURCES = main.c \
	drivers/flow.c \
	drivers/pressure.c \
	drivers/chemistry.c \
	drivers/thermal.c \
	drivers/power.c \
	drivers/inference.c \
	drivers/ble.c \
	drivers/logger.c
OBJECTS = $(SOURCES:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: all clean
