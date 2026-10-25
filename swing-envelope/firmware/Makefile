# Author: jayis1. SPDX-License-Identifier: MIT
CROSS ?= arm-none-eabi-
CC = $(CROSS)gcc
CFLAGS = -std=c11 -Os -g -mcpu=cortex-m4 -mthumb -ffreestanding -fno-builtin -Wall -Wextra -Werror -I.
SOURCES = main.c engine.c wire.c startup.c drivers/board.c drivers/serial.c
.PHONY: all test clean flash
all: build/swing-envelope.bin
build:
	mkdir -p build
build/swing-envelope.elf: $(SOURCES) board.h registers.h engine.h wire.h linker.ld | build
	$(CC) $(CFLAGS) -nostdlib -Tlinker.ld -Wl,-Map=build/swing-envelope.map $(SOURCES) -lgcc -o $@
	$(CROSS)size $@
build/swing-envelope.bin: build/swing-envelope.elf
	$(CROSS)objcopy -O binary $< $@
test: | build
	gcc -std=c11 -Wall -Wextra -Werror -fsanitize=undefined,address -I. ../tests/test_engine.c engine.c wire.c -o build/test-engine
	./build/test-engine
	gcc -std=c11 -Wall -Wextra -Werror -I. ../tests/fixture.c engine.c wire.c -o build/fixture
	./build/fixture > build/fixture.txt
flash: all
	openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c "program build/swing-envelope.elf verify reset exit"
clean:
	rm -rf build
