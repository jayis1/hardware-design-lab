/*
 * startup.c — STM32H733 startup + vector table for HydraScan
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 *
 * Minimal Cortex-M7 startup: copies .data, zeroes .bss, and jumps to
 * main(). The vector table is placed first in flash. SysTick_Handler
 * and the default handler live here too; peripheral IRQ handlers are
 * weak aliases so the linker can override them.
 */
#include "registers.h"

extern int main(void);
extern uint32_t _estack, _sdata, _edata, _sdata_load, _sbss, _ebss;

void Reset_Handler(void)
{
    /* Copy .data from flash to RAM. */
    uint32_t *src = &_sdata_load;
    uint32_t *dst = &_sdata;
    while (dst < &_edata) *dst++ = *src++;
    /* Zero .bss. */
    dst = &_sbss;
    while (dst < &_ebss) *dst++ = 0;

    (void)main();
    for (;;) { }   /* if main returns, halt */
}

void Default_Handler(void) { for (;;) { } }

/* Weak aliases so any unimplemented IRQ falls through to Default. */
#define WEAK_HANDLER(name) void name(void) __attribute__((weak, alias("Default_Handler")))
WEAK_HANDLER(NMI_Handler);
WEAK_HANDLER(HardFault_Handler);
WEAK_HANDLER(MemManage_Handler);
WEAK_HANDLER(BusFault_Handler);
WEAK_HANDLER(UsageFault_Handler);
WEAK_HANDLER(SVC_Handler);
WEAK_HANDLER(DebugMon_Handler);
WEAK_HANDLER(PendSV_Handler);
WEAK_HANDLER(SysTick_Handler);

/* Vector table — first entry is the initial stack pointer. */
__attribute__((section(".isr_vector"), used))
void (* const g_pfnVectors[])(void) = {
    (void (*)(void))(&_estack),
    Reset_Handler,
    NMI_Handler,
    HardFault_Handler,
    MemManage_Handler,
    BusFault_Handler,
    UsageFault_Handler,
    0, 0, 0, 0,
    SVC_Handler,
    DebugMon_Handler,
    0,
    PendSV_Handler,
    SysTick_Handler
    /* Device-specific IRQ handlers omitted for brevity; a production
     * build adds the ~150 entries from RM0433 table 5. */
};