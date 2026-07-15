/*
 * startup_nrf5340.s — Startup assembly for nRF5340 (app core)
 *
 * Initializes stack, copies .data from flash to RAM, zeros .bss,
 * and calls main().
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * License: GPL-3.0
 */

    .syntax unified
    .cpu cortex-m33
    .thumb

/* Stack — placed at top of RAM */
    .section .stack
    .align 3
    .global __stack_top
__stack_top:
    .space 4096

/* Vector table */
    .section .isr_vector,"a",%progbits
    .align 2
    .global __vector_table
__vector_table:
    .word __stack_top          /* 0: Initial Stack Pointer */
    .word Reset_Handler        /* 1: Reset Handler */
    .word NMI_Handler          /* 2: NMI Handler */
    .word HardFault_Handler    /* 3: HardFault Handler */
    .word MemManage_Handler    /* 4: MemManage Handler */
    .word BusFault_Handler     /* 5: BusFault Handler */
    .word UsageFault_Handler   /* 6: UsageFault Handler */
    .word 0                    /* 7: Reserved */
    .word 0                    /* 8: Reserved */
    .word 0                    /* 9: Reserved */
    .word 0                    /* 10: Reserved */
    .word SVC_Handler          /* 11: SVCall Handler */
    .word DebugMon_Handler     /* 12: Debug Monitor Handler */
    .word 0                    /* 13: Reserved */
    .word PendSV_Handler       /* 14: PendSV Handler */
    .word SysTick_Handler      /* 15: SysTick Handler */

    /* External interrupts 16–31 (nRF5340-specific, simplified) */
    .rept 16
    .word Default_Handler
    .endr

/* Reset handler */
    .section .text.Reset_Handler
    .weak Reset_Handler
    .type Reset_Handler, %function
Reset_Handler:
    /* Set stack pointer */
    ldr r0, =__stack_top
    msr psp, r0
    mov sp, r0

    /* Copy .data from flash to RAM */
    ldr r0, =__data_load_start
    ldr r1, =__data_start
    ldr r2, =__data_end
1:
    cmp r1, r2
    bcs 2f
    ldr r3, [r0], #4
    str r3, [r1], #4
    b 1b
2:

    /* Zero .bss */
    ldr r0, =__bss_start
    ldr r1, =__bss_end
    movs r2, #0
3:
    cmp r0, r1
    bcs 4f
    str r2, [r0], #4
    b 3b
4:

    /* Call main() */
    bl main

    /* If main returns, loop forever */
5:
    b 5b

/* Default handler */
    .section .text.Default_Handler
    .weak Default_Handler
    .type Default_Handler, %function
Default_Handler:
    b Default_Handler

/* Weak aliases for all handlers */
    .weak NMI_Handler
    .thumb_set NMI_Handler, Default_Handler
    .weak HardFault_Handler
    .thumb_set HardFault_Handler, Default_Handler
    .weak MemManage_Handler
    .thumb_set MemManage_Handler, Default_Handler
    .weak BusFault_Handler
    .thumb_set BusFault_Handler, Default_Handler
    .weak UsageFault_Handler
    .thumb_set UsageFault_Handler, Default_Handler
    .weak SVC_Handler
    .thumb_set SVC_Handler, Default_Handler
    .weak DebugMon_Handler
    .thumb_set DebugMon_Handler, Default_Handler
    .weak PendSV_Handler
    .thumb_set PendSV_Handler, Default_Handler
    .weak SysTick_Handler
    .thumb_set SysTick_Handler, Default_Handler

    .end