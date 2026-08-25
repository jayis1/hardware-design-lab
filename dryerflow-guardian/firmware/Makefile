# Makefile — DryerFlow Guardian firmware
# Author: jayis1
# Copyright (c) 2026 jayis1
# SPDX-License-Identifier: GPL-3.0

CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -pedantic -O2
LDFLAGS ?= -lm

TARGET = dryerflow-guardian-fw
SRCS = main.c \
       drivers/pressure.c \
       drivers/airflow.c \
       drivers/thermal.c \
       drivers/acoustic.c \
       drivers/gas.c \
       drivers/power.c \
       drivers/analytics.c \
       drivers/logger.c \
       drivers/ble.c
OBJS = $(SRCS:.c=.o)

.PHONY: all clean run loc

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) $(OBJS)

loc:
	wc -l main.c board.h registers.h drivers/*.c drivers/*.h
