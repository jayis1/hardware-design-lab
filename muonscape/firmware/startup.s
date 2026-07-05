/*
 * startup.s — STM32H723 vector table + startup code
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: MIT
 *
 * Minimal Cortex-M7 startup: copy .data from flash to RAM, zero .bss,
 * set up the stack, and call main(). The vector table is placed at the
 * start of flash so the boot ROM picks it up. Default handlers spin.
 */
    .syntax unified
    .cpu cortex-m7
    .thumb

.section .isr_vector, "a", %progbits
    .word _estack
    .word Reset_Handler
    .word NMI_Handler
    .word HardFault_Handler
    .word MemManage_Handler
    .word BusFault_Handler
    .word UsageFault_Handler
    .word 0
    .word 0
    .word 0
    .word 0
    .word SVC_Handler
    .word DebugMon_Handler
    .word 0
    .word PendSV_Handler
    .word SysTick_Handler

    /* External interrupt vectors (subset) */
    .word 0   /* WWDG */
    .word 0   /* PVD */
    .word 0   /* TAMP */
    .word 0   /* RTC */
    .word 0   /* FLASH */
    .word 0   /* RCC */
    .word 0   /* EXTI0 */
    .word 0   /* EXTI1 */
    .word 0   /* EXTI2 */
    .word 0   /* EXTI3 */
    .word EXTI4_IRQHandler       /* EXTI4 — GPS PPS */
    .word 0   /* DMA1 stream0 */
    .word 0   /* DMA1 stream1 */
    .word 0   /* DMA1 stream2 */
    .word 0   /* DMA1 stream3 */
    .word 0   /* DMA1 stream4 */
    .word 0   /* DMA1 stream5 */
    .word 0   /* DMA1 stream6 */
    .word 0   /* ADC1/2 */
    .word 0   /* FDCAN1_IT0 */
    .word 0   /* FDCAN2_IT0 */
    .word 0   /* FDCAN1_IT1 */
    .word 0   /* FDCAN2_IT1 */
    .word 0   /* EXTI9_5 — replaced at runtime? No, vector 23: */
    .word EXTI9_5_IRQHandler
    .word 0   /* TIM1 BRK */
    .word 0   /* TIM1 UP */
    .word 0   /* TIM1 TRG/COM */
    .word 0   /* TIM1 CC */
    .word 0   /* TIM2 */
    .word 0   /* TIM3 */
    .word 0   /* TIM4 */
    .word 0   /* I2C1 EV */
    .word 0   /* I2C1 ER */
    .word 0   /* I2C2 EV */
    .word 0   /* I2C2 ER */
    .word 0   /* SPI1 */
    .word 0   /* SPI2 */
    .word 0   /* USART1 */
    .word USART2_IRQHandler    /* USART2 — GPS NMEA */
    .word 0   /* USART3 */
    .word 0   /* EXTI15_10 */
    .word 0   /* RTC Alarm */
    /* ... remaining vectors up to ~144 omitted for brevity */

.section .text
Reset_Handler:
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
    ldr r0, =_sbss
    ldr r1, =_ebss
    movs r3, #0
4:
    cmp r0, r1
    bcc 5f
    b 6f
5:
    str r3, [r0], #4
    b 4b
6:
    bl main
    b 6b

.section .text.Default_Handler, "ax", %progbits
Default_Handler:
    b Default_Handler

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
    .weak EXTI9_5_IRQHandler
    .thumb_set EXTI9_5_IRQHandler, Default_Handler
    .weak USART2_IRQHandler
    .thumb_set USART2_IRQHandler, Default_Handler
    .weak EXTI4_IRQHandler
    .thumb_set EXTI4_IRQHandler, Default_Handler

.section .bss
    .align 3
_sbss:
    .space 0x20000
_ebss:

.section .data
    .align 3
_sidata:
_sdata:
    .word 0
_edata:

.section .stack
    .align 3
    .space 0x4000
_estack: