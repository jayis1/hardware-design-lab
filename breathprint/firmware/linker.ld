/*
 * linker.ld — Linker script for STM32H733VGT6
 * BreathPrint — Portable Breath VOC Analyzer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 *
 * STM32H733 has 1 MB dual-bank flash and 240 KB DTCM + 512 KB SRAM.
 * This linker script places code in Bank 1 (0x08000000) and
 * reserves Bank 2 (0x08080000) for OTA updates.
 */

ENTRY(Reset_Handler)

/* ========================================================================
 * Memory Regions
 * ======================================================================== */

MEMORY
{
    /* Flash Bank 1: 512 KB at 0x08000000 (active firmware) */
    FLASH_BANK1 (rx)  : ORIGIN = 0x08000000, LENGTH = 512K

    /* Flash Bank 2: 512 KB at 0x08080000 (OTA update target) */
    FLASH_BANK2 (rx)  : ORIGIN = 0x08080000, LENGTH = 512K

    /* DTCM RAM: 128 KB at 0x20000000 (fastest, for stack + critical data) */
    DTCM (rwx)        : ORIGIN = 0x20000000, LENGTH = 128K

    /* SRAM1+SRAM2: 240 KB at 0x24000000 (general purpose + ML model) */
    SRAM (rwx)        : ORIGIN = 0x24000000, LENGTH = 240K

    /* SRAM4: 64 KB at 0x38000000 (backup domain, low-power) */
    SRAM4 (rwx)       : ORIGIN = 0x38000000, LENGTH = 64K
}

/* ========================================================================
 * Stack and Heap
 * ======================================================================== */

_estack = ORIGIN(DTCM) + LENGTH(DTCM);
_Min_Heap_Size = 0x1000;    /* 4 KB heap */
_Min_Stack_Size = 0x2000;   /* 8 KB stack */

/* ========================================================================
 * Sections
 * ======================================================================== */

SECTIONS
{
    /* --- Interrupt Vector Table --- */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } >FLASH_BANK1

    /* --- Text section (code) --- */
    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.glue_7)
        *(.glue_7t)
        *(.eh_frame)

        KEEP(*(.init))
        KEEP(*(.fini))

        . = ALIGN(4);
        _etext = .;
    } >FLASH_BANK1

    /* --- Read-only data --- */
    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } >FLASH_BANK1

    /* --- ARM exception tables --- */
    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } >FLASH_BANK1

    .ARM :
    {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    } >FLASH_BANK1

    /* --- Preinit array --- */
    .preinit_array :
    {
        PROVIDE_HIDDEN(__preinit_array_start = .);
        KEEP(*(.preinit_array*))
        PROVIDE_HIDDEN(__preinit_array_end = .);
    } >FLASH_BANK1

    /* --- Init array --- */
    .init_array :
    {
        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array*))
        PROVIDE_HIDDEN(__init_array_end = .);
    } >FLASH_BANK1

    /* --- Fini array --- */
    .fini_array :
    {
        PROVIDE_HIDDEN(__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array*))
        PROVIDE_HIDDEN(__fini_array_end = .);
    } >FLASH_BANK1

    /* --- DTCM data (fast access, copied from flash at boot) --- */
    _sidtcm = LOADADDR(.dtcm);
    .dtcm :
    {
        . = ALIGN(4);
        _sdtcm = .;
        *(.dtcm)
        *(.dtcm*)
        . = ALIGN(4);
        _edtcm = .;
    } >DTCM AT>FLASH_BANK1

    /* --- SRAM data --- */
    _sidata = LOADADDR(.data);
    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } >SRAM AT>FLASH_BANK1

    /* --- BSS (zero-initialized) --- */
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
    } >SRAM

    /* --- ML Model section (large, in SRAM) --- */
    .ml_model :
    {
        . = ALIGN(4);
        _sml_model = .;
        . = . + 0x20000;   /* Reserve 128 KB for ML model in SRAM */
        . = ALIGN(4);
        _eml_model = .;
    } >SRAM

    /* --- Heap --- */
    .heap :
    {
        . = ALIGN(4);
        _sheap = .;
        . = . + _Min_Heap_Size;
        . = ALIGN(4);
        _eheap = .;
    } >SRAM

    /* --- DTCM stack --- */
    .stack :
    {
        . = ALIGN(4);
        _sstack = .;
        . = . + _Min_Stack_Size;
        . = ALIGN(4);
        _estack_section = .;
    } >DTCM

    /* --- Backup SRAM4 (retained in stop mode) --- */
    .backup :
    {
        . = ALIGN(4);
        *(.backup)
        *(.backup*)
        . = ALIGN(4);
    } >SRAM4

    /* --- Discard --- */
    /DISCARD/ :
    {
        libc.a (*)
        libm.a (*)
        libgcc.a (*)
    }
}

/* ========================================================================
 * Provide stack/heap symbols
 * ======================================================================== */

PROVIDE(_estack = ORIGIN(DTCM) + LENGTH(DTCM));
PROVIDE(__StackTop = _estack);
PROVIDE(__StackLimit = _estack - _Min_Stack_Size);
PROVIDE(__HeapBase = _sheap);
PROVIDE(__HeapLimit = _eheap);