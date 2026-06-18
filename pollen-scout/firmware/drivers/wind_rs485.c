/*
 * wind_rs485.c - Wind speed/direction sensor driver (RS-485 on UART4)
 * Compatible with Davis 6410-style sensor protocol:
 *  - 19200 baud, 8N1
 *  - Poll: send 'B' byte; sensor responds with 6 bytes: [B] [0..255 speed*1.6]
 *    [0..255 dir*1.41] [checksum_hi] [checksum_lo] [LF]
 * Speed scaling: wind_ms = byte1 / 1.6 * 0.447  (mph -> m/s)
 * Direction: deg = byte2 / 1.41 * 1.4
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 */
#include "wind_rs485.h"
#include "board.h"
#include "registers.h"

#define WIND_POLL_BYTE  'B'

static void rs485_tx_enable(int on)
{
    GPIO_REG(WIND_DE_PORT, GPIO_BSRR_OFFSET) = on ?
        (1U << WIND_DE_PIN) : (1U << (WIND_DE_PIN + 16));
}

int wind_init(void)
{
    /* DE pin as output, idle low (receive) */
    GPIO_REG(WIND_DE_PORT, GPIO_MODER_OFFSET) &= ~(3U << (WIND_DE_PIN * 2));
    GPIO_REG(WIND_DE_PORT, GPIO_MODER_OFFSET) |=  (1U << (WIND_DE_PIN * 2));
    rs485_tx_enable(0);

    /* UART4: 19200 8N1 */
    USART_BRR(UART4_BASE) = APB1_CLOCK / WIND_BAUD;
    USART_CR1(UART4_BASE) = (1U << 3) | (1U << 2) | (1U << 0);  /* TX, RX, UE */
    return 0;
}

int wind_read(wind_data_t *out)
{
    uint8_t resp[6];
    int got = 0, tries = 0;

    rs485_tx_enable(1);
    while (!(USART_ISR(UART4_BASE) & USART_ISR_TXE)) { }
    USART_TDR(UART4_BASE) = WIND_POLL_BYTE;
    while (!(USART_ISR(UART4_BASE) & USART_ISR_TC)) { }
    rs485_tx_enable(0);

    /* Read 6 bytes with a timeout */
    while (got < 6 && tries < 6000) {
        if (USART_ISR(UART4_BASE) & USART_ISR_RXNE) {
            resp[got++] = (uint8_t)USART_RDR(UART4_BASE);
        }
        tries++;
    }
    if (got < 6 || resp[0] != WIND_POLL_BYTE) {
        out->speed_mps = 0.0f;
        out->dir_deg   = 0.0f;
        return -1;
    }
    /* Checksum is sum of first 3 bytes, big-endian */
    uint16_t cksum = ((uint16_t)resp[3] << 8) | resp[4];
    uint16_t calc  = resp[0] + resp[1] + resp[2];
    if (cksum != calc) return -2;

    out->speed_mps = (float)resp[1] / 1.6f * 0.447f;  /* mph -> m/s */
    out->dir_deg   = (float)resp[2] / 1.41f * 1.4f;
    if (out->dir_deg >= 360.0f) out->dir_deg -= 360.0f;
    return 0;
}