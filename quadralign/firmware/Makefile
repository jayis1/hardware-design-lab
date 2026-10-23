# QuadraAlign firmware build — Author: jayis1
CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror
LDLIBS := -lm
TARGET := quadralign
SOURCES := main.c sensors.c protocol.c board_sim.c
OBJECTS := $(SOURCES:.c=.o)

.PHONY: all clean test
all: $(TARGET)
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)
%.o: %.c board.h registers.h
	$(CC) $(CFLAGS) -c $< -o $@
test: $(TARGET)
	./$(TARGET) --self-test
clean:
	rm -f $(OBJECTS) $(TARGET)
