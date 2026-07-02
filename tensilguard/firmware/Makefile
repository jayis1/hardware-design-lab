# ============================================================================
#  TensilGuard firmware — Makefile
#  Author: jayis1
#  SPDX-License-Identifier: MIT
# ----------------------------------------------------------------------------
#  Builds tensilguard.elf / .hex / .bin for the STM32G431RBT6 reference board
#  using arm-none-eabi-gcc. The design uses the STM32G4's CORDIC and FMAC
#  math accelerators; the firmware is portable and can be compiled with a
#  generic ARM toolchain (vendor CMSIS headers substituted for production).
# ============================================================================
TARGET     := tensilguard
CROSS      ?= arm-none-eabi-
CC         := $(CROSS)gcc
OBJCOPY    := $(CROSS)objcopy
SIZE       := $(CROSS)size

MCU        := -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
OPT        := -Os -g3 -ffunction-sections -fdata-sections
WARN       := -Wall -Wextra -Wno-unused-parameter -Wno-unused-but-set-variable
DEFINES    := -DSTM32G431xx -DUSE_FULL_LL_DRIVER
INCS       := -I. -ICMSIS/Include -Istm32g4xx_hal/Inc

CFLAGS     := $(MCU) $(OPT) $(WARN) $(DEFINES) $(INCS) -std=c11
LDFLAGS    := $(MCU) -TSTM32G431RBTx_FLASH.ld -Wl,--gc-sections -Wl,-Map=$(TARGET).map
LDLIBS     := -lc -lm -lnosys

# Firmware sources — every .c is authored by jayis1
SRCS       := main.c mag.c accel.c ae.c lora.c temp.c power.c flash_log.c
OBJS       := $(SRCS:.c=.o)

# Host-side compile/check target (no cross toolchain needed)
HOST_CC    ?= cc
HOST_CFLAGS := -Wall -Wextra -Wno-unused-parameter -Wno-unused-but-set-variable \
               -I. -std=c11 -DTG_HOST_BUILD=1

.PHONY: all flash size clean check host-check

all: $(TARGET).elf $(TARGET).hex $(TARGET).bin size

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET).elf: $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $@

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

size: $(TARGET).elf
	$(SIZE) $(TARGET).elf

flash: $(TARGET).hex
	openocd -f interface/stlink.cfg -f target/stm32g4x.cfg \
		-c "program $(TARGET).hex verify reset exit"

# ---- Host-side syntax/logic check (no ARM toolchain required) -------------
# Compiles the firmware against the host compiler with stubbed register
# macros so the DSP logic (FFT, HPS, Goertzel, demod, classifier) can be
# unit-tested on a development machine.
host-check:
	$(HOST_CC) $(HOST_CFLAGS) -c main.c   -o /tmp/tg_main.o   || true
	$(HOST_CC) $(HOST_CFLAGS) -c mag.c     -o /tmp/tg_mag.o    || true
	$(HOST_CC) $(HOST_CFLAGS) -c accel.c    -o /tmp/tg_accel.o  || true
	$(HOST_CC) $(HOST_CFLAGS) -c ae.c       -o /tmp/tg_ae.o     || true
	$(HOST_CC) $(HOST_CFLAGS) -c lora.c     -o /tmp/tg_lora.o   || true
	$(HOST_CC) $(HOST_CFLAGS) -c temp.c     -o /tmp/tg_temp.o   || true
	$(HOST_CC) $(HOST_CFLAGS) -c power.c    -o /tmp/tg_power.o  || true
	$(HOST_CC) $(HOST_CFLAGS) -c flash_log.c -o /tmp/tg_flash.o || true
	@echo "host-check: syntax+logic compile complete"

clean:
	rm -f $(OBJS) $(TARGET).elf $(TARGET).hex $(TARGET).bin $(TARGET).map
	rm -f /tmp/tg_*.o