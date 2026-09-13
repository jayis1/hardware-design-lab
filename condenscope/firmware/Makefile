# CondenScope firmware build
# Author: jayis1

PROJECT := condenscope
CC ?= arm-none-eabi-gcc
OBJCOPY ?= arm-none-eabi-objcopy
CFLAGS := -std=c11 -Os -g3 -Wall -Wextra -Werror -mcpu=cortex-m33 -mthumb \
          -mfloat-abi=hard -mfpu=fpv5-sp-d16 -ffunction-sections -fdata-sections \
          -I. -Idrivers -DSTM32U585xx
LDFLAGS := -mcpu=cortex-m33 -mthumb -Wl,--gc-sections -Wl,-Map,$(PROJECT).map \
           -Tstm32u585.ld --specs=nano.specs --specs=nosys.specs
SOURCES := main.c drivers/sensors.c drivers/protocol.c
OBJECTS := $(SOURCES:.c=.o)

all: $(PROJECT).elf $(PROJECT).bin

$(PROJECT).elf: $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

$(PROJECT).bin: $(PROJECT).elf
	$(OBJCOPY) -O binary $< $@

%.o: %.c board.h registers.h
	$(CC) $(CFLAGS) -c $< -o $@

syntax:
	gcc -std=c11 -Wall -Wextra -Werror -I. -Idrivers -fsyntax-only $(SOURCES)

clean:
	rm -f $(OBJECTS) $(PROJECT).elf $(PROJECT).bin $(PROJECT).map

.PHONY: all syntax clean
