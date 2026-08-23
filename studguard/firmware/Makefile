# Makefile — StudGuard firmware simulation build system
#
# Author: jayis1
# Copyright (C) 2026 jayis1. All rights reserved.
# License: GPL-2.0

CC      = gcc
CFLAGS  = -std=c11 -O2 -Wall -Wextra -Werror -pedantic -I. -Idrivers
LDFLAGS = -lm
TARGET  = studguard_sim

SRCS = main.c \
       drivers/acoustic.c \
       drivers/moisture.c \
       drivers/mesh.c \
       drivers/display.c \
       drivers/ble.c \
       drivers/power.c \
       drivers/logger.c

OBJS = $(SRCS:.c=.o)

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
