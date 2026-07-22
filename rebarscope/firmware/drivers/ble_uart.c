/*
 * drivers/ble_uart.c — BLE 5 GATT custom service over BGM220E UART HCI
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 *
 * The BGM220E module runs a pre-loaded GATT application image exposing a
 * custom NUS-like service. The MCU talks to it over USART2 at 921600 baud
 * using a minimal framed protocol with COBS + CRC-16 for integrity.
 *
 * Service UUID:  0000f4c1-0000-1000-8000-00805f9b34fb
 * RX characteristic (phone → device): 0000f4c2-...
 * TX characteristic (device → phone):  0000f4c3-...
 */
#include "../board.h"
#include "../registers.h"
#include "ble_uart.h"
#include "cobs.h"
#include "crc16.h"

#define BLE_MAX_PAYLOAD 64u
#define BLE_FRAME_MAX   (BLE_MAX_PAYLOAD + 2 + 2)  /* payload + CRC + COBS */

static uint8_t rx_buf[BLE_FRAME_MAX];
static uint16_t rx_idx = 0;
static uint8_t tx_buf[BLE_FRAME_MAX];
static uint8_t decode_buf[BLE_MAX_PAYLOAD];

static ble_rx_cb_t g_rx_cb = 0;
static uint8_t g_connected = 0;

static void ble_delay_us(uint32_t us)
{
    volatile uint32_t n = us * 20u;
    while (n--) __asm volatile("nop");
}

void ble_uart_init(ble_rx_cb_t cb)
{
    /* USART2 pins: PA2=TX, PA3=RX, PA6=CTS, PA7=RTS, AF7 */
    GPIOA->MODER = (GPIOA->MODER & ~(0x3u << 4))  | (0x2u << 4);   /* PA2 AF */
    GPIOA->MODER = (GPIOA->MODER & ~(0x3u << 6))  | (0x2u << 6);   /* PA3 AF */
    GPIOA->AFRL  = (GPIOA->AFRL  & ~(0xFu << 8))  | (0x7u << 8);
    GPIOA->AFRL  = (GPIOA->AFRL  & ~(0xFu << 12)) | (0x7u << 12);

    RCC->APB1ENR1 |= RCC_APB1ENR1_USART2;
    USART2->BRR = 130u;   /* 120 MHz / 921600 ≈ 130 */
    USART2->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
    USART2->CR3 = 0;       /* no flow-control for now */

    /* Reset BGM220E (active-low pulse) */
    GPIOC->BRR = (1u << 8);
    ble_delay_us(10000);
    GPIOC->BSRR = (1u << 8);
    ble_delay_us(50000);

    g_rx_cb = cb;
}

int ble_uart_send(const uint8_t *payload, uint8_t len)
{
    if (len > BLE_MAX_PAYLOAD) return -1;
    uint16_t crc = crc16_compute(payload, len);
    /* Frame = [len][payload][crc_lo][crc_hi], COBS-encoded */
    uint8_t raw[BLE_MAX_PAYLOAD + 3];
    raw[0] = len;
    for (uint8_t i = 0; i < len; i++) raw[1 + i] = payload[i];
    raw[1 + len] = (uint8_t)(crc & 0xFF);
    raw[2 + len] = (uint8_t)(crc >> 8);
    uint8_t cobs_len = cobs_encode(raw, 1u + len + 2u, tx_buf);
    tx_buf[cobs_len++] = 0x00;   /* delimiter */
    for (uint8_t i = 0; i < cobs_len; i++) {
        while (!(USART2->ISR & USART_ISR_TXE)) ;
        USART2->TDR = tx_buf[i];
    }
    return 0;
}

void ble_uart_poll(void)
{
    while (USART2->ISR & USART_ISR_RXNE) {
        uint8_t c = USART2->RDR;
        if (c == 0x00) {
            /* end of frame */
            if (rx_idx > 0) {
                uint8_t decoded = cobs_decode(rx_buf, rx_idx, decode_buf);
                rx_idx = 0;
                if (decoded >= 3u) {
                    uint8_t plen = decode_buf[0];
                    if (plen + 3u == decoded) {
                        uint16_t crc_recv = (uint16_t)decode_buf[1 + plen] |
                                            ((uint16_t)decode_buf[2 + plen] << 8);
                        uint16_t crc_calc = crc16_compute(&decode_buf[1], plen);
                        if (crc_recv == crc_calc && g_rx_cb) {
                            g_rx_cb(&decode_buf[1], plen);
                        }
                    }
                }
            }
        } else {
            if (rx_idx < BLE_FRAME_MAX) {
                rx_buf[rx_idx++] = c;
            } else {
                rx_idx = 0;   /* overflow, restart */
            }
        }
    }
}

void ble_uart_set_connected(uint8_t connected)
{
    g_connected = connected;
}

uint8_t ble_uart_is_connected(void)
{
    return g_connected;
}