/*
 * linker.ld — Linker Script for STM32H733VIT6
 *
 * LignoScan — Portable Acoustic Tomography Scanner
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 *
 * STM32H733: 1 MB Flash @ 0x08000000, 564 KB SRAM @ 0x20000000,
 * 128 KB DTCM @ 0x20000000, 8 MB QSPI Flash @ 0x90000000
 */

ENTRY(Reset_Handler)

/* Stack: top of SRAM2 (0x20020000) */
_estack = 0x20020000;

/* Minimum stack/heap sizes */
_Min_Stack_Size = 0x2000;    /* 8 KB stack */
_Min_Heap_Size  = 0x1000;    /* 4 KB heap */

MEMORY
{
    FLASH  (rx)  : ORIGIN = 0x08000000, LENGTH = 1024K
    DTCM   (xrw) : ORIGIN = 0x20000000, LENGTH = 128K
    SRAM1  (xrw) : ORIGIN = 0x20020000, LENGTH = 364K
    SRAM2  (xrw) : ORIGIN = 0x2007B000, LENGTH = 16K
    QSPI   (rx)  : ORIGIN = 0x90000000, LENGTH = 8192K
}

SECTIONS
{
    /* ---- Vector table (at start of flash) ---- */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    /* ---- Code ---- */
    .text :
    {
        . = ALIGN(4);
        *(.text)           /* .text sections (code) */
        *(.text*)
        *(.glue_7)
        *(.glue_7t)
        *(.eh_frame)
        KEEP(*(.init))
        KEEP(*(.fini))
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    /* ---- Read-only data ---- */
    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } > FLASH

    /* ---- ARM exception tables ---- */
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

    /* ---- Pre-init array ---- */
    .preinit_array :
    {
        PROVIDE_HIDDEN(__preinit_array_start = .);
        KEEP(*(.preinit_array*))
        PROVIDE_HIDDEN(__preinit_array_end = .);
    } > FLASH

    /* ---- Init array (C++ constructors) ---- */
    .init_array :
    {
        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array*))
        PROVIDE_HIDDEN(__init_array_end = .);
    } > FLASH

    /* ---- Fini array ---- */
    .fini_array :
    {
        PROVIDE_HIDDEN(__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array*))
        PROVIDE_HIDDEN(__fini_array_end = .);
    } > FLASH

    /* ---- SIDATA (init data for SRAM) ---- */
    _sidata = LOADADDR(.data);

    /* ---- Initialized data (copied from flash to SRAM at boot) ---- */
    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > SRAM1 AT > FLASH

    /* ---- BSS (zero-initialized data) ---- */
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

    /* ---- DTCM (fast data — tomography reconstruction buffers) ---- */
    .dtcm :
    {
        . = ALIGN(4);
        *(.dtcm)
        *(.dtcm*)
        . = ALIGN(4);
    } > DTCM

    /* ---- QSPI flash (waveform storage, calibration data) ---- */
    .qspi :
    {
        . = ALIGN(4);
        *(.qspi)
        *(.qspi*)
        . = ALIGN(4);
    } > QSPI

    /* ---- Remove unused sections ---- */
    /DISCARD/ :
    {
        *(.ARM.attributes)
        *(.comment)
    }

    /* ---- Check stack/heap sizes ---- */
    ASSERT(_Min_Stack_Size >= 0x200, "Stack too small")
    ASSERT(_Min_Heap_Size >= 0x100, "Heap too small")
}

/* EOF — linker.ld
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */