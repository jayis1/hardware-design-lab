/*
 * ble_drv.c — BLE UART driver for BMD-300 module
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Communicates with the BMD-300 BLE module over USART2 using the
 * Nordic UART Service (NUS) transparent UART mode. The module handles
 * all BLE protocol; the MCU just sees a serial port.
 *
 * Also implements a simple JSON command parser for receiving commands
 * from the companion app (trigger, calibrate, mode switch, etc.).
 */

#include "ble_drv.h"
#include "registers.h"
#include "board.h"
#include <string.h>

static bool ble_connected = false;
static uint8_t rx_buffer[BLE_RX_BUF_SIZE];
static volatile uint32_t rx_head = 0;
static volatile uint32_t rx_tail = 0;

static uint8_t json_buf[BLE_RX_BUF_SIZE];
static uint32_t json_len = 0;

void ble_drv_init(void)
{
    /* Enable USART2 and GPIOA clocks */
    RCC_APB1ENR1 |= RCC_APB1ENR1_USART2;
    RCC_AHB2ENR |= RCC_AHB2ENR_GPIOA;

    /* Configure PA14 (TX) and PA15 (RX) as AF7 (USART2) */
    GPIOA->MODER &= ~(3UL << (BLE_TX_PIN * 2));
    GPIOA->MODER |= (GPIO_MODE_AF << (BLE_TX_PIN * 2));
    GPIOA->AFRH &= ~(0xFUL << ((BLE_TX_PIN - 8) * 4));
    GPIOA->AFRH |= (BLE_AF << ((BLE_TX_PIN - 8) * 4));

    GPIOA->MODER &= ~(3UL << (BLE_RX_PIN * 2));
    GPIOA->MODER |= (GPIO_MODE_AF << (BLE_RX_PIN * 2));
    GPIOA->AFRH &= ~(0xFUL << ((BLE_RX_PIN - 8) * 4));
    GPIOA->AFRH |= (BLE_AF << ((BLE_RX_PIN - 8) * 4));

    /* Configure USART2: 115200 baud, 8N1, enable RX interrupt */
    USART2->BRR = (SYSCLK_HZ / BLE_BAUD);
    USART2->CR1 = USART_CR1_UE | USART_CR1_RE | USART_CR1_TE | USART_CR1_RXNEIE;

    /* Enable USART2 interrupt in NVIC */
    NVIC_IPR(IRQ_USART2) = (6 << 4);  /* Priority 6 */
    NVIC_ISER(0) = (1UL << IRQ_USART2);
}

bool ble_drv_is_connected(void)
{
    return ble_connected;
}

void ble_drv_set_connected(bool connected)
{
    ble_connected = connected;
}

uint32_t ble_drv_send(const uint8_t *data, uint32_t len)
{
    uint32_t sent = 0;
    for (uint32_t i = 0; i < len; i++) {
        while (!(USART2->ISR & USART_ISR_TXE))
            ;
        USART2->TDR = data[i];
        sent++;
    }
    return sent;
}

void ble_drv_send_string(const char *str)
{
    ble_drv_send((const uint8_t *)str, strlen(str));
}

uint32_t ble_drv_receive(uint8_t *buffer, uint32_t max_len)
{
    uint32_t count = 0;
    while (rx_tail != rx_head && count < max_len) {
        buffer[count++] = rx_buffer[rx_tail];
        rx_tail = (rx_tail + 1) % BLE_RX_BUF_SIZE;
    }
    return count;
}

/* Simple JSON result formatter */
static int json_append(char *buf, int pos, const char *fmt, ...)
{
    /* In production, use a proper snprintf. Here we do simple concatenation. */
    return pos;
}

void ble_drv_send_scan_result(const char *alloy_name, float confidence,
                               const char *alts[], int alt_count,
                               float liftoff_mm)
{
    /* Build JSON: {"type":"scan","alloy":"XXX","confidence":0.89,...}
     * Since we don't have snprintf in this minimal environment,
     * we build the string manually with a simple float-to-ASCII helper. */

    char buf[BLE_TX_BUF_SIZE];
    int pos = 0;

    /* Helper to append a string */
    #define APPEND_STR(s) do { \
        const char *_s = (s); \
        while (*_s && pos < (int)sizeof(buf) - 1) buf[pos++] = *_s++; \
    } while (0)

    /* Helper to append an integer */
    #define APPEND_INT(n) do { \
        int _n = (n); \
        if (_n < 0) { buf[pos++] = '-'; _n = -_n; } \
        char _tmp[12]; int _i = 0; \
        if (_n == 0) buf[pos++] = '0'; \
        while (_n > 0) { _tmp[_i++] = '0' + (_n % 10); _n /= 10; } \
        while (_i > 0 && pos < (int)sizeof(buf) - 1) buf[pos++] = _tmp[--_i]; \
    } while (0)

    /* Helper to append a float (2 decimal places) */
    #define APPEND_FLOAT(f) do { \
        float _f = (f); \
        if (_f < 0) { buf[pos++] = '-'; _f = -_f; } \
        int _int = (int)_f; \
        int _frac = (int)((_f - _int) * 100.0f + 0.5f); \
        APPEND_INT(_int); \
        buf[pos++] = '.'; \
        if (_frac < 10) buf[pos++] = '0'; \
        APPEND_INT(_frac); \
    } while (0)

    APPEND_STR("{\"type\":\"scan\",\"alloy\":\"");
    APPEND_STR(alloy_name);
    APPEND_STR("\",\"confidence\":");
    APPEND_FLOAT(confidence);

    if (alt_count > 0) {
        APPEND_STR(",\"alts\":[");
        for (int i = 0; i < alt_count; i++) {
            if (i > 0) buf[pos++] = ',';
            buf[pos++] = '"';
            APPEND_STR(alts[i]);
            buf[pos++] = '"';
        }
        buf[pos++] = ']';
    }

    APPEND_STR(",\"liftoff\":");
    APPEND_FLOAT(liftoff_mm);
    buf[pos++] = '}';
    buf[pos++] = '\n';

    ble_drv_send((uint8_t *)buf, pos);

    #undef APPEND_STR
    #undef APPEND_INT
    #undef APPEND_FLOAT
}

void ble_drv_process(void)
{
    /* Process received characters, looking for complete JSON commands */
    uint8_t ch;
    while (ble_drv_receive(&ch, 1) > 0) {
        if (ch == '\n' || json_len >= BLE_RX_BUF_SIZE - 1) {
            if (json_len > 0) {
                json_buf[json_len] = '\0';
                /* Parse and handle command
                 * Commands: {"cmd":"trigger"}, {"cmd":"calibrate","step":0}, etc.
                 * In production, use a proper JSON parser. Here we do simple
                 * string matching. */
                if (strstr((char *)json_buf, "\"trigger\"") != NULL) {
                    /* Signal main loop to trigger a scan */
                    /* This would set a flag read by the state machine */
                }
                if (strstr((char *)json_buf, "\"calibrate\"") != NULL) {
                    /* Enter calibration mode */
                }
                json_len = 0;
            }
        } else {
            json_buf[json_len++] = ch;
        }
    }
}

/* USART2 RX interrupt handler */
void USART2_IRQHandler(void)
{
    if (USART2->ISR & USART_ISR_RXNE) {
        uint8_t ch = (uint8_t)USART2->RDR;
        uint32_t next = (rx_head + 1) % BLE_RX_BUF_SIZE;
        if (next != rx_tail) {
            rx_buffer[rx_head] = ch;
            rx_head = next;
        }
    }
}