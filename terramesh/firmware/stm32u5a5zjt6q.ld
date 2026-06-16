/**
 * @file    stm32u5a5zjt6q.ld
 * @brief   Terramesh — Linker script for STM32U5A5ZJT6Q
 * @author  jayis1
 * @copyright Copyright © 2026 jayis1
 * @license GPL-2.0
 *
 * Memory layout:
 *   Flash: 4 MB  (0x08000000 – 0x083FFFFF)
 *     - Bootloader: 32 KB  (0x08000000 – 0x08007FFF)
 *     - Application: 3904 KB (0x08008000 – 0x083EFFFF)
 *     - Log storage: 128 KB (0x083F0000 – 0x083FFFFF)
 *   SRAM1: 2.5 MB (0x30000000 – 0x3027FFFF)
 *   SRAM2: 64 KB  (0x20000000 – 0x2000FFFF) — backup domain
 */

MEMORY
{
    FLASH_BOOT  (rx)  : ORIGIN = 0x08000000, LENGTH = 32K
    FLASH_APP   (rx)  : ORIGIN = 0x08008000, LENGTH = 3904K
    FLASH_LOG   (r)   : ORIGIN = 0x083F0000, LENGTH = 128K
    SRAM1      (rwx)  : ORIGIN = 0x30000000, LENGTH = 2560K
    SRAM2      (rw)   : ORIGIN = 0x20000000, LENGTH = 64K
}

/* Entry point */
ENTRY(Reset_Handler)

/* Stack and heap sizes */
__stack_size  = 0x4000;   /* 16 KB stack */
__heap_size   = 0x8000;   /* 32 KB heap */

/* Section definitions */
SECTIONS
{
    /* Vector table */
    .isr_vector : ALIGN(256)
    {
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH_APP

    /* Code */
    .text : ALIGN(4)
    {
        *(.text)
        *(.text.*)
        *(.glue_7)
        *(.glue_7t)
        *(.gnu.linkonce.t.*)
        *(.rodata)
        *(.rodata.*)
        *(.gnu.linkonce.r.*)
        . = ALIGN(4);
    } > FLASH_APP

    /* Initialized data */
    .data : ALIGN(4)
    {
        __data_start = .;
        *(.data)
        *(.data.*)
        *(.gnu.linkonce.d.*)
        . = ALIGN(4);
        __data_end = .;
    } > SRAM1 AT > FLASH_APP

    __data_load = LOADADDR(.data);

    /* Uninitialized data */
    .bss : ALIGN(4)
    {
        __bss_start = .;
        *(.bss)
        *(.bss.*)
        *(.gnu.linkonce.b.*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end = .;
    } > SRAM1

    /* Heap */
    .heap : ALIGN(8)
    {
        __heap_start = .;
        . = . + __heap_size;
        . = ALIGN(8);
        __heap_end = .;
    } > SRAM1

    /* Stack */
    .stack : ALIGN(8)
    {
        __stack_start = .;
        . = . + __stack_size;
        . = ALIGN(8);
        __stack_end = .;
    } > SRAM1

    /* Log storage (read-only from firmware perspective) */
    .log_storage : ALIGN(4096)
    {
        KEEP(*(.log_storage))
    } > FLASH_LOG

    /* Discard unused sections */
    /DISCARD/ :
    {
        *(.ARM.exidx)
        *(.ARM.exidx.*)
        *(.ARM.attributes)
        *(.comment)
        *(.debug*)
    }
}

/* Provide symbols for startup code */
PROVIDE(__stack = __stack_end);
PROVIDE(__heap_base = __heap_start);
PROVIDE(__heap_limit = __heap_end);
