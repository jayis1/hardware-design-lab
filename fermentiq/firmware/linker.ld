/*
 * linker.ld — Linker Script for FermenTiq (ESP32-S3)
 *
 * Defines memory regions for the ESP32-S3-WROOM-1-N16R8 module:
 *   - 16 MB external SPI flash (instruction + data)
 *   - 8 MB external PSRAM (octal SPI)
 *   - 512 KB internal SRAM
 *   - 128 KB RTC slow memory
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: GPL-3.0
 */

/* ESP32-S3 Memory Map */
MEMORY
{
    /* Internal SRAM (data + bss) */
    IRAM    (rw) : org = 0x40378000, len = 128K    /* Instruction RAM */
    DRAM    (rw) : org = 0x3FC88000, len = 384K    /* Data RAM */

    /* RTC memory (for deep sleep retention) */
    RTC_SLOW (rw) : org = 0x50002000, len = 16K

    /* External flash (via SPI, mapped to instruction/data bus) */
    IROM    (rx) : org = 0x42000000, len = 15M     /* Code in flash */
    DROM    (r)  : org = 0x3C000000, len = 4M      /* Read-only data */

    /* External PSRAM (octal SPI, 8 MB) */
    PSRAM   (rw) : org = 0x3C800000, len = 8M

    /* Application partition in flash */
    APP     (rx) : org = 0x42000000, len = 3M
    /* Factory NVS partition */
    NVS     (rw) : org = 0x42300000, len = 64K
    /* OTA data partition */
    OTA     (rw) : org = 0x42310000, len = 16K
    /* SPIFFS / model storage */
    STORAGE (rw) : org = 0x42314000, len = 512K
}

/* Entry point */
ENTRY(app_main)

/* Stack configuration */
_stack_top = ORIGIN(DRAM) + LENGTH(DRAM);
_stack_size = 0x2000;  /* 8 KB main stack */

/* Heap configuration — uses all remaining DRAM + PSRAM */
_heap_start = .;
_heap_end = ORIGIN(DRAM) + LENGTH(DRAM) - _stack_size;
_heap_size = _heap_end - _heap_start;

_psram_heap_start = ORIGIN(PSRAM);
_psram_heap_size  = LENGTH(PSRAM);

/* Sections */
SECTIONS
{
    /* Code */
    .text : {
        *(.text.app_main)
        *(.text*)
        *(.rodata*)
    } > IROM

    /* Data */
    .data : {
        *(.data*)
        *(.sdata*)
    } > DRAM AT > DROM

    /* BSS */
    .bss : {
        _bss_start = .;
        *(.bss*)
        *(.sbss*)
        *(COMMON)
        _bss_end = .;
    } > DRAM

    /* Heap area marker */
    .heap (NOLOAD) : {
        . = ALIGN(4);
        _heap_start = .;
        . = . + _heap_size;
        _heap_end = .;
    } > DRAM

    /* Stack area marker */
    .stack (NOLOAD) : {
        . = ALIGN(8);
        _stack_bottom = .;
        . = . + _stack_size;
        _stack_top = .;
    } > DRAM

    /* PSRAM heap (for large buffers: audio FFT, ML model) */
    .psram_heap (NOLOAD) : {
        _psram_start = .;
        . = . + _psram_heap_size;
    } > PSRAM

    /* RTC slow memory (retained during deep sleep) */
    .rtc_slow (NOLOAD) : {
        *(.rtc_slow*)
    } > RTC_SLOW
}

/* Provide symbols for the startup code */
PROVIDE(_bss_start = _bss_start);
PROVIDE(_bss_end = _bss_end);
PROVIDE(_stack_top = _stack_top);
PROVIDE(_heap_start = _heap_start);
PROVIDE(_heap_end = _heap_end);
PROVIDE(_psram_start = _psram_start);
PROVIDE(_psram_size = _psram_heap_size);