/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "board.h"
#include "registers.h"

/* Restore the documented reset clock tree when entered by a debugger.
 * Every wait uses an iteration budget because SysTick is not running yet.
 * On failure the caller stops before initializing the external interface.
 * This is not a general bootloader handoff: interrupts must be disabled
 * and SRAM/vector mapping must still match startup.c/linker.ld. */
#define RCC_CR REG(0x40023800u)
#define RCC_CFGR REG(0x40023808u)
#define FLASH_ACR REG(0x40023c00u)
#define CLOCK_BUDGET 1000000u
#define HSI_ON 1u
#define HSI_READY 2u
#define PLL_ON (1u << 24)
#define PLL_READY (1u << 25)
#define SWITCH_MASK 3u
#define STATUS_MASK 12u
#define PRESCALER_MASK ((15u << 4) | (7u << 10) | (7u << 13))

static bool wait_set(volatile uint32_t *reg, uint32_t mask)
{
    for (uint32_t budget = CLOCK_BUDGET; budget; --budget) {
        if ((*reg & mask) == mask)
            return true;
    }
    return false;
}

static bool wait_clear(volatile uint32_t *reg, uint32_t mask)
{
    for (uint32_t budget = CLOCK_BUDGET; budget; --budget) {
        if (!(*reg & mask))
            return true;
    }
    return false;
}

bool board_clock_reset(void)
{
    RCC_CR |= HSI_ON;
    if (!wait_set(&RCC_CR, HSI_READY))
        return false;
    /* Do not reduce flash latency while a faster clock might be active. */
    RCC_CFGR &= ~SWITCH_MASK;
    if (!wait_clear(&RCC_CFGR, STATUS_MASK))
        return false;
    RCC_CFGR &= ~PRESCALER_MASK;
    if (RCC_CFGR & PRESCALER_MASK)
        return false;
    RCC_CR &= ~PLL_ON;
    if (!wait_clear(&RCC_CR, PLL_READY))
        return false;
    FLASH_ACR &= ~7u;
    if (FLASH_ACR & 7u)
        return false;
    return true;
}

bool board_inputs_configured(void)
{
    /* Fail closed if a debugger or future edit left an input as an output.
     * External 10k pullups, not internal pull resistors, set the bias. */
    const uint32_t a_inputs = (3u << 0) | (3u << 2) | (3u << 8);
    if (MODER(GPIOA) & a_inputs)
        return false;
    if (MODER(GPIOB) & 3u)
        return false;
    if (PUPDR(GPIOA) & a_inputs)
        return false;
    if (PUPDR(GPIOB) & 3u)
        return false;
    if ((MODER(GPIOA) & (3u << 4)) != (2u << 4))
        return false;
    if ((AFRL(GPIOA) & (15u << 8)) != (7u << 8))
        return false;
    if (!(RCC_AHB1ENR & 1u))
        return false;
    if (!(RCC_AHB1ENR & 2u))
        return false;
    if (!(RCC_APB1ENR & (1u << 17)))
        return false;
    if (USART_BRR != (CORE_HZ + SERIAL_BAUD / 2u) / SERIAL_BAUD)
        return false;
    if (!(USART_CR1 & (1u << 13)))
        return false;
    if (!(USART_CR1 & (1u << 3)))
        return false;
    /* A receiver-enabled build violates the receive-only host contract. */
    if (USART_CR1 & (1u << 2))
        return false;
    return true;
}
