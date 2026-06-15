# PicoRadar Firmware Makefile
# Cross-compile for STM32H743 (Cortex-M7, arm-none-eabi)

# ---- Toolchain ----
CC      = arm-none-eabi-gcc
AS      = arm-none-eabi-as
LD      = arm-none-eabi-ld
OBJCOPY = arm-none-eabi-objcopy
SIZE    = arm-none-eabi-size

# ---- CPU flags ----
CPU_FLAGS = -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard

# ---- Include paths ----
INCDIRS = -I. -Idrivers

# ---- Compiler flags ----
CFLAGS  = $(CPU_FLAGS) -Wall -Wextra -Wno-unused-parameter -O2 -g3 \
          -ffunction-sections -fdata-sections -fno-common \
          -ffreestanding -nostdlib -nostartfiles \
          $(INCDIRS)

# ---- Linker flags ----
LDFLAGS = $(CPU_FLAGS) -T stm32h743vit6.ld -Wl,--gc-sections \
          -Wl,--no-wchar-size-warning -Wl,--print-memory-usage

# ---- Source files ----
CSRCS   = main.c \
          drivers/imu_icm42688.c \
          drivers/oled_sh1106.c

ASRCS   = startup_stm32h743.s

# ---- Object files ----
COBJS   = $(CSRCS:.c=.o)
AOBJS   = $(ASRCS:.s=.o)
OBJS    = $(AOBJS) $(COBJS)

# ---- Output ----
BUILDDIR = build
TARGET   = $(BUILDDIR)/pico-radar

# ---- Targets ----

.PHONY: all clean flash size

all: $(TARGET).bin $(TARGET).hex size

$(TARGET).elf: $(OBJS) stm32h743vit6.ld | $(BUILDDIR)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) -lgcc

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.s
	$(AS) $(CPU_FLAGS) -o $@ $<

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

size: $(TARGET).elf
	$(SIZE) $<

flash: $(TARGET).elf
	openocd -f interface/stlink.cfg -f target/stm32h7x.cfg \
		-c "program $< verify reset exit"

clean:
	rm -f $(OBJS) $(TARGET).elf $(TARGET).bin $(TARGET).hex
	rm -rf $(BUILDDIR)

# ---- Dependencies (simplified) ----
main.o: main.c registers.h board.h drivers/imu_icm42688.h drivers/oled_sh1106.h
drivers/imu_icm42688.o: drivers/imu_icm42688.c drivers/imu_icm42688.h registers.h board.h
drivers/oled_sh1106.o: drivers/oled_sh1106.c drivers/oled_sh1106.h registers.h board.h