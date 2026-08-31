# Threshold Veil firmware Makefile
# Author: jayis1
# Copyright (C) 2026 jayis1. All rights reserved.

CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -Werror -std=c11
INCS = -I. -Idrivers

SRCS = main.c \
       drivers/environment.c \
       drivers/acoustic.c \
       drivers/seal.c \
       drivers/power.c \
       drivers/inference.c \
       drivers/comms.c \
       drivers/logger.c

OBJS = $(SRCS:.c=.o)
TARGET = threshold_veil_sim

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
