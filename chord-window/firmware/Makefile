# Author: jayis1. SPDX-License-Identifier: MIT
CROSS ?= arm-none-eabi-
CC = $(CROSS)gcc
FLAGS = -std=c11 -Os -g -mcpu=cortex-m4 -mthumb -ffreestanding -fno-builtin -Wall -Wextra -Werror -I.
SOURCES = main.c engine.c wire.c startup.c drivers/board.c drivers/serial.c drivers/clock.c
.PHONY: all test clean flash
all: build/chord-window.bin
build:
	mkdir -p build
build/chord-window.elf: $(SOURCES) board.h registers.h engine.h wire.h linker.ld | build
	$(CC) $(FLAGS) -nostdlib -Tlinker.ld -Wl,-Map=build/chord-window.map $(SOURCES) -lgcc -o $@
	$(CROSS)size $@
build/chord-window.bin: build/chord-window.elf
	$(CROSS)objcopy -O binary $< $@
test: | build
	gcc -std=c11 -Wall -Wextra -Werror -fsanitize=undefined,address -I. ../tests/test_engine.c engine.c wire.c -o build/test-engine
	./build/test-engine
	gcc -std=c11 -Wall -Wextra -Werror -fsanitize=undefined,address -I. ../tests/test_clock.c -o build/test-clock
	./build/test-clock
flash: build/chord-window.elf
	openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c "program $< verify reset exit"
clean:
	rm -rf build
