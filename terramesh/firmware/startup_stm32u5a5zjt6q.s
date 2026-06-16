/**
 * @file    startup_stm32u5a5zjt6q.s
 * @brief   Terramesh — STM32U5A5ZJT6Q startup code
 * @author  jayis1
 * @copyright Copyright © 2026 jayis1
 * @license GPL-2.0
 *
 * Reset handler, vector table, and low-level initialization for the
 * Cortex-M33 processor. Sets up stack, BSS, data sections, and
 * calls main().
 */

.syntax unified
.cpu cortex-m33
.fpu fpv5-sp-d16
.thumb

/* Stack top from linker */
.word   __stack_end

/* Vector table */
.section .isr_vector, "a", %progbits
.type   g_pfnVectors, %object
.size   g_pfnVectors, . - g_pfnVectors

g_pfnVectors:
    .word   __stack_end                    /* 0: Initial SP value */
    .word   Reset_Handler                  /* 1: Reset */
    .word   NMI_Handler                    /* 2: NMI */
    .word   HardFault_Handler              /* 3: Hard fault */
    .word   MemManage_Handler              /* 4: MemManage */
    .word   BusFault_Handler               /* 5: Bus fault */
    .word   UsageFault_Handler             /* 6: Usage fault */
    .word   SecureFault_Handler            /* 7: Secure fault */
    .word   0                              /* 8: Reserved */
    .word   0                              /* 9: Reserved */
    .word   0                              /* 10: Reserved */
    .word   SVC_Handler                    /* 11: SVCall */
    .word   DebugMon_Handler               /* 12: Debug monitor */
    .word   0                              /* 13: Reserved */
    .word   PendSV_Handler                 /* 14: PendSV */
    .word   SysTick_Handler                /* 15: SysTick */

    /* External interrupts (STM32U5A5) */
    .word   WWDG_IRQHandler               /* 0: WWDG */
    .word   PVD_PVM_IRQHandler            /* 1: PVD/PVM */
    .word   RTC_TAMP_IRQHandler           /* 2: RTC TAMP */
    .word   RTC_WKUP_IRQHandler           /* 3: RTC WKUP */
    .word   FLASH_ITF_IRQHandler          /* 4: Flash ITF */
    .word   RCC_IRQHandler                /* 5: RCC */
    .word   EXTI0_IRQHandler              /* 6: EXTI0 */
    .word   EXTI1_IRQHandler              /* 7: EXTI1 */
    .word   EXTI2_IRQHandler              /* 8: EXTI2 */
    .word   EXTI3_IRQHandler              /* 9: EXTI3 */
    .word   EXTI4_IRQHandler              /* 10: EXTI4 */
    .word   DMA1_CH1_IRQHandler           /* 11: DMA1 CH1 */
    .word   DMA1_CH2_IRQHandler           /* 12: DMA1 CH2 */
    .word   DMA1_CH3_IRQHandler           /* 13: DMA1 CH3 */
    .word   DMA1_CH4_IRQHandler           /* 14: DMA1 CH4 */
    .word   DMA1_CH5_IRQHandler           /* 15: DMA1 CH5 */
    .word   DMA1_CH6_IRQHandler           /* 16: DMA1 CH6 */
    .word   DMA1_CH7_IRQHandler           /* 17: DMA1 CH7 */
    .word   ADC1_IRQHandler               /* 18: ADC1 */
    .word   TIM1_BRK_IRQHandler           /* 19: TIM1 BRK */
    .word   TIM1_UP_IRQHandler            /* 20: TIM1 UP */
    .word   TIM1_TRG_COM_IRQHandler       /* 21: TIM1 TRG/COM */
    .word   TIM1_CC_IRQHandler            /* 22: TIM1 CC */
    .word   TIM2_IRQHandler               /* 23: TIM2 */
    .word   TIM3_IRQHandler               /* 24: TIM3 */
    .word   TIM4_IRQHandler               /* 25: TIM4 */
    .word   0                              /* 26: Reserved */
    .word   I2C1_EV_IRQHandler            /* 27: I2C1 EV */
    .word   I2C1_ER_IRQHandler            /* 28: I2C1 ER */
    .word   I2C2_EV_IRQHandler            /* 29: I2C2 EV */
    .word   I2C2_ER_IRQHandler            /* 30: I2C2 ER */
    .word   SPI1_IRQHandler               /* 31: SPI1 */
    .word   SPI2_IRQHandler               /* 32: SPI2 */
    .word   USART1_IRQHandler             /* 33: USART1 */
    .word   USART2_IRQHandler             /* 34: USART2 */
    .word   LPUART1_IRQHandler            /* 35: LPUART1 */
    .word   EXTI9_5_IRQHandler            /* 36: EXTI9-5 */
    .word   TIM15_IRQHandler              /* 37: TIM15 */
    .word   TIM16_IRQHandler              /* 38: TIM16 */
    .word   TIM17_IRQHandler              /* 39: TIM17 */
    .word   DMA2_CH1_IRQHandler           /* 40: DMA2 CH1 */
    .word   DMA2_CH2_IRQHandler           /* 41: DMA2 CH2 */
    .word   DMA2_CH3_IRQHandler           /* 42: DMA2 CH3 */
    .word   DMA2_CH4_IRQHandler           /* 43: DMA2 CH4 */
    .word   DMA2_CH5_IRQHandler           /* 44: DMA2 CH5 */
    .word   DMA2_CH6_IRQHandler           /* 45: DMA2 CH6 */
    .word   DMA2_CH7_IRQHandler           /* 46: DMA2 CH7 */
    .word   EXTI15_10_IRQHandler          /* 47: EXTI15-10 */
    .word   RTC_ALARM_IRQHandler          /* 48: RTC ALARM */
    .word   LPTIM1_IRQHandler             /* 49: LPTIM1 */
    .word   LPTIM2_IRQHandler             /* 50: LPTIM2 */

/* Weak default handlers */
.macro def_irq name
.thumb_func
.weak   \name
.type   \name, %function
\name:
    b       .
.endm

def_irq  NMI_Handler
def_irq  HardFault_Handler
def_irq  MemManage_Handler
def_irq  BusFault_Handler
def_irq  UsageFault_Handler
def_irq  SecureFault_Handler
def_irq  SVC_Handler
def_irq  DebugMon_Handler
def_irq  PendSV_Handler
def_irq  SysTick_Handler
def_irq  WWDG_IRQHandler
def_irq  PVD_PVM_IRQHandler
def_irq  RTC_TAMP_IRQHandler
def_irq  RTC_WKUP_IRQHandler
def_irq  FLASH_ITF_IRQHandler
def_irq  RCC_IRQHandler
def_irq  EXTI0_IRQHandler
def_irq  EXTI1_IRQHandler
def_irq  EXTI2_IRQHandler
def_irq  EXTI3_IRQHandler
def_irq  EXTI4_IRQHandler
def_irq  DMA1_CH1_IRQHandler
def_irq  DMA1_CH2_IRQHandler
def_irq  DMA1_CH3_IRQHandler
def_irq  DMA1_CH4_IRQHandler
def_irq  DMA1_CH5_IRQHandler
def_irq  DMA1_CH6_IRQHandler
def_irq  DMA1_CH7_IRQHandler
def_irq  ADC1_IRQHandler
def_irq  TIM1_BRK_IRQHandler
def_irq  TIM1_UP_IRQHandler
def_irq  TIM1_TRG_COM_IRQHandler
def_irq  TIM1_CC_IRQHandler
def_irq  TIM2_IRQHandler
def_irq  TIM3_IRQHandler
def_irq  TIM4_IRQHandler
def_irq  I2C1_EV_IRQHandler
def_irq  I2C1_ER_IRQHandler
def_irq  I2C2_EV_IRQHandler
def_irq  I2C2_ER_IRQHandler
def_irq  SPI1_IRQHandler
def_irq  SPI2_IRQHandler
def_irq  USART1_IRQHandler
def_irq  USART2_IRQHandler
def_irq  LPUART1_IRQHandler
def_irq  EXTI9_5_IRQHandler
def_irq  TIM15_IRQHandler
def_irq  TIM16_IRQHandler
def_irq  TIM17_IRQHandler
def_irq  DMA2_CH1_IRQHandler
def_irq  DMA2_CH2_IRQHandler
def_irq  DMA2_CH3_IRQHandler
def_irq  DMA2_CH4_IRQHandler
def_irq  DMA2_CH5_IRQHandler
def_irq  DMA2_CH6_IRQHandler
def_irq  DMA2_CH7_IRQHandler
def_irq  EXTI15_10_IRQHandler
def_irq  RTC_ALARM_IRQHandler
def_irq  LPTIM1_IRQHandler
def_irq  LPTIM2_IRQHandler

/* Reset handler */
.thumb_func
.type   Reset_Handler, %function
.global Reset_Handler

Reset_Handler:
    /* Set stack pointer */
    ldr     r0, =__stack_end
    msr     msp, r0

    /* Clear BSS section */
    ldr     r0, =__bss_start
    ldr     r1, =__bss_end
    movs    r2, #0
    b       .L_bss_check

.L_bss_loop:
    str     r2, [r0]
    adds    r0, r0, #4

.L_bss_check:
    cmp     r0, r1
    blt     .L_bss_loop

    /* Copy data section from flash to SRAM */
    ldr     r0, =__data_load
    ldr     r1, =__data_start
    ldr     r2, =__data_end
    b       .L_data_check

.L_data_loop:
    ldr     r3, [r0]
    str     r3, [r1]
    adds    r0, r0, #4
    adds    r1, r1, #4

.L_data_check:
    cmp     r1, r2
    blt     .L_data_loop

    /* Call constructors (if any) */
    bl      __libc_init_array

    /* Call main */
    bl      main

    /* Infinite loop if main returns */
    b       .
