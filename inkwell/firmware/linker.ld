/*
 * linker.ld — Linker script for Inkwell firmware on nRF52833
 *
 * Places code in flash starting at 0x0 and RAM at 0x20000000.
 * Reserves a 4 KB MBR page at the bottom of flash and a 24 KB DFU
 * bootloader region at the top, leaving the application 484 KB.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

MEMORY
{
    FLASH (rx) : ORIGIN = 0x00001000, LENGTH = 0x79000   /* 484 KB app */
    RAM   (rwx): ORIGIN = 0x20000000, LENGTH = 0x20000  /* 128 KB */
}

SECTIONS
{
    .text :
    {
        KEEP(*(.isr_vector))
        *(.text)
        *(.text.*)
        *(.rodata)
        *(.rodata.*)
        . = ALIGN(4);
    } > FLASH

    .ARM.exidx :
    {
        *(.ARM.exidx*)
    } > FLASH

    .data : AT(__data_lma)
    {
        . = ALIGN(4);
        __data_start = .;
        *(.data)
        *(.data.*)
        . = ALIGN(4);
        __data_end = .;
    } > RAM

    __data_lma = LOADADDR(.data);

    .bss (NOLOAD) :
    {
        . = ALIGN(4);
        __bss_start = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end = .;
    } > RAM

    .stack (NOLOAD) :
    {
        . = ALIGN(8);
        . += 0x1000;   /* 4 KB stack */
        __stack_top = .;
    } > RAM

    /DISCARD/ :
    {
        *(.comment)
        *(.note)
    }
}

ENTRY(Reset_Handler)

PROVIDE(__stack_top = ORIGIN(RAM) + LENGTH(RAM));