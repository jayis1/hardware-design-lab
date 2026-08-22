# Canopy Sentinel firmware Makefile
# Author: jayis1
# Copyright (C) 2026 jayis1. All rights reserved.
# License: GPL-2.0

CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -Werror -std=c11
INCS = -I. -Idrivers

SRCS = main.c \
       drivers/power.c \
       drivers/display.c \
       drivers/ble.c \
       drivers/climate.c \
       drivers/thermal.c \
       drivers/leaf.c \
       drivers/spore.c \
       drivers/storage.c

OBJS = $(SRCS:.c=.o)
TARGET = canopy_sentinel_sim

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) -lm

%.o: %.c
	$(CC) $(CFLAGS) $(INCS) -c -o $@ $<

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
