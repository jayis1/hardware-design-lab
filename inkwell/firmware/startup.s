/*
 * startup.s — ARM Cortex-M4F startup code for Inkwell (nRF52833)
 *
 * Initial vector table, reset handler, and zero-fill of .bss / copy
 * of .data. The first entry is the stack pointer, set by the linker
 * script's __stack_top symbol.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

    .syntax unified
    .cpu cortex-m4
    .thumb

    .extern Reset_Handler
    .extern __stack_top
    .weak   Default_Handler

    .section .isr_vector, "a", %progbits
    .align 2
    .type g_pfnVectors, %object
g_pfnVectors:
    .word __stack_top
    .word Reset_Handler
    .word NMI_Handler
    .word HardFault_Handler
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word SVC_Handler
    .word DebugMon_Handler
    .word 0
    .word PendSV_Handler
    .word SysTick_Handler
    /* IRQ 0-31 fillers */
    .rept 32
    .word Default_Handler
    .endr
    .size g_pfnVectors, . - g_pfnVectors

    .section .text.Reset_Handler, "ax", %progbits
    .align 2
    .global Reset_Handler
    .type Reset_Handler, %function
Reset_Handler:
    ldr   r0, =__data_lma
    ldr   r1, =__data_start
    ldr   r2, =__data_end
1:  cmp   r1, r2
    bcc   2f
    b     3f
2:  ldr   r3, [r0], #4
    str   r3, [r1], #4
    b     1b
3:
    ldr   r1, =__bss_start
    ldr   r2, =__bss_end
    movs  r3, #0
4:  cmp   r1, r2
    bcc   5f
    b     6f
5:  str   r3, [r1], #4
    b     4b
6:
    bl    main
    b     .

    .section .text.Default_Handler, "ax", %progbits
    .align 2
    .global Default_Handler
    .type Default_Handler, %function
Default_Handler:
    b     Default_Handler

    .weak NMI_Handler
    .weak HardFault_Handler
    .weak SVC_Handler
    .weak DebugMon_Handler
    .weak PendSV_Handler
    .weak SysTick_Handler
NMI_Handler:
HardFault_Handler:
SVC_Handler:
DebugMon_Handler:
PendSV_Handler:
SysTick_Handler:
    b     Default_Handler

    .end