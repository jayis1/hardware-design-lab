/*
 * drivers/ble.c — BLE GATT server via ANNA-B112 (Nordic UART Service)
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 *
 * The ANNA-B112 module is a pre-certified BLE 5.2 SoC running u-blox's
 * NINA-B3 firmware exposing a Nordic-UART-Service transparent pipe over
 * UART4. We send newline-terminated ASCII lines to push results, and
 * receive lines for library commands. This keeps the BLE stack on the
 * module, so the STM32H7 side has no Bluetooth code at all.
 *
 * Wire protocol (ASCII, lines end with \n):
 *   Out:  R,<class_id>,<confidence>,<adulterant>,<ratio>,<temp_c>\n
 *   In:   L,ADD,<class_id>,<n>,<16×PCA-centroid floats>,<16×variances>\n
 */
#include "ble.h"
#include "../registers.h"
#include <stdio.h>
#include <string.h>

static const hgpio_t tx  = PIN_BLE_TX;
static const hgpio_t rx  = PIN_BLE_RX;
static const hgpio_t en  = PIN_BLE_EN;

static uint8_t  connected = 0;
static uint8_t  rx_buf[256];
static uint16_t rx_len = 0;

static void uart4_init(void)
{
    RCC_REG32(RCC_APB1LENR_OF) |= RCC_APB1LENR_UART4EN;
    (void)RCC_REG32(RCC_APB1LENR_OF);
    hgpio_t pins[2] = { tx, rx };
    for (int i = 0; i < 2; ++i) {
        pins[i].port->MODER  &= ~(3u << (2u * pins[i].pin));
        pins[i].port->MODER  |= (GPIO_MODE_AF << (2u * pins[i].pin));
        /* AF8 = UART4 on PA0/PA1 */
        pins[i].port->AFRL |= (8u << (4u * pins[i].pin));
    }
    /* 115200-8-N-1 from 120 MHz PCLK1. BRR = 120e6/115200 ≈ 1042. */
    UART4.BRR = 1042u;
    UART4.CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

static void uart4_putc(char c)
{
    while (!(UART4.ISR & USART_ISR_TXE)) { }
    UART4.TDR = (uint32_t)c;
}

static void uart4_puts(const char *s)
{
    while (*s) uart4_putc(*s++);
}

hydra_err_t ble_init(void)
{
    /* EN pin as output, module off initially. */
    en.port->MODER &= ~(3u << (2u * en.pin));
    en.port->MODER |= (GPIO_MODE_OUTPUT << (2u * en.pin));
    en.port->BSRR = (1u << en.pin) << 16;   /* EN = 0 → off          */
    board_delay_ms(50);
    en.port->BSRR = 1u << en.pin;           /* EN = 1 → power up     */
    board_delay_ms(300);
    uart4_init();

    /* Module starts advertising automatically; we wait for a connect
     * indication line "+CONNECTED" delivered over the UART pipe. */
    connected = 0;
    return HYDRA_OK;
}

uint8_t ble_is_connected(void) { return connected; }

void ble_notify_result(const classify_result_t *r, float temp_c)
{
    char line[96];
    int n = snprintf(line, sizeof(line),
                     "R,%u,%.3f,%u,%.3f,%.2f\n",
                     r->class_id, (double)r->confidence,
                     r->adulterant ? 1u : 0u,
                     (double)r->adulterant_ratio, (double)temp_c);
    if (n > 0 && (size_t)n < sizeof(line)) uart4_puts(line);
}

void ble_poll(void)
{
    /* Drain the UART RX into rx_buf until \n; then parse the line. */
    while (UART4.ISR & USART_ISR_RXNE) {
        char c = (char)(UART4.RDR & 0xFF);
        if (rx_len < sizeof(rx_buf) - 1) rx_buf[rx_len++] = c;
        if (c == '\n') {
            rx_buf[rx_len] = 0;
            /* Simple line classifier for module status + app commands. */
            if (strncmp((const char *)rx_buf, "+CONNECTED", 10) == 0) {
                connected = 1;
            } else if (strncmp((const char *)rx_buf, "+DISCONNECTED", 13) == 0) {
                connected = 0;
            } else if (rx_buf[0] == 'L' && rx_buf[1] == ',') {
                /* Library add/update command — full parse handled in
                 * flash_lib.c::library_handle_command(). */
                extern void library_handle_command(const char *line);
                library_handle_command((const char *)rx_buf);
            }
            rx_len = 0;
        }
    }
}