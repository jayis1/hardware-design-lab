/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "board.h"
#include "registers.h"
static volatile uint32_t ticks;
void SysTick_Handler(void)
{
    ++ticks;
}
uint32_t board_millis(void)
{
    return ticks;
}
void board_led(bool on)
{
    BSRR(GPIOA) = on ? 1u << 5 : 1u << 21;
}
void board_init(void)
{
    /* HSI 16MHz reset default; no PLL, external crystal or floating point. */
    RCC_AHB1ENR |= 3u;
    RCC_APB1ENR |= 1u << 17;
    (void)RCC_AHB1ENR;
    (void)RCC_APB1ENR;
    MODER(GPIOA) &= ~((3u << 0) | (3u << 2) | (3u << 8));
    MODER(GPIOB) &= ~3u;
    PUPDR(GPIOA) &= ~((3u << 0) | (3u << 2) | (3u << 8));
    PUPDR(GPIOB) &= ~3u;
    board_led(false);
    MODER(GPIOA) = (MODER(GPIOA) & ~(3u << 10)) | (1u << 10);
    MODER(GPIOA) = (MODER(GPIOA) & ~(3u << 4)) | (2u << 4);
    AFRL(GPIOA) = (AFRL(GPIOA) & ~(15u << 8)) | (7u << 8);
    USART_CR1 = 0;
    USART_CR2 = 0;
    USART_CR3 = 0;
    USART_BRR = (CORE_HZ + SERIAL_BAUD / 2u) / SERIAL_BAUD;
    USART_CR1 = (1u << 13) | (1u << 3);
    ticks = 0;
    SYST_RVR = CORE_HZ / 1000u - 1u;
    SYST_CVR = 0;
    SYST_CSR = 7u;
}
uint8_t board_closed(void)
{
    uint32_t a = IDR(GPIOA);
    uint32_t b = IDR(GPIOB);
    uint8_t raw = (uint8_t)((a & 3u) | ((a >> 2) & 4u) | ((b & 1u) << 3));
    return (uint8_t)(~raw & 15u);
}
bool board_watchdog_start(void)
{
    RCC_CSR |= 1u;
    uint32_t start = board_millis();
    while (!(RCC_CSR & 2u)) {
        if (board_millis() - start >= 100u)
            return false;
    }
    IWDG_KR = 0x5555u;
    IWDG_PR = 4u;
    IWDG_RLR = 1023u;
    start = board_millis();
    while (IWDG_SR) {
        if (board_millis() - start >= 100u)
            return false;
    }
    IWDG_KR = 0xaaaau;
    IWDG_KR = 0xccccu;
    return true;
}
void board_watchdog_feed(void)
{
    IWDG_KR = 0xaaaau;
}
