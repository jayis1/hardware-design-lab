/*
 * startup.c — minimal Cortex-M7 startup + vector table for STM32H743.
 *
 * Author : jayis1
 * License: MIT
 *
 * Provides the reset handler, the default vector table, and weak aliases
 * for the ISRs we use (SysTick, BDMA ch0, USART3). The rest of the vector
 * table falls through to Default_Handler.
 */
#include "registers.h"

extern int  __stack_top;          /* provided by linker script */
extern int main(void);

void Reset_Handler(void);
void Default_Handler(void);
void SysTick_Handler(void);
void bdma_ch0_irq(void);
void USART3_irqhandler(void);

/* Weak aliases so the user can override. */
__attribute__((weak)) void SysTick_Handler(void)        {}
__attribute__((weak)) void bdma_ch0_irq(void)           {}
__attribute__((weak)) void USART3_irqhandler(void)      {}

/* ---- Vector table (first 16 + peripheral IRQs) ------------------ */
__attribute__((section(".isr_vector"), used))
void (* const g_pfnVectors[])(void) = {
    (void (*)(void))&__stack_top,   /* Initial SP */
    Reset_Handler,
    Default_Handler,                /* NMI */
    Default_Handler,                /* HardFault */
    Default_Handler,                /* MemManage */
    Default_Handler,                /* BusFault */
    Default_Handler,                /* UsageFault */
    0, 0, 0, 0,                     /* Reserved */
    Default_Handler,                /* SVCall */
    Default_Handler,                /* DebugMon */
    0,                              /* Reserved */
    Default_Handler,                /* PendSV */
    SysTick_Handler,                /* SysTick */
    /* External IRQs (0..127 for H743; only those we use are named). */
    bdma_ch0_irq,                   /* 0  BDMA ch0 */
    Default_Handler,                /* 1  */
    Default_Handler,                /* 2  */
    Default_Handler,                /* 3  */
    Default_Handler,                /* 4  */
    Default_Handler,                /* 5  */
    Default_Handler,                /* 6  */
    Default_Handler,                /* 7  */
    Default_Handler,                /* 8  */
    Default_Handler,                /* 9  */
    Default_Handler,                /* 10 */
    Default_Handler,                /* 11 */
    Default_Handler,                /* 12 */
    Default_Handler,                /* 13 */
    Default_Handler,                /* 14 */
    Default_Handler,                /* 15 */
    Default_Handler,                /* 16 */
    Default_Handler,                /* 17 */
    Default_Handler,                /* 18 */
    Default_Handler,                /* 19 */
    Default_Handler,                /* 20 */
    Default_Handler,                /* 21 */
    Default_Handler,                /* 22 */
    Default_Handler,                /* 23 */
    Default_Handler,                /* 24 */
    Default_Handler,                /* 25 */
    Default_Handler,                /* 26 */
    Default_Handler,                /* 27 */
    Default_Handler,                /* 28 */
    Default_Handler,                /* 29 */
    Default_Handler,                /* 30 */
    Default_Handler,                /* 31 */
    Default_Handler,                /* 32 */
    Default_Handler,                /* 33 */
    Default_Handler,                /* 34 */
    Default_Handler,                /* 35 */
    Default_Handler,                /* 36 */
    Default_Handler,                /* 37 */
    Default_Handler,                /* 38 */
    Default_Handler,                /* 39 */
    Default_Handler,                /* 40 */
    USART3_irqhandler,              /* 41 USART3 */
};

/* Pad the rest of the table to 128 entries with Default_Handler. */
#define IRQ_REMAINDER (128 - 42)
__attribute__((section(".isr_vector_tail"), used))
void (* const g_pfnVectors_tail[IRQ_REMAINDER])(void) = {
    [0 ... IRQ_REMAINDER - 1] = Default_Handler
};

/* ---- Reset handler ------------------------------------------------ */
extern unsigned long _sidata, _sdata, _edata, _sbss, _ebss;

void Reset_Handler(void) {
    unsigned long *src = &_sidata;
    unsigned long *dst = &_sdata;
    while (dst < &_edata) *dst++ = *src++;
    for (dst = &_sbss; dst < &_ebss; ) *dst++ = 0;
    (void)main();
    while (1) { }
}

void Default_Handler(void) {
    while (1) { }
}