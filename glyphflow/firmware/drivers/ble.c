/*
 * ble.c — BLE module transport over UART (BGM220P / EFR32BG22)
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 *
 * The BGM220P runs a pre-flashed BLE HID-over-GATT stack with a simple
 * NCP-style command protocol over UART (115200 8N1). This driver
 * implements the small subset of commands GlyphFlow needs:
 *   - start / stop advertising
 *   - send an HID keyboard report
 *   - notify a custom trajectory characteristic
 *   - send battery level
 *   - sleep / wake the radio
 *
 * Command frame format: [0xAA][LEN][CMD][payload...][CRC8][0x55]
 */
#include "ble.h"
#include "../registers.h"
#include "../board.h"
#include <string.h>

#define BLE_FRAME_SOF  0xAA
#define BLE_FRAME_EOF  0x55

/* Command opcodes used by the BGM220P application image. */
#define BLE_CMD_ADVERTISE_START   0x01
#define BLE_CMD_ADVERTISE_STOP    0x02
#define BLE_CMD_HID_REPORT        0x10
#define BLE_CMD_NOTIFY_TRAJ       0x11
#define BLE_CMD_BATTERY          0x12
#define BLE_CMD_STATUS           0x13
#define BLE_CMD_ACTIVE_SET       0x14
#define BLE_CMD_SLEEP            0x20
#define BLE_CMD_WAKE             0x21
#define BLE_CMD_CONN_STATE        0x30

static volatile int g_connected = 0;
static volatile uint8_t g_rx_byte;
static volatile uint8_t g_rx_ready = 0;

/* ---- CRC8 (polynomial 0x07, init 0x00) --------------------------- */
static uint8_t crc8(const uint8_t *p, uint8_t n)
{
    uint8_t c = 0;
    for (uint8_t i = 0; i < n; i++) {
        c ^= p[i];
        for (uint8_t b = 0; b < 8; b++) {
            if (c & 0x80) c = (uint8_t)((c << 1) ^ 0x07);
            else          c = (uint8_t)(c << 1);
        }
    }
    return c;
}

/* ---- UART1 init --------------------------------------------------- */
static void uart_init(void)
{
    RCC_AHB2ENR  |= RCC_AHB2ENR_GPIOAEN;
    RCC_APB2ENR  |= RCC_APB2ENR_USART1EN;

    /* PA9 = TX (AF7), PA10 = RX (AF7) */
    GPIOA->MODER = (GPIOA->MODER & ~(3U << (9*2)))  | (GPIO_MODE_AF << (9*2));
    GPIOA->MODER = (GPIOA->MODER & ~(3U << (10*2))) | (GPIO_MODE_AF << (10*2));
    /* PA9/PA10 are pins 9/10 → use AFRH (pins 8-15), alt fn 7. */
    GPIOA->AFRH  = (GPIOA->AFRH  & ~(0xFU << ((9-8)*4)))  | (7U << ((9-8)*4));
    GPIOA->AFRH  = (GPIOA->AFRH  & ~(0xFU << ((10-8)*4))) | (7U << ((10-8)*4));

    /* PA2 = BLE WAKE / RST (output, high idle) */
    GPIOA->MODER = (GPIOA->MODER & ~(3U << (PIN_BLE_WAKE_BIT*2)))
                 | (GPIO_MODE_OUT << (PIN_BLE_WAKE_BIT*2));
    GPIO_SET(PIN_BLE_WAKE_PORT, PIN_BLE_WAKE_BIT);

    /* 115200 baud: PCLK2 = 80 MHz, oversample 16 → BRR = 80000000/115200 ≈ 694 */
    USART1->BRR = 694;
    USART1->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE;
    /* Enable the USART1 IRQ for RX. */
    NVIC_ISER0 = (1U << 27);  /* USART1_IRQn = 27 on Cortex-M4 */
}

/* ---- Blocking send/recv ------------------------------------------- */
static void uart_putc(uint8_t c)
{
    while (!(USART1->ISR & USART_ISR_TXE)) { }
    USART1->TDR = c;
}

static uint8_t uart_getc(uint32_t timeout_ticks)
{
    uint32_t t = 0;
    while (!(USART1->ISR & USART_ISR_RXNE)) {
        if (++t > timeout_ticks) return 0xFF;
    }
    return (uint8_t)USART1->RDR;
}

/* ---- Frame send with ack ------------------------------------------ */
static int ble_send_frame(uint8_t cmd, const uint8_t *payload, uint8_t plen)
{
    uint8_t buf[32];
    if (plen > 28) return -1;
    buf[0] = BLE_FRAME_SOF;
    buf[1] = plen + 1;       /* LEN = cmd byte + payload */
    buf[2] = cmd;
    memcpy(&buf[3], payload, plen);
    uint8_t crc = crc8(&buf[1], buf[1] + 1);  /* CRC over LEN..payload */
    buf[3 + plen] = crc;
    buf[4 + plen] = BLE_FRAME_EOF;

    for (uint8_t i = 0; i < (uint8_t)(5 + plen); i++) uart_putc(buf[i]);

    /* Read ack: same frame with cmd | 0x80. */
    uint8_t sof = uart_getc(200000);
    if (sof != BLE_FRAME_SOF) return -1;
    (void)uart_getc(200000);  /* LEN */
    uint8_t ackcmd = uart_getc(200000);
    if (ackcmd != (cmd | 0x80)) return -1;
    /* Drain remaining bytes of the ack (we don't parse the body here). */
    for (int i = 0; i < 4; i++) (void)uart_getc(200000);
    return 0;
}

/* ---- USART1 IRQ (RX) --------------------------------------------- */
void USART1_IRQHandler(void)
{
    if (USART1->ISR & USART_ISR_RXNE) {
        g_rx_byte = (uint8_t)USART1->RDR;
        g_rx_ready = 1;
        /* Minimal response parser: a 'C' means connected, 'D' disconnected. */
        if (g_rx_byte == 'C') g_connected = 1;
        else if (g_rx_byte == 'D') g_connected = 0;
    }
}

/* ---- Public API -------------------------------------------------- */
int ble_init(void)
{
    uart_init();
    /* Pulse WAKE low for 10 ms to reset the module. */
    GPIO_CLR(PIN_BLE_WAKE_PORT, PIN_BLE_WAKE_BIT);
    for (volatile int i = 0; i < 80000; i++) { }
    GPIO_SET(PIN_BLE_WAKE_PORT, PIN_BLE_WAKE_BIT);
    for (volatile int i = 0; i < 80000; i++) { }
    return 0;
}

int ble_advertise_start(void) { return ble_send_frame(BLE_CMD_ADVERTISE_START, 0, 0); }
int ble_advertise_stop(void)  { return ble_send_frame(BLE_CMD_ADVERTISE_STOP,  0, 0); }

int ble_send_hid(const hid_report_t *r)
{
    uint8_t buf[8];
    buf[0] = r->modifiers;
    buf[1] = r->reserved;
    memcpy(&buf[2], r->keys, 6);
    return ble_send_frame(BLE_CMD_HID_REPORT, buf, 8);
}

int ble_send_char(uint8_t hid_scancode, uint8_t modifiers)
{
    hid_report_t r = { .modifiers = modifiers, .reserved = 0,
                       .keys = { hid_scancode, 0,0,0,0,0 } };
    return ble_send_hid(&r);
}

int ble_notify_trajectory(const int16_t *pts, uint8_t n)
{
    /* Send up to 7 int16 points per frame (14 bytes). Caller chunks. */
    if (n > 7) n = 7;
    uint8_t buf[14];
    for (uint8_t i = 0; i < n; i++) {
        buf[i*2]   = (uint8_t)(pts[i] >> 8);
        buf[i*2+1] = (uint8_t)(pts[i] & 0xFF);
    }
    return ble_send_frame(BLE_CMD_NOTIFY_TRAJ, buf, n * 2);
}

int ble_send_battery(uint8_t percent)
{
    return ble_send_frame(BLE_CMD_BATTERY, &percent, 1);
}

int ble_send_status(const char *msg, uint8_t len)
{
    if (len > 28) len = 28;
    return ble_send_frame(BLE_CMD_STATUS, (const uint8_t *)msg, len);
}

int ble_set_active_set(uint8_t set_mask)
{
    return ble_send_frame(BLE_CMD_ACTIVE_SET, &set_mask, 1);
}

int ble_sleep(void) { return ble_send_frame(BLE_CMD_SLEEP, 0, 0); }
int ble_wake(void)  { return ble_send_frame(BLE_CMD_WAKE,  0, 0); }

int ble_is_connected(void) { return g_connected; }