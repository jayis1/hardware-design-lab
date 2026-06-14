/*
 * SkyPilot — Linker Script for STM32H743VIT6
 * Flash: 2MB at 0x08000000
 * ITCM:  64KB at 0x00000000 (flight loop code)
 * DTCM:  128KB at 0x20000000 (sensor data, PID state)
 * AXI:   512KB at 0x24000000 (DMA buffers, telemetry)
 * SRAM1: 128KB at 0x30000000 (RTOS heap)
 * SRAM2: 128KB at 0x30020000 (companion IPC)
 * SRAM4: 64KB at 0x38000000 (backup)
 */

MEMORY
{
    ITCM    (rx)  : ORIGIN = 0x00000000, LENGTH = 64K
    FLASH   (rx)  : ORIGIN = 0x08000000, LENGTH = 2048K
    DTCM    (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
    AXI     (rwx) : ORIGIN = 0x24000000, LENGTH = 512K
    SRAM1   (rwx) : ORIGIN = 0x30000000, LENGTH = 128K
    SRAM2   (rwx) : ORIGIN = 0x30020000, LENGTH = 128K
    SRAM4   (rwx) : ORIGIN = 0x38000000, LENGTH = 64K
}

ENTRY(Reset_Handler)

SECTIONS
{
    /* Vector table at start of Flash */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    /* Flight-critical code in ITCM for zero-wait execution */
    .itcm_text :
    {
        . = ALIGN(4);
        *(.itcm_code)
        *(.itcm_code.*)
        . = ALIGN(4);
    } > ITCM AT > FLASH

    /* Main code in Flash */
    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.glue_7)
        *(.glue_7t)
        KEEP(*(.init))
        KEEP(*(.fini))
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    /* Read-only data */
    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } > FLASH

    /* Provide symbol for .data init */
    _sidata = LOADADDR(.data);

    /* Initialized data — DTCM for fast access */
    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > DTCM AT > FLASH

    /* DMA buffers — AXI SRAM */
    .dma_buffers (NOLOAD) :
    {
        . = ALIGN(32);
        _sdma_buffers = .;
        *(.dma_buffers)
        *(.dma_buffers.*)
        . = ALIGN(32);
        _edma_buffers = .;
    } > AXI

    /* BSS — DTCM */
    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > DTCM

    /* User heap — SRAM1 */
    .user_heap (NOLOAD) :
    {
        . = ALIGN(4);
        _sheap = .;
        . = . + 64K;
        . = ALIGN(4);
        _eheap = .;
    } > SRAM1

    /* FreeRTOS heap — SRAM1 */
    .freertos_heap (NOLOAD) :
    {
        . = ALIGN(4);
        _srtos_heap = .;
        . = . + 64K;
        . = ALIGN(4);
        _ertos_heap = .;
    } > SRAM1

    /* Companion IPC — SRAM2 */
    .companion_ipc (NOLOAD) :
    {
        . = ALIGN(4);
        _sipc = .;
        . = . + 4K;
        . = ALIGN(4);
        _eipc = .;
    } > SRAM2

    /* Stack in DTCM */
    _estack = ORIGIN(DTCM) + LENGTH(DTCM);

    /* Discard unwanted sections */
    /DISCARD/ :
    {
        *(.ARM.exidx*)
        *(.ARM.attributes*)
    }
}