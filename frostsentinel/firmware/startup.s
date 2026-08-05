/*
 * startup.s — STM32U575 startup code and vector table
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

    .syntax unified
    .cpu cortex-m33
    .thumb

/* Stack pointer initialized by the linker */
    .word _estack

/* Vector table (at least the first 16 system entries + a few IRQs) */
    .section .isr_vector,"a",%progbits
    .align 2
    .global g_pfnVectors
g_pfnVectors:
    .word _estack              /* 0: Initial stack pointer */
    .word Reset_Handler        /* 1: Reset */
    .word NMI_Handler          /* 2: NMI */
    .word HardFault_Handler    /* 3: HardFault */
    .word 0                    /* 4: MemManage */
    .word 0                    /* 5: BusFault */
    .word 0                    /* 6: UsageFault */
    .word 0                    /* 7: Reserved */
    .word 0                    /* 8: Reserved */
    .word 0                    /* 9: Reserved */
    .word 0                    /* 10: Reserved */
    .word 0                    /* 11: SVC */
    .word 0                    /* 12: DebugMon */
    .word 0                    /* 13: Reserved */
    .word 0                    /* 14: Reserved */
    .word 0                    /* 15: SysTick */
    /* STM32U5 external interrupts (subset used by FrostSentinel) */
    .word 0                    /* 0: WWDG */
    .word 0                    /* 1: PVD */
    .word 0                    /* 2: RTC */
    .word 0                    /* 3: RTC wakeup */
    .word 0                    /* 4: FLASH */
    .word 0                    /* 5: RCC */
    .word 0                    /* 6: EXTI0 */
    .word 0                    /* 7: EXTI1 */
    .word 0                    /* 8: EXTI2 */
    .word 0                    /* 9: EXTI3 */
    .word 0                    /* 10: EXTI4 */
    .word DMA1_Channel1_IRQHandler /* 11: DMA1 CH1 */
    .word 0                    /* 12: DMA1 CH2 */
    .word 0                    /* 13: DMA1 CH3 */
    .word 0                    /* 14: DMA1 CH4 */
    .word 0                    /* 15: DMA1 CH5 */
    .word 0                    /* 16: DMA1 CH6 */
    .word 0                    /* 17: DMA1 CH7 */
    .word 0                    /* 18: ADC1 */
    .word 0                    /* 19: DAC1 */
    .word 0                    /* 20: FDCAN1 */
    .word 0                    /* 21: TIM1_BRK */
    .word 0                    /* 22: TIM1_UP */
    .word 0                    /* 23: TIM1_TRG_COM */
    .word 0                    /* 24: TIM1_CC */
    .word 0                    /* 25: TIM2 */
    .word 0                    /* 26: TIM3 */
    .word 0                    /* 27: TIM4 */
    .word 0                    /* 28: TIM5 */
    .word 0                    /* 29: TIM6 */
    .word 0                    /* 30: TIM7 */
    .word 0                    /* 31: TIM8 */
    .word 0                    /* 32: I2C1_EV */
    .word 0                    /* 33: I2C1_ER */
    .word 0                    /* 34: I2C2_EV */
    .word 0                    /* 35: I2C2_ER */
    .word 0                    /* 36: SPI1 */
    .word 0                    /* 37: SPI2 */
    .word USART2_IRQHandler    /* 38: USART2 (BLE) */
    .word USART3_IRQHandler    /* 39: USART3 (debug) */
    .word 0                    /* 40: EXTI9_5 */
    .word 0                    /* 41: TIM12 */
    .word RTC_IRQHandler       /* 42: RTC alarm (TAMP) */
    .word 0                    /* 43: reserved */
    .word 0                    /* 44: reserved */
    .word 0                    /* 45: I2C4_EV */
    .word 0                    /* 46: I2C4_ER */
    .word 0                    /* 47: TIM13 */
    .word 0                    /* 48: TIM14 */
    .word 0                    /* 49: TIM15 */
    .word 0                    /* 50: TIM16 */
    .word 0                    /* 51: TIM17 */
    .word 0                    /* 52: I2C3_EV */
    .word 0                    /* 53: I2C3_ER */
    .word TIM6_DAC_IRQHandler  /* 54: TIM6 + DAC1 (system tick) */
    .word TIM7_IRQHandler      /* 55: TIM7 */
    .word 0                    /* 56: reserved */
    .word 0                    /* 57: DMA2 CH1 */
    .word 0                    /* 58: DMA2 CH2 */
    .word 0                    /* 59: DMA2 CH3 */
    .word 0                    /* 60: DMA2 CH4 */
    .word 0                    /* 61: DMA2 CH5 */
    .word 0                    /* 62: DMA2 CH6 */
    .word 0                    /* 63: DMA2 CH7 */
    .word 0                    /* 64-77: more reserved/misc IRQs */
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word AES1_IRQHandler      /* 78: AES1 */
    .word 0                    /* 79-89: reserved */
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word LPUART1_IRQHandler   /* 91: LPUART1 */

/* ------------------------------------------------------------------ */
/*  Reset handler                                                      */
/* ------------------------------------------------------------------ */
    .section .text.Reset_Handler
    .weak Reset_Handler
    .type Reset_Handler, %function
Reset_Handler:
    /* Set stack pointer from linker symbol */
    ldr r0, =_estack
    msr psp, r0
    mov sp, r0

    /* Copy .data from flash to RAM */
    ldr r0, =_sidata
    ldr r1, =_sdata
    ldr r2, =_edata
1:
    cmp r1, r2
    bcc 2f
    b 3f
2:
    ldr r3, [r0], #4
    str r3, [r1], #4
    b 1b
3:
    /* Zero .bss */
    ldr r0, =_sbss
    ldr r1, =_ebss
    movs r2, #0
4:
    cmp r0, r1
    bcc 5f
    b 6f
5:
    str r2, [r0], #4
    b 4b
6:
    /* Call main() */
    bl main
    /* If main returns, loop forever */
7:
    b 7b

    .size Reset_Handler, . - Reset_Handler

/* ------------------------------------------------------------------ */
/*  Default IRQ handler                                                */
/* ------------------------------------------------------------------ */
    .section .text.Default_IRQHandler
    .weak Default_IRQHandler
    .type Default_IRQHandler, %function
Default_IRQHandler:
    b Default_IRQHandler

/* Weak aliases for IRQ handlers not defined in C */
    .weak DMA1_Channel1_IRQHandler
    .thumb_set DMA1_Channel1_IRQHandler, Default_IRQHandler
    .weak USART2_IRQHandler
    .thumb_set USART2_IRQHandler, Default_IRQHandler
    .weak USART3_IRQHandler
    .thumb_set USART3_IRQHandler, Default_IRQHandler
    .weak RTC_IRQHandler
    .thumb_set RTC_IRQHandler, Default_IRQHandler
    .weak TIM7_IRQHandler
    .thumb_set TIM7_IRQHandler, Default_IRQHandler
    .weak AES1_IRQHandler
    .thumb_set AES1_IRQHandler, Default_IRQHandler
    .weak LPUART1_IRQHandler
    .thumb_set LPUART1_IRQHandler, Default_IRQHandler

    .end