/*
 * ble_svc.c — BLE GATT service bridge for walk-by download.
 *
 * The nRF52840 co-MCU runs the BLE stack and communicates with the STM32L4
 * over a UART link (USART1, 115200 baud). The STM32 sends commands; the
 * nRF52840 acks with events. The protocol is a simple framed line format:
 *
 *   STX <cmd> [payload] ETX
 *
 * Commands:
 *   A — start advertising (payload: node_id string)
 *   S — stop advertising
 *   R — read status (STM32 responds with packed status)
 *   B — begin bulk transfer
 *   D — data record (64 bytes)
 *   E — end bulk transfer
 *   C — config update from phone
 *
 * Author: jayis1
 * License: MIT
 */

#include "ble_svc.h"
#include "../board.h"
#include "../registers.h"
#include <string.h>

#define BLE_FRAME_STX   0x02
#define BLE_FRAME_ETX   0x03

static int s_initialized = 0;

/* UART1 ring buffer for receiving events from the nRF52840. */
#define BLE_RX_BUF 128
static uint8_t  s_rxbuf[BLE_RX_BUF];
static volatile uint16_t s_rx_head = 0;
static volatile uint16_t s_rx_tail = 0;

/* ------------------------------------------------------------------------ */
/* USART1 setup                                                              */
/* ------------------------------------------------------------------------ */
static void uart1_init(void)
{
    /* PA9 (TX) AF7, PA10 (RX) AF7. */
    volatile uint32_t *pa = (volatile uint32_t *)GPIOA;
    pa[7] = (pa[7] & ~(0xFU << (9*4))) | (0x7U << (9*4));   /* PA9 AFRL */
    pa[8] = (pa[8] & ~(0xFU << (2*4))) | (0x7U << (2*4));   /* PA10 AFRH */
    pa[0] = (pa[0] & ~(0x3U << (9*2))) | (GPIO_MODE_AF << (9*2));
    pa[0] = (pa[0] & ~(0x3U << (10*2))) | (GPIO_MODE_AF << (10*2));

    /* BRR = 80 MHz / 115200 */
    USART1->BRR = BOARD_SYSCLK_HZ / BLE_UART_BAUD;
    USART1->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
}

static void uart1_tx(const uint8_t *buf, int len)
{
    for (int i = 0; i < len; i++) {
        while (!(USART1->ISR & USART_ISR_TXE)) { }
        USART1->TDR = buf[i];
    }
    while (!(USART1->ISR & USART_ISR_TC)) { }
}

static int uart1_rx_byte(uint8_t *b, uint32_t timeout_ms)
{
    for (uint32_t t = 0; t < timeout_ms * 100; t++) {
        if (USART1->ISR & USART_ISR_RXNE) {
            *b = (uint8_t)USART1->RDR;
            return 1;
        }
        for (volatile int d = 0; d < 100; d++) { }
    }
    return 0;
}

static void send_frame(uint8_t cmd, const uint8_t *payload, int len)
{
    uint8_t hdr[2] = { BLE_FRAME_STX, cmd };
    uart1_tx(hdr, 2);
    if (len > 0 && payload != NULL) uart1_tx(payload, len);
    uint8_t etx = BLE_FRAME_ETX;
    uart1_tx(&etx, 1);
}

/* ------------------------------------------------------------------------ */
/* Public API                                                                 */
/* ------------------------------------------------------------------------ */
int ble_init(void)
{
    uart1_init();

    /* Reset the nRF52840. */
    volatile uint32_t *pa = (volatile uint32_t *)GPIOA;
    /* PA15 output */
    pa[0] = (pa[0] & ~(0x3U << (15*2))) | (GPIO_MODE_OUTPUT << (15*2));
    pa[6] = (1U << 15);  /* low */
    for (volatile int d = 0; d < 50000; d++) { }
    pa[5] = (1U << 15);  /* high */
    for (volatile int d = 0; d < 50000; d++) { }

    s_initialized = 1;
    return 0;
}

int ble_advertise_start(const char *node_id)
{
    if (!s_initialized) return -1;
    /* Pack: 1 byte len + node_id (up to 11). */
    uint8_t buf[12];
    size_t n = strlen(node_id);
    if (n > 11) n = 11;
    buf[0] = (uint8_t)n;
    memcpy(&buf[1], node_id, n);
    send_frame('A', buf, (int)n + 1);
    return 0;
}

int ble_stop_advertising(void)
{
    send_frame('S', NULL, 0);
    return 0;
}

int ble_poll_event(ble_event_t *evt, uint32_t timeout_ms)
{
    if (!s_initialized || evt == NULL) return -1;
    memset(evt, 0, sizeof(*evt));

    /* Look for a framed event from the nRF52840:
     *   STX <evt_type> [payload...] ETX  */
    uint8_t b;
    if (!uart1_rx_byte(&b, timeout_ms)) return -2;
    if (b != BLE_FRAME_STX) return -3;

    if (!uart1_rx_byte(&b, 10)) return -4;
    evt->type = (ble_event_type_t)b;

    /* Read payload until ETX or maxlen. */
    uint8_t n = 0;
    while (n < sizeof(evt->data)) {
        if (!uart1_rx_byte(&b, 10)) break;
        if (b == BLE_FRAME_ETX) {
            evt->len = n;
            return 0;
        }
        evt->data[n++] = b;
    }
    evt->len = n;
    return 0;
}

int ble_send_status(const ble_status_t *st)
{
    if (!s_initialized || st == NULL) return -1;
    send_frame('R', (const uint8_t *)st, (int)sizeof(ble_status_t));
    return 0;
}

int ble_send_bulk_record(const void *rec)
{
    if (!s_initialized || rec == NULL) return -1;
    /* Frame: STX 'D' <64 bytes> ETX */
    uint8_t hdr[2] = { BLE_FRAME_STX, 'D' };
    uart1_tx(hdr, 2);
    uart1_tx((const uint8_t *)rec, 64);
    uint8_t etx = BLE_FRAME_ETX;
    uart1_tx(&etx, 1);
    return 0;
}

int ble_send_bulk_done(void)
{
    send_frame('E', NULL, 0);
    return 0;
}