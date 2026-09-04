# sash-sentinel/firmware/Makefile
# Author: jayis1
# Copyright (C) 2026 jayis1. All rights reserved.

CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -pedantic -O2
LDFLAGS ?= -lm
TARGET := sash_sentinel_sim
SRCS := main.c \
	drivers/env.c \
	drivers/thermal.c \
	drivers/latch.c \
	drivers/airflow.c \
	drivers/power.c \
	drivers/logger.c \
	drivers/comms.c \
	drivers/inference.c
OBJS := $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

%.o: %.c board.h registers.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
