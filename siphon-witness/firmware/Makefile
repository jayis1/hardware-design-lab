# Author: jayis1. SPDX-License-Identifier: MIT
CROSS ?= arm-none-eabi-
CC = $(CROSS)gcc
FLAGS = -std=c11 -Os -g -mcpu=cortex-m4 -mthumb -ffreestanding -fno-builtin -Wall -Wextra -Werror -I.
SOURCES = main.c engine.c wire.c startup.c drivers/board.c drivers/serial.c drivers/indicator.c
.PHONY: all test clean
all: build/siphon-witness.bin
build:
	mkdir -p build
build/siphon-witness.elf: $(SOURCES) board.h registers.h engine.h wire.h drivers/indicator.h linker.ld | build
	$(CC) $(FLAGS) -nostdlib -Tlinker.ld -Wl,-Map=build/siphon-witness.map $(SOURCES) -lgcc -o $@
	$(CROSS)size $@
build/siphon-witness.bin: build/siphon-witness.elf
	$(CROSS)objcopy -O binary $< $@
test: | build
	gcc -std=c11 -Wall -Wextra -Werror -fsanitize=undefined,address -I. ../tests/test_engine.c engine.c wire.c -o build/test-engine
	./build/test-engine
clean:
	rm -rf build
