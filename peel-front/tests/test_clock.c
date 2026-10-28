/* Author: jayis1. SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
/* Simulated register storage tests masks and failure paths, not hardware. */
#define PF_REGISTERS_H
static uint32_t cr, cfgr, acr, ahb, apb, ma, mb, pa, pb, af, brr, control;
static volatile uint32_t *reg(uint32_t address)
{
    if (address == 0x40023800u) return &cr;
    if (address == 0x40023808u) return &cfgr;
    assert(address == 0x40023c00u);
    return &acr;
}
#define REG(a) (*reg(a))
#define GPIOA 0
#define GPIOB 1
#define MODER(b) ((b) == GPIOA ? ma : mb)
#define PUPDR(b) ((b) == GPIOA ? pa : pb)
#define AFRL(b) af
#define RCC_AHB1ENR ahb
#define RCC_APB1ENR apb
#define USART_BRR brr
#define USART_CR1 control
#include "../firmware/drivers/clock.c"
int main(void)
{
    assert(!board_clock_reset());
    cr = 2; cfgr = 0; acr = 7;
    assert(board_clock_reset() && acr == 0);
    cr = 2; cfgr = 4;
    assert(!board_clock_reset());
    cr = 2 | (1u << 25); cfgr = 0;
    assert(!board_clock_reset());
    ma = 2u << 4; af = 7u << 8; ahb = 3; apb = 1u << 17;
    brr = (CORE_HZ + SERIAL_BAUD / 2u) / SERIAL_BAUD;
    control = (1u << 13) | (1u << 3);
    assert(board_inputs_configured());
    control |= 1u << 2;
    assert(!board_inputs_configured());
    control &= ~(1u << 2); ma |= 1;
    assert(!board_inputs_configured());
    puts("PASS clock masks, bounded failure paths and input configuration");
    return 0;
}
