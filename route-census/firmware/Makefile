# Author: jayis1. SPDX-License-Identifier: MIT
CROSS ?= arm-none-eabi-
CC = $(CROSS)gcc
FLAGS = -std=c11 -Os -g -mcpu=cortex-m4 -mthumb -ffreestanding -fno-builtin -Wall -Wextra -Werror -I.
SOURCES = main.c engine.c wire.c startup.c drivers/board.c drivers/serial.c
.PHONY: all test clean flash
all: build/route-census.bin
build:
	mkdir -p build
build/route-census.elf: $(SOURCES) board.h registers.h engine.h wire.h linker.ld | build
	$(CC) $(FLAGS) -nostdlib -Tlinker.ld -Wl,-Map=build/route-census.map $(SOURCES) -lgcc -o $@
	$(CROSS)size $@
build/route-census.bin: build/route-census.elf
	$(CROSS)objcopy -O binary $< $@
test: | build
	gcc -std=c11 -Wall -Wextra -Werror -fsanitize=undefined,address -I. ../tests/test_engine.c engine.c wire.c -o build/test-engine
	./build/test-engine
	gcc -std=c11 -Wall -Wextra -Werror -fsanitize=undefined,address -I. ../tests/test_serial.c -o build/test-serial
	./build/test-serial
flash: build/route-census.elf
	openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c "program $< verify reset exit"
clean:
	rm -rf build
