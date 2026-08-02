/*
 * linker.ld — STM32H733VG linker script for HydraScan
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 *
 * 1 MB flash at 0x08000000, 564 KB SRAM. QSPI XIP region aliased at
 * 0x90000000 for the liquid library.
 */
ENTRY(Reset_Handler)

MEMORY
{
    FLASH  (rx)  : ORIGIN = 0x08000000, LENGTH = 1024K
    RAM    (rwx) : ORIGIN = 0x24000000, LENGTH = 512K
    DTCM   (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
    ITCM   (rx)  : ORIGIN = 0x00000000, LENGTH = 16K
}

_estack = ORIGIN(RAM) + LENGTH(RAM);

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
        *(.text*)
        *(.rodata*)
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    .ARM.extab : { *(.ARM.extab* .gnu.linkonce.armextab.*) } > FLASH
    .ARM       : { __exidx_start = .; *(.ARM.exidx*) __exidx_end = .; } > FLASH

    .data : AT (_etext)
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM

    .bss (NOLOAD) :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > RAM

    /DISCARD/ :
    {
        *(.ARM.attributes)
    }
}

PROVIDE(_sdata_load = LOADADDR(.data));