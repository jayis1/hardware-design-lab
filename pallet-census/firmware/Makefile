# Author: jayis1. SPDX-License-Identifier: MIT
CROSS ?= arm-none-eabi-
CC = $(CROSS)gcc
FLAGS = -std=c11 -Os -g -mcpu=cortex-m4 -mthumb -ffreestanding -fno-builtin -Wall -Wextra -Werror -I.
SOURCES = main.c engine.c wire.c startup.c drivers/board.c drivers/serial.c drivers/clock.c
.PHONY: all test clean flash fixture
all: build/pallet-census.bin
build:
	mkdir -p build
build/pallet-census.elf: $(SOURCES) board.h registers.h engine.h wire.h linker.ld | build
	$(CC) $(FLAGS) -nostdlib -Tlinker.ld -Wl,-Map=build/pallet-census.map $(SOURCES) -lgcc -o $@
	$(CROSS)size $@
build/pallet-census.bin: build/pallet-census.elf
	$(CROSS)objcopy -O binary $< $@
test: | build
	gcc -std=c11 -Wall -Wextra -Werror -fsanitize=undefined,address -I. ../tests/test_engine.c engine.c wire.c -o build/test-engine
	./build/test-engine
	gcc -std=c11 -Wall -Wextra -Werror -fsanitize=undefined,address -I. ../tests/test_clock.c -o build/test-clock
	./build/test-clock
	gcc -std=c11 -Wall -Wextra -Werror -fsanitize=undefined,address -I. ../tests/test_serial.c -o build/test-serial
	./build/test-serial
fixture: | build
	gcc -std=c11 -Wall -Wextra -Werror -I. ../tests/fixture.c engine.c wire.c -o build/fixture
	./build/fixture > build/demo.pc1
flash: build/pallet-census.elf
	openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c "program $< verify reset exit"
clean:
	rm -rf build
