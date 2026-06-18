/*
 * pms5003.c - Plantower PMS5003 particulate sensor driver (USART3)
 * PMS5003 outputs 32-byte frames at 9600 baud, active mode.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 */
#include "pms5003.h"
#include "board.h"
#include "registers.h"
#include <string.h>

#define PMS_FRAME_LEN 32
#define PMS_START1 0x42
#define PMS_START2 0x4D

static uint8_t  s_rxbuf[PMS_FRAME_LEN];
static volatile uint8_t s_idx;
static volatile int     s_frame_ready;
static pms_data_t s_last;

/* USART3 ISR (declared in vector table) */
void USART3_IRQHandler(void)
{
    if (USART_ISR(USART3_BASE) & USART_ISR_RXNE) {
        uint8_t b = (uint8_t)USART_RDR(USART3_BASE);
        if (s_idx == 0 && b != PMS_START1) { return; }
        if (s_idx == 1 && b != PMS_START2) { s_idx = 0; return; }
        s_rxbuf[s_idx++] = b;
        if (s_idx >= PMS_FRAME_LEN) {
            s_idx = 0;
            s_frame_ready = 1;
        }
    }
}

int pms5003_init(void)
{
    /* Configure USART3: 9600 8N1, enable RX interrupt */
    USART_BRR(USART3_BASE) = APB1_CLOCK / PMS5003_BAUD;
    USART_CR1(USART3_BASE) = (1U << 2) | (1U << 5) | (1U << 0);  /* RX, RXNEIE, UE */
    s_idx = 0;
    s_frame_ready = 0;
    return 0;
}

static uint16_t u16(const uint8_t *p) { return ((uint16_t)p[0] << 8) | p[1]; }

int pms5003_read(pms_data_t *out)
{
    if (!s_frame_ready) {
        *out = s_last;
        return 0;
    }
    s_frame_ready = 0;
    /* validate checksum */
    uint16_t cksum = 0;
    for (int i = 0; i < PMS_FRAME_LEN - 2; i++) cksum += s_rxbuf[i];
    if (cksum != u16(&s_rxbuf[30])) return -1;

    s_last.pm1_0  = u16(&s_rxbuf[10]);
    s_last.pm2_5  = u16(&s_rxbuf[12]);
    s_last.pm10   = u16(&s_rxbuf[14]);
    s_last.pm1_0_factory = u16(&s_rxbuf[4]);
    s_last.pm2_5_factory = u16(&s_rxbuf[6]);
    s_last.pm10_factory  = u16(&s_rxbuf[8]);
    s_last.particles_03 = u16(&s_rxbuf[16]);
    s_last.particles_05 = u16(&s_rxbuf[18]);
    s_last.particles_1  = u16(&s_rxbuf[20]);
    s_last.particles_25 = u16(&s_rxbuf[22]);
    s_last.particles_5  = u16(&s_rxbuf[24]);
    s_last.particles_10 = u16(&s_rxbuf[26]);

    *out = s_last;
    return 0;
}

int pms5003_sleep(void)
{
    /* Command: sleep = 0x42 0x4D 0xE4 0x00 0x00 0x01 0x73 */
    static const uint8_t cmd[7] = {0x42,0x4D,0xE4,0x00,0x00,0x01,0x73};
    for (int i = 0; i < 7; i++) {
        while (!(USART_ISR(USART3_BASE) & USART_ISR_TXE)) { }
        USART_TDR(USART3_BASE) = cmd[i];
    }
    return 0;
}

int pms5003_wakeup(void)
{
    static const uint8_t cmd[7] = {0x42,0x4D,0xE4,0x00,0x01,0x01,0x74};
    for (int i = 0; i < 7; i++) {
        while (!(USART_ISR(USART3_BASE) & USART_ISR_TXE)) { }
        USART_TDR(USART3_BASE) = cmd[i];
    }
    return 0;
}