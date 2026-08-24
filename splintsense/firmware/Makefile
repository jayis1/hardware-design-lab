# SplintSense firmware Makefile
# Author: jayis1
# Copyright (C) 2026 jayis1. All rights reserved.

CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -Werror -std=c11
INCS = -I. -Idrivers

SRCS = main.c \
       drivers/power.c \
       drivers/pressure.c \
       drivers/moisture.c \
       drivers/sensor_hub.c \
       drivers/ble.c \
       drivers/logger.c \
       drivers/haptics.c

OBJS = $(SRCS:.c=.o)
TARGET = splintsense_sim

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
