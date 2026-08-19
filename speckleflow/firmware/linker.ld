/*
 * linker.ld — Linker script for STM32H733VIT6 (SpeckleFlow)
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 *
 * Memory map:
 *   FLASH: 1 MB at 0x08000000 (dual-bank: 2 × 512 KB)
 *   ITCM:  64 KB at 0x00000000
 *   DTCM:  128 KB at 0x20000000
 *   SRAM:  864 KB at 0x24000000 (contiguous)
 *   SRAM2: 64 KB at 0x38000000 (backup domain)
 */

ENTRY(reset_handler)

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 1024K
    ITCM  (rwx) : ORIGIN = 0x00000000, LENGTH = 64K
    DTCM  (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
    SRAM  (rwx) : ORIGIN = 0x24000000, LENGTH = 864K
    SRAM2 (rwx) : ORIGIN = 0x38000000, LENGTH = 64K
}

_estack = ORIGIN(DTCM) + LENGTH(DTCM);

SECTIONS
{
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.rodata)
        *(.rodata*)
        *(.glue_7)
        *(.glue_7t)
        *(.eh_frame)
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > FLASH

    .ARM :
    {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    } > FLASH

    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > SRAM AT > FLASH

    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        __bss_start__ = _sbss;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
        __bss_end__ = _ebss;
    } > SRAM

    .heap_stack :
    {
        . = ALIGN(8);
        . = . + 0x4000;  /* 16 KB heap */
        _end = .;
    } > SRAM
}

_sidata = LOADADDR(.data);