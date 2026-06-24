/*
 * linker.ld — STM32H743 linker script
 * WattLens — 3-Phase Power Quality Analyzer & NILM
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */

ENTRY(Reset_Handler)

MEMORY
{
    ITCM  (rwx) : ORIGIN = 0x00000000, LENGTH = 64K    /* Instruction TCM */
    DTCM  (rwx) : ORIGIN = 0x20000000, LENGTH = 128K   /* Data TCM */
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 2048K  /* Main flash 2 MB */
    SRAM1 (rwx) : ORIGIN = 0x30000000, LENGTH = 128K   /* D2 SRAM1 */
    SRAM2 (rwx) : ORIGIN = 0x30020000, LENGTH = 128K   /* D2 SRAM2 */
    SRAM3 (rwx) : ORIGIN = 0x30040000, LENGTH = 32K    /* D2 SRAM3 */
    SRAM4 (rwx) : ORIGIN = 0x38000000, LENGTH = 64K    /* D3 SRAM4 (backup) */
}

/* Stack in DTCM */
_stack_top = ORIGIN(DTCM) + LENGTH(DTCM);

SECTIONS
{
    /* Interrupt vector table at flash start */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    /* Code — placed in ITCM for zero-wait-state execution */
    .text :
    {
        . = ALIGN(4);
        *(.text)           /* .text sections (code) */
        *(.text*)
        *(.rodata)         /* read-only data (constants, strings) */
        *(.rodata*)
        *(.glue_7)
        *(.glue_7t)
        *(.eh_frame)
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    /* Critical DSP code in ITCM for deterministic latency */
    .itcm_text :
    {
        . = ALIGN(4);
        *(.itcm)
        *(.itcm*)
        . = ALIGN(4);
    } > ITCM AT > FLASH

    /* ARM vector table attributes */
    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > FLASH

    .ARM :
    {
        __exidx_start = .;
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
        __exidx_end = .;
    } > FLASH

    /* Data — SRAM1 (initial values copied from flash) */
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

    /* BSS — zero-initialized */
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
    } > SRAM1

    /* Heap grows upward from end of BSS */
    .heap (NOLOAD) :
    {
        . = ALIGN(8);
        _sheap = .;
        . = . + 0x10000;     /* 64 KB heap */
        . = ALIGN(8);
        _eheap = .;
    } > SRAM1

    /* Stack — at top of DTCM */
    .stack (NOLOAD) :
    {
        . = ALIGN(8);
        _sstack = .;
        . = ORIGIN(DTCM) + LENGTH(DTCM);
        _estack = .;
    } > DTCM

    /* DMA buffers in SRAM2 — must be accessible by DMA controller */
    .dma_buffers (NOLOAD) :
    {
        . = ALIGN(32);
        *(.dma_buffers)
        *(.dma_buffers*)
        . = ALIGN(32);
    } > SRAM2

    /* NILM weight arrays in SRAM3 (large const arrays) */
    .nilm_weights (NOLOAD) :
    {
        . = ALIGN(4);
        *(.nilm_weights)
        *(.nilm_weights*)
        . = ALIGN(4);
    } > SRAM3

    /* Discard unused */
    /DISCARD/ :
    {
        *(.ARM.MEMATTR)
        *(.comment)
        *(.note*)
    }
}

/* Provide aliases for startup code */
PROVIDE(_stack_top = _estack);
PROVIDE(end = _eheap);