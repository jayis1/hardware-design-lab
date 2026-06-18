/*
 * linkerscript.ld - STM32H743VIT6 memory layout
 * Flash 2 MB at 0x08000000, DTCM 128 KB at 0x20000000,
 * SRAM1+SRAM2 512 KB at 0x30000000 (contiguous), SRAM4 64 KB at 0x38000000.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 */
ENTRY(Reset_Handler)

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 2048K
    DTCM  (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
    SRAM  (rwx) : ORIGIN = 0x30000000, LENGTH = 512K
    SRAM4 (rwx) : ORIGIN = 0x38000000, LENGTH = 64K
}

_estack = ORIGIN(DTCM) + LENGTH(DTCM);

SECTIONS
{
    .isr_vector : {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    .text : {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    .data : AT (_etext) {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > SRAM

    .bss (NOLOAD) : {
        . = ALIGN(4);
        _sbss = .;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > SRAM

    .heap (NOLOAD) : {
        . = ALIGN(8);
        _sheap = .;
        . = . + 64K;
        _eheap = .;
    } > SRAM
}