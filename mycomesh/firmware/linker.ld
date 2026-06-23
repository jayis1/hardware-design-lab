/*
 * linker.ld — Linker script for STM32L4R9ZI
 * MycoMesh — Open-Source Fungal Mycelium Electrophysiology & Chemical Sensing Network
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Memory map for STM32L4R9ZI:
 *   Flash: 2 MB at 0x08000000
 *   SRAM1: 192 KB at 0x20000000
 *   SRAM2: 64 KB at 0x10000000 (backed-up SRAM)
 *   CCM:   384 KB total (SRAM1 + SRAM2 contiguous at 0x20000000)
 */

ENTRY(Reset_Handler)

/* Stack starts at the top of SRAM. */
_estack = ORIGIN(RAM) + LENGTH(RAM);

/* Minimum stack/heap sizes. */
_Min_Heap_Size  = 0x1000;   /* 4 KB heap   */
_Min_Stack_Size = 0x1000;   /* 4 KB stack  */

MEMORY
{
    FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 2048K
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 640K
}

SECTIONS
{
    /* --- Interrupt vector table (at the start of flash) --- */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    /* --- Code and read-only data --- */
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

        /* C++ constructors (not used, but included for completeness). */
        KEEP(*(.init))
        KEEP(*(.fini))

        . = ALIGN(4);
        _etext = .;
    } > FLASH

    /* --- ARM exception tables --- */
    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > FLASH

    .ARM.exidx :
    {
        __exidx_start = .;
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
        __exidx_end = .;
    } > FLASH

    /* --- SIDATA (init data in flash, copied to RAM at startup) --- */
    _sidata = LOADADDR(.data);
    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM AT > FLASH

    /* --- BSS (zero-initialized data) --- */
    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        __bss_start__ = .;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
        __bss_end__ = .;
    } > RAM

    /* --- Heap and stack --- */
    ._user_heap_stack :
    {
        . = ALIGN(8);
        PROVIDE(end = .);
        PROVIDE(_end = .);
        . = . + _Min_Heap_Size;
        . = . + _Min_Stack_Size;
        . = ALIGN(8);
    } > RAM

    /* --- Discard sections we don't need --- */
    /DISCARD/ :
    {
        *(.ARM.attributes)
        *(.comment)
        *(.note*)
    }
}