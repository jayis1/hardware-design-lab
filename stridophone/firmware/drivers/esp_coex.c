/*
 * esp_coex.c — UART protocol bridge to the ESP32-C6 connectivity coprocessor.
 *
 * Author : jayis1
 * License: MIT
 *
 * Frame format (little-endian):
 *   0xAA | 0x55 | type(1) | len_lo(1) | len_hi(1) | payload[len] | crc(1)
 * CRC is XOR of type, len_lo, len_hi, and all payload bytes.
 *
 * Message types:
 *   0x01 HELLO      (ESP -> STM32, sent after boot)
 *   0x02 WIFI_UP    (ESP -> STM32)
 *   0x03 WIFI_DOWN  (ESP -> STM32)
 *   0x10 EVENT_JSON (STM32 -> ESP, payload = UTF-8 JSON)
 *   0x11 WAV_REF    (STM32 -> ESP, payload = uint32 event id to stream)
 *   0x20 CFG_SET    (ESP -> STM32, payload = config blob)
 *   0x21 CFG_GET    (STM32 -> ESP)
 */
#include "esp_coex.h"
#include "../registers.h"
#include <string.h>

#define ESP_BUF_SZ 256
static uint8_t  g_rx_buf[ESP_BUF_SZ];
static volatile int g_rx_len = 0;
static volatile uint8_t g_rx_state = 0;   /* simple state machine */
static volatile uint8_t g_rx_type = 0;
static volatile int     g_rx_expect = 0;
static volatile int     g_rx_idx = 0;
static volatile uint8_t g_rx_crc = 0;

static volatile int g_wifi_up = 0;

void USART3_irqhandler(void) {
    while (USART3->ISR & USART_ISR_RXNE) {
        uint8_t b = (uint8_t)USART3->RDR;
        switch (g_rx_state) {
        case 0: if (b == 0xAA) g_rx_state = 1; break;
        case 1: if (b == 0x55) g_rx_state = 2; else g_rx_state = 0; break;
        case 2: g_rx_type = b; g_rx_crc = b; g_rx_state = 3; break;
        case 3: g_rx_expect = b; g_rx_crc ^= b; g_rx_state = 4; break;
        case 4: g_rx_expect |= (b << 8); g_rx_crc ^= b; g_rx_state = 5; g_rx_idx = 0; break;
        case 5:
            if (g_rx_idx < ESP_BUF_SZ) g_rx_buf[g_rx_idx] = b;
            g_rx_crc ^= b;
            g_rx_idx++;
            if (g_rx_idx >= g_rx_expect) g_rx_state = 6;
            break;
        case 6:
            if (b == g_rx_crc) {
                g_rx_len = g_rx_idx;
            } else {
                g_rx_len = 0;     /* CRC fail: drop */
            }
            g_rx_state = 0;
            break;
        }
    }
    USART3->ICR = 0xFFFFFFFFU;
}

void esp_coex_init(void) {
    RCC->APB1LENR |= RCC_APB1LENR_USART3;
    USART3->BRR = BOARD_APB1_HZ / ESP_UART_BAUD;
    USART3->CR3 = 0;
    USART3->CR2 = 0;
    USART3->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE;
    nvic_enable(USART3_IRQ);
    g_rx_state = 0;
    g_rx_len = 0;
    g_wifi_up = 0;
}

int esp_coex_boot(void) {
    /* Pulse EN low for 10 ms then high, with BOOT0 high (run). */
    gpio_set(ESP_BOOT_PORT, ESP_BOOT_PAD);
    gpio_clr(ESP_EN_PORT,   ESP_EN_PAD);
    for (volatile int i = 0; i < 100000; i++) { }
    gpio_set(ESP_EN_PORT, ESP_EN_PAD);
    /* Wait up to ~1 s for a HELLO frame. */
    for (int tries = 0; tries < 1000; tries++) {
        uint8_t t; uint8_t b[4];
        int n = esp_coex_recv(&t, b, 4);
        if (n > 0 && t == 0x01) return 0;
        for (volatile int j = 0; j < 1000; j++) { }
    }
    return -1;
}

int esp_coex_send(uint8_t type, const uint8_t *payload, int len) {
    if (len > 255) return -1;
    while (!(USART3->ISR & USART_ISR_TXE)) { }
    USART3->TDR = 0xAA;
    while (!(USART3->ISR & USART_ISR_TXE)) { }
    USART3->TDR = 0x55;
    while (!(USART3->ISR & USART_ISR_TXE)) { }
    USART3->TDR = type;
    uint8_t crc = type;
    while (!(USART3->ISR & USART_ISR_TXE)) { }
    USART3->TDR = (uint8_t)(len & 0xFF); crc ^= (uint8_t)(len & 0xFF);
    while (!(USART3->ISR & USART_ISR_TXE)) { }
    USART3->TDR = (uint8_t)((len >> 8) & 0xFF); crc ^= (uint8_t)((len >> 8) & 0xFF);
    for (int i = 0; i < len; i++) {
        while (!(USART3->ISR & USART_ISR_TXE)) { }
        USART3->TDR = payload[i];
        crc ^= payload[i];
    }
    while (!(USART3->ISR & USART_ISR_TXE)) { }
    USART3->TDR = crc;
    return len;
}

int esp_coex_recv(uint8_t *type, uint8_t *buf, int maxlen) {
    int n = g_rx_len;
    if (n == 0) return 0;
    *type = g_rx_type;
    if (n > maxlen) n = maxlen;
    memcpy(buf, (const void *)g_rx_buf, n);
    g_rx_len = 0;     /* consume */
    /* Side-effect: act on WIFI_UP/DOWN. */
    if (*type == 0x02) g_wifi_up = 1;
    else if (*type == 0x03) g_wifi_up = 0;
    return n;
}

int esp_wifi_up(void) { return g_wifi_up; }