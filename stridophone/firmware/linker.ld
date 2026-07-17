/*
 * linker.ld — minimal linker script for STM32H743VIT6.
 *
 * Author : jayis1
 * License: MIT
 *
 * Flash : 2 MB starting at 0x08000000 (we use 0x08000000 for boot, dual-bank
 *         OTA would place bank B at 0x08100000).
 * RAM   : 1 MB SRAM starting at 0x24000000 (AXI SRAM, main).
 *         DTCM @ 0x20000000 (128 KB) and ITCM @ 0x00000000 (64 KB) available
 *         but we keep everything in AXI SRAM for simplicity.
 */
ENTRY(Reset_Handler)

_estack = 0x24000000 + 0x100000;   /* top of 1 MB AXI SRAM */

MEMORY
{
    FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 2048K
    RAM   (rwx): ORIGIN = 0x24000000, LENGTH = 1024K
}

SECTIONS
{
    .isr_vector :
    {
        KEEP(*(.isr_vector))
        KEEP(*(.isr_vector_tail))
    } > FLASH

    .text :
    {
        *(.text*)
        *(.rodata*)
        KEEP(*(.init))
        KEEP(*(.fini))
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    .ARM.exidx :
    {
        *(.ARM.exidx*)
    } > FLASH

    _sidata = LOADADDR(.data);
    .data : AT(_etext)
    {
        _sdata = .;
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM

    .bss (NOLOAD) :
    {
        _sbss = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > RAM

    .heap_stack (NOLOAD) :
    {
        . = ALIGN(8);
        . = . + 0x4000;       /* 16 KB heap */
        . = . + 0x1000;       /* 4 KB stack guard */
        __stack_top = .;
    } > RAM

    /DISCARD/ :
    {
        *(.comment)
        *(.note*)
    }
}