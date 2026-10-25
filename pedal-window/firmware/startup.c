/* Author: jayis1. SPDX-License-Identifier: MIT */
#include <stdint.h>
#include <stddef.h>
extern uint32_t _estack, _sidata, _sdata, _edata, _sbss, _ebss;
extern int main(void);
extern void SysTick_Handler(void);
void Reset_Handler(void);
static void fault(void)
{
    for (;;) { }
}
__attribute__((section(".vectors"), used))
void (*const vectors[])(void) = {
    (void (*)(void))(&_estack), Reset_Handler, fault, fault,
    fault, fault, fault, 0, 0, 0, 0, fault, fault, 0, fault, SysTick_Handler
};
void Reset_Handler(void)
{
    uint32_t *source = &_sidata;
    for (uint32_t *p = &_sdata; p < &_edata; ) {
        *p++ = *source++;
    }
    for (uint32_t *p = &_sbss; p < &_ebss; ) {
        *p++ = 0;
    }
    (void)main();
    fault();
}
void *memset(void *destination, int value, size_t size)
{
    unsigned char *p = destination;
    for (size_t i = 0; i < size; ++i) {
        p[i] = (unsigned char)value;
    }
    return destination;
}
