/*
 * linker.ld — RootTrace STM32H743 Linker Script
 * RootTrace — Open-Source Electrical Impedance Tomography Root Imaging System
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 *
 * Memory map for STM32H743ZIT6:
 *   Flash (Bank 1): 0x08000000, 1 MB
 *   Flash (Bank 2): 0x08100000, 1 MB
 *   DTCM RAM:        0x20000000, 128 KB
 *   ITCM RAM:         0x00000000, 64 KB
 *   SRAM1:            0x24000000, 512 KB
 *   SRAM2:            0x24080000, 128 KB
 *   SRAM4:            0x38010000, 64 KB
 *
 * The firmware is linked to run from Bank 1 flash with main code/data
 * in SRAM1 and time-critical stacks in DTCM.
 */

ENTRY(Reset_Handler)

MEMORY
{
    ITCM  (rx) : ORIGIN = 0x00000000, LENGTH = 64K
    FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 1024K
    DTCM  (rw) : ORIGIN = 0x20000000, LENGTH = 128K
    SRAM1 (rwx): ORIGIN = 0x24000000, LENGTH = 512K
    SRAM2 (rwx): ORIGIN = 0x24080000, LENGTH = 128K
    SRAM4 (rw) : ORIGIN = 0x38010000, LENGTH = 64K
}

/* Stack sizes */
STACK_SIZE = 0x4000;      /* 16 KB main stack in DTCM */
MIN_HEAP_SIZE = 0x2000;   /* 8 KB minimum heap */

/* Section definitions */
SECTIONS
{
    /* Vector table — must be at the start of flash */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    /* Code and read-only data */
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

    /* ARM exception unwinding tables */
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

    /* ITCM code (for time-critical functions) */
    .itcm_text :
    {
        . = ALIGN(4);
        *(.itcm_text)
        *(.itcm_text*)
        . = ALIGN(4);
    } > ITCM AT > FLASH

    /* Initialization data copied from flash to SRAM at startup */
    _sidata = LOADADDR(.data);
    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > SRAM1 AT > FLASH

    /* DTCM data (fast access) */
    .dtcm_data :
    {
        . = ALIGN(4);
        *(.dtcm_data)
        *(.dtcm_data*)
        . = ALIGN(4);
    } > DTCM AT > FLASH

    /* BSS (zero-initialized) */
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
    } > SRAM1

    /* DTCM BSS */
    .dtcm_bss (NOLOAD) :
    {
        . = ALIGN(4);
        *(.dtcm_bss)
        *(.dtcm_bss*)
        . = ALIGN(4);
    } > DTCM

    /* Heap section */
    .heap (NOLOAD) :
    {
        . = ALIGN(8);
        _sheap = .;
        . = . + MIN_HEAP_SIZE;
        . = ALIGN(8);
        _eheap = .;
    } > SRAM1

    /* Stack in DTCM for fast access */
    .dtcm_stack (NOLOAD) :
    {
        . = ALIGN(8);
        _estack = ORIGIN(DTCM) + LENGTH(DTCM);
        . = ORIGIN(DTCM) + LENGTH(DTCM) - STACK_SIZE;
        _sstack = .;
        . = . + STACK_SIZE;
        . = ALIGN(8);
    } > DTCM

    /* SRAM2: used for DMA buffers (non-cached) */
    .sram2 (NOLOAD) :
    {
        . = ALIGN(4);
        *(.sram2)
        *(.sram2*)
        . = ALIGN(4);
    } > SRAM2

    /* SRAM4: backup RAM (battery-backed) */
    .sram4 (NOLOAD) :
    {
        . = ALIGN(4);
        *(.sram4)
        *(.sram4*)
        . = ALIGN(4);
    } > SRAM4

    /* Discard debugging info */
    /DISCARD/ :
    {
        *(.ARM.attributes)
        *(.comment)
        *(.note*)
    }
}

/* Provide stack pointer initial value */
PROVIDE(_stack_top = ORIGIN(DTCM) + LENGTH(DTCM));

/* Assertions */
ASSERT(_estack >= _sstack, "DTCM stack overlap")
ASSERT(_eheap >= _sheap, "Heap section error")