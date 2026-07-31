/*
 * startup.s — Cortex-M33 vector table and reset handler for nRF5340 app core.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

    .syntax unified
    .cpu cortex-m33
    .thumb

    .section .isr_vector, "a", %progbits
    .align 2
    .global g_pfnVectors
g_pfnVectors:
    .word _estack_top            /* 0: Initial stack pointer */
    .word Reset_Handler          /* 1: Reset */
    .word NMI_Handler            /* 2: NMI */
    .word HardFault_Handler      /* 3: Hard fault */
    .word MemManage_Handler      /* 4: MemManage fault */
    .word BusFault_Handler       /* 5: Bus fault */
    .word UsageFault_Handler     /* 6: Usage fault */
    .word 0                      /* 7: Reserved */
    .word 0                      /* 8: Reserved */
    .word 0                      /* 9: Reserved */
    .word 0                      /* 10: Reserved */
    .word SVC_Handler            /* 11: SVCall */
    .word DebugMon_Handler       /* 12: Debug monitor */
    .word 0                      /* 13: Reserved */
    .word PendSV_Handler         /* 14: PendSV */
    .word SysTick_Handler        /* 15: SysTick */

    /* External IRQs (nRF5340 app core) */
    .word Default_Handler        /* 0: SPU */
    .word Default_Handler        /* 1: CLOCK */
    .word Default_Handler        /* 2: POWER */
    .word Default_Handler        /* 3: 2 */
    .word Default_Handler        /* 4: RADIO */
    .word Default_Handler        /* 5: RNG */
    .word Default_Handler        /* 6: ECB */
    .word Default_Handler        /* 7: CCM_AAR */
    .word TIMER0_IRQHandler      /* 8: TIMER0 */
    .word TIMER1_IRQHandler      /* 9: TIMER1 */
    .word TIMER2_IRQHandler      /* 10: TIMER2 */
    .word Default_Handler        /* 11: 3 */
    .word Default_Handler        /* 12: 4 */
    .word Default_Handler        /* 13: 5 */
    .word Default_Handler        /* 14: RTC0 */
    .word Default_Handler        /* 15: RTC1 */
    .word Default_Handler        /* 16: 6 */
    .word Default_Handler        /* 17: 7 */
    .word Default_Handler        /* 18: 8 */
    .word Default_Handler        /* 19: SAADC */
    .word Default_Handler        /* 20: 9 */
    .word SPIM0_IRQHandler       /* 21: SPIM0 */
    .word SPIM1_IRQHandler       /* 22: SPIM1 */
    .word Default_Handler        /* 23: SPIM2 */
    .word TWIM0_IRQHandler       /* 24: TWIM0 */
    .word Default_Handler        /* 25: TWIM1 */
    .word Default_Handler        /* 26: 10 */
    .word Default_Handler        /* 27: 11 */
    .word GPIOTE0_IRQHandler     /* 28: GPIOTE0 */
    .word Default_Handler        /* 29: 12 */
    .word Default_Handler        /* 30: 13 */
    .word Default_Handler        /* 31: 14 */
    .word Default_Handler        /* 32: 15 */
    .word Default_Handler        /* 33: 16 */
    .word Default_Handler        /* 34: 17 */
    .word Default_Handler        /* 35: 18 */
    .word Default_Handler        /* 36: 19 */
    .word Default_Handler        /* 37: 20 */
    .word Default_Handler        /* 38: 21 */
    .word Default_Handler        /* 39: 22 */
    .word Default_Handler        /* 40: 23 */
    .word Default_Handler        /* 41: 24 */
    .word IPC_IRQHandler         /* 42: IPC */
    .word Default_Handler        /* 43: 25 */
    .word Default_Handler        /* 44: 26 */
    .word Default_Handler        /* 45: 27 */
    .word Default_Handler        /* 46: 28 */
    .word Default_Handler        /* 47: 29 */
    .word Default_Handler        /* 48: 30 */
    .word Default_Handler        /* 49: 31 */
    .word Default_Handler        /* 50: 32 */
    .word Default_Handler        /* 51: 33 */
    .word Default_Handler        /* 52: 34 */
    .word Default_Handler        /* 53: 35 */
    .word Default_Handler        /* 54: 36 */
    .word Default_Handler        /* 55: 37 */
    .word USBD_IRQHandler        /* 56: USBD */

    .size g_pfnVectors, . - g_pfnVectors

/* -------------------------------------------------------------------------
 * Reset handler — copy .data, zero .bss, call main()
 * Author: jayis1
 * ------------------------------------------------------------------------- */
    .section .text.Reset_Handler, "ax", %progbits
    .align 2
    .global Reset_Handler
    .thumb_func
Reset_Handler:
    ldr     r0, =_sdata
    ldr     r1, =_edata
    ldr     r2, =_etext
    /* Copy .data from flash to RAM */
1:  cmp     r0, r1
    bcc     2f
    b       3f
2:  ldr     r3, [r2], #4
    str     r3, [r0], #4
    b       1b
    /* Zero .bss */
3:  ldr     r0, =_sbss
    ldr     r1, =_ebss
    movs    r2, #0
4:  cmp     r0, r1
    bcc     5f
    b       6f
5:  str     r2, [r0], #4
    b       4b
    /* Call main() */
6:  bl      main
    /* If main returns, loop forever */
7:  b       7b

/* -------------------------------------------------------------------------
 * Default handler for unconfigured IRQs
 * ------------------------------------------------------------------------- */
    .section .text.Default_Handler, "ax", %progbits
    .align 2
    .global Default_Handler
    .thumb_func
Default_Handler:
    b       Default_Handler

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
    .weak TIMER0_IRQHandler
    .thumb_set TIMER0_IRQHandler, Default_Handler
    .weak TIMER1_IRQHandler
    .thumb_set TIMER1_IRQHandler, Default_Handler
    .weak TIMER2_IRQHandler
    .thumb_set TIMER2_IRQHandler, Default_Handler
    .weak SPIM0_IRQHandler
    .thumb_set SPIM0_IRQHandler, Default_Handler
    .weak SPIM1_IRQHandler
    .thumb_set SPIM1_IRQHandler, Default_Handler
    .weak TWIM0_IRQHandler
    .thumb_set TWIM0_IRQHandler, Default_Handler
    .weak GPIOTE0_IRQHandler
    .thumb_set GPIOTE0_IRQHandler, Default_Handler
    .weak IPC_IRQHandler
    .thumb_set IPC_IRQHandler, Default_Handler
    .weak USBD_IRQHandler
    .thumb_set USBD_IRQHandler, Default_Handler

    .end