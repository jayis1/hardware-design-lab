# SealBeat firmware Makefile
# Author: jayis1

CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -pedantic -O2
TARGET = sealbeat-sim
SRCS = main.c \
       drivers/acoustic.c \
       drivers/door.c \
       drivers/seal.c \
       drivers/thermal.c \
       drivers/power.c \
       drivers/inference.c \
       drivers/ble.c \
       drivers/logger.c

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -I. $(SRCS) -o $(TARGET)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
