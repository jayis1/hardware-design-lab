/*
 * link.ld — Linker script for SpectraPest firmware (STM32H753)
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 *
 * Memory layout:
 *   FLASH: 2 MB at 0x08000000
 *     - Bootloader: 64 KB (0x08000000 - 0x0800FFFF)
 *     - Firmware: 448 KB (0x08010000 - 0x0807FFFF)
 *     - ML model: 128 KB (0x08080000 - 0x0809FFFF)
 *     - Config: 384 KB (0x080A0000 - 0x080FFFFF)
 *   SRAM: 1 MB at 0x20000000
 *   SRAM2: 128 KB at 0x10000000 (non-cacheable, for DMA buffers)
 */

ENTRY(Reset_Handler)

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08010000, LENGTH = 448K
    ML    (r)   : ORIGIN = 0x08080000, LENGTH = 128K
    CONFIG (rw) : ORIGIN = 0x080A0000, LENGTH = 384K
    SRAM  (rwx) : ORIGIN = 0x20000000, LENGTH = 896K
    SRAM2 (rwx) : ORIGIN = 0x10000000, LENGTH = 128K
}

/* Stack at top of SRAM */
_stack_top = ORIGIN(SRAM) + LENGTH(SRAM);

SECTIONS
{
    /* Vector table */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    /* Code */
    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.glue_7)
        *(.glue_7t)
        *(.eh_frame)
        . = ALIGN(4);
    } > FLASH

    /* Read-only data */
    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } > FLASH

    /* ML model weights (loaded to SRAM at runtime) */
    .ml_weights : ALIGN(4)
    {
        KEEP(*(.ml_weights))
    } > ML

    /* Configuration data */
    .config :
    {
        . = ALIGN(4);
        KEEP(*(.config))
        . = ALIGN(4);
    } > CONFIG

    /* ARM attributes */
    .ARM.attributes :
    {
        *(.ARM.attributes)
    } > FLASH

    /* Init array */
    .init_array :
    {
        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array*))
        PROVIDE_HIDDEN(__init_array_end = .);
    } > FLASH

    /* Initialized data (copied from FLASH to SRAM at startup) */
    _sidata = LOADADDR(.data);
    .data : ALIGN(4)
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > SRAM AT > FLASH

    /* BSS (zero-initialized) */
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
    } > SRAM

    /* DMA buffers in non-cacheable SRAM2 */
    .dma_buf (NOLOAD) :
    {
        . = ALIGN(32);
        *(.dma_buf)
        *(.dma_buf*)
        . = ALIGN(32);
    } > SRAM2

    /* Stack */
    .stack (NOLOAD) :
    {
        . = ALIGN(8);
        _sstack = .;
        . = . + 64K;
        . = ALIGN(8);
        _estack = .;
    } > SRAM

    /* Heap */
    .heap (NOLOAD) :
    {
        . = ALIGN(8);
        _sheap = .;
        . = . + 64K;
        . = ALIGN(8);
        _eheap = .;
    } > SRAM

    /* Discard */
    /DISCARD/ :
    {
        *(.comment)
        *(.ARM.exidx*)
    }
}