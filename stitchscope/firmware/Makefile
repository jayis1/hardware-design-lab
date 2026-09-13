# StitchScope portable firmware/simulator build
# Author: jayis1
# Copyright (C) 2026 jayis1. All rights reserved.
CC ?= gcc
CFLAGS ?= -O2 -std=c11 -Wall -Wextra -Wpedantic -Werror
CPPFLAGS ?= -I.
LDLIBS ?= -lm
TARGET := stitchscope_sim
SOURCES := main.c sensors.c signal.c classifier.c protocol.c
OBJECTS := $(SOURCES:.c=.o)

.PHONY: all clean run check
all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS) $(LDLIBS)

%.o: %.c board.h registers.h stitchscope.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

run: $(TARGET)
	./$(TARGET)

check: $(TARGET)
	./$(TARGET) --self-test

clean:
	rm -f $(OBJECTS) $(TARGET)
