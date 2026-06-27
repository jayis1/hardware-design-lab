/*
 * linker.ld — Linker script for nRF5340 Application Core
 * TremorSense — Wearable Tremor Characterization Band
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 *
 * Memory layout for nRF5340 QFAA variant:
 *   App core: 1 MB Flash (0x00000000), 512 KB RAM (0x20000000)
 *   Net core: 256 KB Flash (0x01000000), 64 KB RAM (0x01000000)
 *
 * This linker script is for the application core.
 */

MEMORY
{
    /* Application core flash: 1 MB at 0x00000000 */
    FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 0x100000

    /* Application core RAM: 512 KB at 0x20000000 */
    RAM  (rwx) : ORIGIN = 0x20000000, LENGTH = 0x80000

    /* Network core RAM (shared IPC region): 64 KB at 0x20010000
     * Used for RPMsg communication between app and net cores.
     */
    NET_RAM (rwx) : ORIGIN = 0x20010000, LENGTH = 0x10000

    /* External SPI flash (8 MB) — mapped via XiP or accessed via driver */
    EXT_FLASH (rx) : ORIGIN = 0x10000000, LENGTH = 0x800000

    /* Non-Volatile Memory (NVMC) — configuration storage */
    NVM (rwx) : ORIGIN = 0x000F0000, LENGTH = 0x10000  /* Last 64 KB of app flash */
}

/* Entry point */
ENTRY(Reset_Handler)

/* Stack configuration */
STACK_SIZE = 0x4000;  /* 16 KB stack */

/* Heap configuration (TremorSense uses minimal heap) */
HEAP_SIZE = 0x0200;  /* 512 bytes */

SECTIONS
{
    /* ---- Vector table ---- */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    /* ---- Firmware metadata header (for secure boot verification) ---- */
    .firmware_metadata :
    {
        . = ALIGN(4);
        KEEP(*(.firmware_metadata))
        . = ALIGN(4);
    } > FLASH

    /* ---- Code ---- */
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

        /* Keep ML model weights in flash */
        . = ALIGN(4);
        *(.ml_weights)
        *(.ml_weights*)

        . = ALIGN(4);
        _etext = .;
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

    /* ---- Config data (stored in NVM region) ---- */
    .config_data :
    {
        . = ALIGN(4);
        KEEP(*(.config_data))
        . = ALIGN(4);
    } > NVM

    /* ---- Initialization data (copied from FLASH to RAM at startup) ---- */
    .data : AT (_etext)
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM

    /* ---- BSS (zero-initialized) ---- */
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
    } > RAM

    /* ---- Heap ---- */
    .heap (NOLOAD) :
    {
        . = ALIGN(8);
        _sheap = .;
        . = . + HEAP_SIZE;
        . = ALIGN(8);
        _eheap = .;
    } > RAM

    /* ---- Stack ---- */
    .stack (NOLOAD) :
    {
        . = ALIGN(8);
        _sstack = .;
        . = . + STACK_SIZE;
        . = ALIGN(8);
        _estack = .;
    } > RAM

    /* ---- IPC shared memory (for BLE stack communication) ---- */
    .ipc :
    {
        . = ALIGN(4);
        KEEP(*(.ipc))
        . = ALIGN(4);
    } > NET_RAM

    /* ---- ML model weight section in external flash ---- */
    .ml_model_flash :
    {
        . = ALIGN(4);
        *(.ml_model_flash)
        . = ALIGN(4);
    } > EXT_FLASH

    /* ---- Discard unneeded sections ---- */
    /DISCARD/ :
    {
        *(.ARM.exidx*)
        *(.ARM.extab*)
        *(.comment)
        *(.note*)
    }
}

/* ---- Symbols for startup code ---- */
PROVIDE(_stack_top = ORIGIN(RAM) + LENGTH(RAM));
PROVIDE(_stack_bottom = _stack_top - STACK_SIZE);

/* ---- End of linker.ld ---- */