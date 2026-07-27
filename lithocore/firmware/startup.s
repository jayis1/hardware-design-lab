/*
 * startup.s — Reset handler and vector table for STM32G474RET6.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

    .syntax unified
    .cpu cortex-m4
    .thumb
    .fpu fpv4-sp-d16

/* Stack pointer — initialized by the first vector table entry */
    .section .isr_vector,"a",%progbits
    .align 2
    .global g_pfnVectors
    .type g_pfnVectors, %object
g_pfnVectors:
    .word _estack               /* Initial stack pointer */
    .word Reset_Handler         /* Reset handler */
    .word NMI_Handler           /* NMI */
    .word HardFault_Handler     /* Hard fault */
    .word MemManage_Handler     /* Memory management fault */
    .word BusFault_Handler      /* Bus fault */
    .word UsageFault_Handler    /* Usage fault */
    .word 0                     /* Reserved */
    .word 0                     /* Reserved */
    .word 0                     /* Reserved */
    .word 0                     /* Reserved */
    .word SVC_Handler           /* SVCall */
    .word DebugMon_Handler      /* Debug monitor */
    .word 0                     /* Reserved */
    .word PendSV_Handler        /* PendSV */
    .word SysTick_Handler       /* SysTick */

    /* External interrupts (STM32G4 — up to IRQ 102, we define the key ones) */
    .word WWDG_IRQHandler       /* 0  Window watchdog */
    .word PVD_PVM_IRQHandler    /* 1  PVD/PVM */
    .word RTC_TAMP_LSECSS_IRQHandler /* 2  RTC */
    .word RTC_WKUP_IRQHandler   /* 3  RTC wake */
    .word FLASH_IRQHandler      /* 4  Flash */
    .word RCC_IRQHandler        /* 5  RCC */
    .word EXTI0_IRQHandler      /* 6  EXTI0 */
    .word EXTI1_IRQHandler      /* 7  EXTI1 */
    .word EXTI2_IRQHandler      /* 8  EXTI2 */
    .word EXTI3_IRQHandler      /* 9  EXTI3 */
    .word EXTI4_IRQHandler      /* 10 EXTI4 */
    .word DMA1_Channel1_IRQHandler /* 11 DMA1 Ch1 */
    .word DMA1_Channel2_IRQHandler /* 12 */
    .word DMA1_Channel3_IRQHandler /* 13 */
    .word DMA1_Channel4_IRQHandler /* 14 */
    .word DMA1_Channel5_IRQHandler /* 15 */
    .word DMA1_Channel6_IRQHandler /* 16 */
    .word DMA1_Channel7_IRQHandler /* 17 */
    .word ADC1_2_IRQHandler     /* 18 ADC1/2 */
    .word USB_HP_IRQHandler     /* 19 USB high priority */
    .word CAN1_TX_IRQHandler    /* 20 */
    .word CAN1_RX0_IRQHandler   /* 21 */
    .word CAN1_RX1_IRQHandler   /* 22 */
    .word CAN1_SCE_IRQHandler   /* 23 */
    .word TIM1_UP_IRQHandler    /* 24 TIM1 update */
    .word TIM1_TRG_COM_IRQHandler /* 25 */
    .word TIM1_CC_IRQHandler    /* 26 */
    .word TIM2_IRQHandler       /* 27 */
    .word TIM3_IRQHandler       /* 28 */
    .word TIM4_IRQHandler       /* 29 */
    .word I2C1_EV_IRQHandler    /* 30 */
    .word I2C1_ER_IRQHandler    /* 31 */
    .word I2C2_EV_IRQHandler    /* 32 */
    .word I2C3_EV_IRQHandler    /* 33 */
    .word I2C2_ER_IRQHandler    /* 34 */
    .word SPI1_IRQHandler       /* 35 SPI1 */
    .word SPI2_IRQHandler       /* 36 */
    .word USART1_IRQHandler     /* 37 USART1 — actually IRQ 53 on G4, but
                                            we alias the common ones */
    .word USART2_IRQHandler     /* 38 */
    .word USART3_IRQHandler     /* 39 */
    .word EXTI9_5_IRQHandler    /* 40 EXTI 9-5 */
    .word TIM1_BRK_IRQHandler   /* 41 */
    .word TIM15_IRQHandler      /* 42 */
    .word TIM16_IRQHandler      /* 43 */
    .word TIM17_IRQHandler      /* 44 */
    .word TIM6_DAC_IRQHandler   /* 45 */
    .word TIM7_IRQHandler       /* 46 */
    .word TIM8_UP_IRQHandler    /* 47 */
    .word 0                     /* 48 */
    .word TIM8_CC_IRQHandler    /* 49 */
    .word 0                     /* 50 */
    .word SPI3_IRQHandler       /* 51 SPI3 */
    .word UART4_IRQHandler      /* 52 */
    .word USART1_IRQHandler     /* 53 USART1 (correct position) */
    .word 0                     /* 54 */
    .word 0                     /* 55 */
    .word 0                     /* 56 */
    .word 0                     /* 57 */
    .word 0                     /* 58 */
    .word 0                     /* 59 */
    .word 0                     /* 60 */
    .word 0                     /* 61 */
    .word 0                     /* 62 */
    .word 0                     /* 63 */
    .word 0                     /* 64 */
    .word 0                     /* 65 */
    .word 0                     /* 66 */
    .word 0                     /* 67 */
    .word 0                     /* 68 */
    .word 0                     /* 69 */
    .word 0                     /* 70 */
    .word 0                     /* 71 */
    .word 0                     /* 72 */
    .word 0                     /* 73 */
    .word USB_HP_IRQHandler     /* 74 USB high priority (G4) */
    .word USB_LP_IRQHandler     /* 75 USB low priority */
    .rept 30                    /* remaining IRQs up to 102 */
    .word 0
    .endr

    .size g_pfnVectors, . - g_pfnVectors

/* -------------------------------------------------------------------------
 * Reset handler — copies .data from flash to RAM, zeros .bss, calls main
 * ------------------------------------------------------------------------- */
    .section .text.Reset_Handler, "ax", %progbits
    .align 2
    .global Reset_Handler
    .type Reset_Handler, %function
Reset_Handler:
    /* Set stack pointer */
    ldr sp, =_estack

    /* Copy .data from flash to RAM */
    ldr r0, =_sidata       /* source: .data load address in flash */
    ldr r1, =_sdata        /* dest: .data start in RAM */
    ldr r2, =_edata        /* end: .data end in RAM */
1:
    cmp r1, r2
    bcs 2f                 /* if dest >= end, done */
    ldr r3, [r0], #4       /* load word from flash, increment source */
    str r3, [r1], #4       /* store to RAM, increment dest */
    b 1b
2:

    /* Zero .bss section */
    ldr r0, =_sbss         /* start of BSS */
    ldr r1, =_ebss         /* end of BSS */
    movs r2, #0            /* zero value */
3:
    cmp r0, r1
    bcs 4f                 /* if start >= end, done */
    str r2, [r0], #4       /* store zero, increment */
    b 3b
4:

    /* Call SystemInit (if defined) — we skip it and let main.c do clock init */
    /* bl SystemInit */

    /* Call main() */
    bl main

    /* If main returns, loop forever */
5:
    b 5b

    .size Reset_Handler, . - Reset_Handler

/* -------------------------------------------------------------------------
 * Default interrupt handler — loops forever
 * ------------------------------------------------------------------------- */
    .section .text.Default_Handler, "ax", %progbits
    .align 2
    .global Default_Handler
    .type Default_Handler, %function
Default_Handler:
    b Default_Handler
    .size Default_Handler, . - Default_Handler

/* -------------------------------------------------------------------------
 * Weak aliases for all interrupt handlers — default to Default_Handler
 * unless overridden in C
 * ------------------------------------------------------------------------- */
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

    /* External IRQ handlers */
    .weak WWDG_IRQHandler
    .thumb_set WWDG_IRQHandler, Default_Handler
    .weak PVD_PVM_IRQHandler
    .thumb_set PVD_PVM_IRQHandler, Default_Handler
    .weak RTC_TAMP_LSECSS_IRQHandler
    .thumb_set RTC_TAMP_LSECSS_IRQHandler, Default_Handler
    .weak RTC_WKUP_IRQHandler
    .thumb_set RTC_WKUP_IRQHandler, Default_Handler
    .weak FLASH_IRQHandler
    .thumb_set FLASH_IRQHandler, Default_Handler
    .weak RCC_IRQHandler
    .thumb_set RCC_IRQHandler, Default_Handler
    .weak EXTI0_IRQHandler
    .thumb_set EXTI0_IRQHandler, Default_Handler
    .weak EXTI1_IRQHandler
    .thumb_set EXTI1_IRQHandler, Default_Handler
    .weak EXTI2_IRQHandler
    .thumb_set EXTI2_IRQHandler, Default_Handler
    .weak EXTI3_IRQHandler
    .thumb_set EXTI3_IRQHandler, Default_Handler
    .weak EXTI4_IRQHandler
    .thumb_set EXTI4_IRQHandler, Default_Handler
    .weak DMA1_Channel1_IRQHandler
    .thumb_set DMA1_Channel1_IRQHandler, Default_Handler
    .weak DMA1_Channel2_IRQHandler
    .thumb_set DMA1_Channel2_IRQHandler, Default_Handler
    .weak DMA1_Channel3_IRQHandler
    .thumb_set DMA1_Channel3_IRQHandler, Default_Handler
    .weak DMA1_Channel4_IRQHandler
    .thumb_set DMA1_Channel4_IRQHandler, Default_Handler
    .weak DMA1_Channel5_IRQHandler
    .thumb_set DMA1_Channel5_IRQHandler, Default_Handler
    .weak DMA1_Channel6_IRQHandler
    .thumb_set DMA1_Channel6_IRQHandler, Default_Handler
    .weak DMA1_Channel7_IRQHandler
    .thumb_set DMA1_Channel7_IRQHandler, Default_Handler
    .weak ADC1_2_IRQHandler
    .thumb_set ADC1_2_IRQHandler, Default_Handler
    .weak USB_HP_IRQHandler
    .thumb_set USB_HP_IRQHandler, Default_Handler
    .weak USB_LP_IRQHandler
    .thumb_set USB_LP_IRQHandler, Default_Handler
    .weak TIM1_UP_IRQHandler
    .thumb_set TIM1_UP_IRQHandler, Default_Handler
    .weak SPI1_IRQHandler
    .thumb_set SPI1_IRQHandler, Default_Handler
    .weak SPI3_IRQHandler
    .thumb_set SPI3_IRQHandler, Default_Handler
    .weak USART1_IRQHandler
    .thumb_set USART1_IRQHandler, Default_Handler
    .weak USART3_IRQHandler
    .thumb_set USART3_IRQHandler, Default_Handler
    .weak EXTI9_5_IRQHandler
    .thumb_set EXTI9_5_IRQHandler, Default_Handler
    .weak TIM6_DAC_IRQHandler
    .thumb_set TIM6_DAC_IRQHandler, Default_Handler
    .weak TIM7_IRQHandler
    .thumb_set TIM7_IRQHandler, Default_Handler
    .weak TIM8_UP_IRQHandler
    .thumb_set TIM8_UP_IRQHandler, Default_Handler
    .weak TIM8_CC_IRQHandler
    .thumb_set TIM8_CC_IRQHandler, Default_Handler

/* End of startup.s — Author: jayis1 */