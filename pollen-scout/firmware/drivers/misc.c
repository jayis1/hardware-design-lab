/*
 * misc.c - Small helpers used by main.c: blower PWM, ESP32 bridge
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 */
#include "board.h"
#include "registers.h"
#include <string.h>

/* ---- Blower PWM (TIM1 CH2 on PE5) ---- */
void blower_set_pwm(uint8_t pct)
{
    /* TIM1 configured for PWM at ~25 kHz. ARR = 4800 (10-bit resolution).
     * CCR2 = pct/100 * ARR. */
    static int inited = 0;
    if (!inited) {
        TIM_PSC(TIM1_BASE)  = 0;
        TIM_ARR(TIM1_BASE)  = 4800;
        TIM_CCMR1(TIM1_BASE) = 0x0068;  /* PWM mode 1, preload */
        TIM_CCER(TIM1_BASE)  = 0x0010;  /* CC2E */
        TIM_BDTR(&TIM1_BASE) = 0x8000;  /* MOE (main output enable) */
        TIM_EGR(TIM1_BASE)   = 1U;
        TIM_CR1(TIM1_BASE)   = 1U;      /* enable */
        inited = 1;
    }
    uint32_t ccr = ((uint32_t)pct * 4800U) / 100U;
    TIM_CCR2(TIM1_BASE) = ccr;
}

/* ---- ESP32-C3 bridge over USART6 ---- */
static volatile uint8_t s_esp_rxbuf[64];
static volatile int     s_esp_rxlen;
static volatile int     s_esp_rxready;

void USART6_IRQHandler(void)
{
    if (USART_ISR(USART6_BASE) & USART_ISR_RXNE) {
        uint8_t b = (uint8_t)USART_RDR(USART6_BASE);
        if (s_esp_rxlen < (int)sizeof(s_esp_rxbuf)) {
            s_esp_rxbuf[s_esp_rxlen++] = b;
            if (b == '\n' || s_esp_rxlen == (int)sizeof(s_esp_rxbuf))
                s_esp_rxready = 1;
        }
    }
}

int esp32_bridge_recv(uint8_t *buf, int maxlen, int timeout_ms)
{
    (void)timeout_ms;
    if (!s_esp_rxready) return 0;
    int n = s_esp_rxlen < maxlen ? s_esp_rxlen : maxlen;
    memcpy(buf, (const void *)s_esp_rxbuf, n);
    s_esp_rxlen = 0;
    s_esp_rxready = 0;
    return n;
}

int esp32_bridge_send(const uint8_t *buf, int len)
{
    for (int i = 0; i < len; i++) {
        while (!(USART_ISR(USART6_BASE) & USART_ISR_TXE)) { }
        USART_TDR(USART6_BASE) = buf[i];
    }
    return 0;
}

void esp32_ota_start(void)
{
    /* Pulse ESP32 BOOT0 + reset to force STM32 system bootloader,
     * then the ESP32 streams the signed image over USART6. */
    GPIO_REG(ESP32_BOOT_PORT, GPIO_BSRR_OFFSET) = (1U << (ESP32_BOOT_PIN + 16));
    GPIO_REG(ESP32_RST_PORT,  GPIO_BSRR_OFFSET) = (1U << (ESP32_RST_PIN + 16));
    for (volatile int i = 0; i < 50000; i++) { }
    GPIO_REG(ESP32_RST_PORT,  GPIO_BSRR_OFFSET) = (1U << ESP32_RST_PIN);
    for (volatile int i = 0; i < 50000; i++) { }
    GPIO_REG(ESP32_BOOT_PORT, GPIO_BSRR_OFFSET) = (1U << ESP32_BOOT_PIN);
}

/* TIM1 BDTR register helper (not in registers.h) */
static volatile uint32_t *TIM_BDTR(volatile uint32_t *tim) {
    return (volatile uint32_t *)((char *)tim + 0x44U);
}