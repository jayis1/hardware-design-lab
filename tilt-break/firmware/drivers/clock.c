/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "board.h"
#include "registers.h"
static bool wait_bits(volatile uint32_t *reg,uint32_t mask,uint32_t desired)
{
    /* SysTick is not available at clock startup; use a bounded iteration wait. */
    for (uint32_t budget = 1000000u; budget; --budget) {
        if ((*reg & mask) == desired)
            return true;
    }
    return false;
}
bool board_clock_reset(void)
{
    RCC_CR |= 1u;
    if (!wait_bits(&RCC_CR, 2u, 2u))
        return false;
    RCC_CFGR &= ~3u;
    if (!wait_bits(&RCC_CFGR, 12u, 0))
        return false;
    const uint32_t prescalers = (15u << 4) | (7u << 10) | (7u << 13);
    RCC_CFGR &= ~prescalers;
    if (RCC_CFGR & prescalers)
        return false;
    RCC_CR &= ~(1u << 24);
    if (!wait_bits(&RCC_CR, 1u << 25, 0))
        return false;
    FLASH_ACR &= ~7u;
    return (FLASH_ACR & 7u) == 0;
}
