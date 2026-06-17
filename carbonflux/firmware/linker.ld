/**
 * linker.ld — CarbonFlux STM32U5A9ZG Linker Script
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 *
 * Memory map:
 *   Region        Start         Size      Contents
 *   BOOTLOADER    0x08000000    16 kB     USB DFU bootloader
 *   FLASH_APP     0x08004000    240 kB    Application firmware
 *   FLASH_CONFIG  0x08040000    8 kB      Calibration + keys (encrypted)
 *   FLASH_SAFE    0x08042000    8 kB      Secure backup (TrustZone)
 *   SRAM1         0x20000000    768 kB    Main data (128 kB .data/.bss)
 *                                256 kB CO₂ buffer
 *                                384 kB Free
 *   SRAM2         0x20080000    256 kB    QSPI cache / display
 *   SRAM4         0x28000000    64 kB     Backup SRAM (retained, not used)
 */

MEMORY
{
    BOOTLOADER (rx)  : ORIGIN = 0x08000000, LENGTH = 16K
    FLASH      (rx)  : ORIGIN = 0x08004000, LENGTH = 240K
    CONFIG     (rx)  : ORIGIN = 0x08040000, LENGTH = 8K
    FLASH_SAFE (rx)  : ORIGIN = 0x08042000, LENGTH = 8K
    RAM       (rwx)  : ORIGIN = 0x20000000, LENGTH = 768K
    RAM2      (rwx)  : ORIGIN = 0x20080000, LENGTH = 256K
    BKP_SRAM  (rw)   : ORIGIN = 0x28000000, LENGTH = 64K
}

/* Stack and heap sizes */
__stack_size = 4096;
__heap_size  = 2048;

/* Entry point */
ENTRY(Reset_Handler)

SECTIONS
{
    /* .text: application code and read-only data */
    .text :
    {
        . = ALIGN(4);
        _stext = .;
        KEEP(*(.isr_vector))           /* Interrupt vector table */
        *(.text*)                       /* Program code */
        *(.rodata*)                     /* Read-only data */
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    /* .init_array: C++ constructors (unused but present) */
    .preinit_array :
    {
        . = ALIGN(4);
        __preinit_array_start = .;
        KEEP(*(.preinit_array))
        __preinit_array_end = .;
    } > FLASH

    .init_array :
    {
        . = ALIGN(4);
        __init_array_start = .;
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
        __init_array_end = .;
    } > FLASH

    /* .fini_array: C++ destructors */
    .fini_array :
    {
        . = ALIGN(4);
        __fini_array_start = .;
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array))
        __fini_array_end = .;
    } > FLASH

    /* .data: initialized data (copied from FLASH to RAM at startup) */
    . = ALIGN(4);
    _sidata = LOADADDR(.data);

    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM AT > FLASH

    /* .bss: zero-initialized data */
    . = ALIGN(4);
    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        __bss_start__ = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
        __bss_end__ = .;
    } > RAM

    /* Heap */
    . = ALIGN(8);
    .heap :
    {
        _heap_start = .;
        . = . + __heap_size;
        _heap_end = .;
    } > RAM

    /* Stack */
    . = ALIGN(8);
    .stack :
    {
        _stack_start = .;
        . = . + __stack_size;
        . = ALIGN(8);
        _estack = .;
    } > RAM

    /* Discard unused sections */
    /DISCARD/ :
    {
        *(.ARM.exidx*)
        *(.ARM.attributes*)
        *(.comment*)
        *(.debug*)   /* Debug sections stripped for release builds */
    }
}

/* Export symbols for startup code */
PROVIDE(_sidata = LOADADDR(.data));
PROVIDE(_sdata = .data);
PROVIDE(_edata = .data + SIZEOF(.data));
PROVIDE(_sbss = .bss);
PROVIDE(_ebss = .bss + SIZEOF(.bss));