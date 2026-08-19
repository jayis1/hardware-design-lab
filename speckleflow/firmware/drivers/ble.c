/*
 * ble.c — nRF52840 BLE bridge driver for SpeckleFlow
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 *
 * The nRF52840 runs an independent BLE 5.2 stack. The STM32 communicates
 * with it over USART3 at 3 Mbps using a simple framed protocol:
 *
 *   Byte 0:  Start marker (0xA5)
 *   Byte 1:  Message type
 *   Byte 2-3: Payload length (little-endian)
 *   Byte 4..N: Payload
 *   Byte N+1: CRC-8 (poly 0x07, init 0x00)
 *
 * Message types:
 *   0x01 = Flow tile (128 bytes: 16×8 pixels, 8-bit)
 *   0x02 = Status (8 bytes: battery, laser, fps, temp, frame_cnt)
 *   0x03 = Command response (4 bytes)
 *   0x10 = Command from app (4 bytes, received via nRF52840)
 */

#include "ble.h"
#include "board.h"
#include "registers.h"
#include <string.h>

#define BLE_START_BYTE   0xA5
#define BLE_MSG_FLOW     0x01
#define BLE_MSG_STATUS   0x02
#define BLE_MSG_CMD_RESP 0x03
#define BLE_MSG_CMD_RECV 0x10

/* RX ring buffer for commands received from the app (via nRF52840) */
#define BLE_CMD_QUEUE_SIZE  16
static struct {
    uint8_t buf[BLE_CMD_QUEUE_SIZE][BLE_CMD_SIZE];
    uint8_t head;
    uint8_t tail;
} cmd_queue;

static volatile uint8_t ble_tx_busy = 0;

/* ---- CRC-8 -------------------------------------------------------------- */

static uint8_t crc8(const uint8_t *data, uint32_t len) {
    uint8_t crc = 0;
    for (uint32_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++) {
            if (crc & 0x80) crc = (uint8_t)((crc << 1) ^ 0x07);
            else            crc = (uint8_t)(crc << 1);
        }
    }
    return crc;
}

/* ---- UART primitives ---------------------------------------------------- */

static void uart3_wait_tx(void) {
    while (!(USART3->ISR & USART_ISR_TXE)) { }
}

static void uart3_write(const uint8_t *data, uint32_t len) {
    for (uint32_t i = 0; i < len; i++) {
        uart3_wait_tx();
        USART3->TDR = data[i];
    }
}

static void uart3_wait_tc(void) {
    while (!(USART3->ISR & USART_ISR_TC)) { }
}

/* ---- Frame send --------------------------------------------------------- */

static void ble_send_frame(uint8_t msg_type, const uint8_t *payload,
                           uint16_t len) {
    uint8_t header[4];
    header[0] = BLE_START_BYTE;
    header[1] = msg_type;
    header[2] = (uint8_t)(len & 0xFF);
    header[3] = (uint8_t)(len >> 8);

    uint8_t crc = crc8(payload, len);

    ble_tx_busy = 1;
    uart3_write(header, 4);
    if (len > 0) uart3_write(payload, len);
    uart3_wait_tx();
    USART3->TDR = crc;
    uart3_wait_tc();
    ble_tx_busy = 0;
}

/* ---- RX state machine --------------------------------------------------- */

static struct {
    enum { RX_IDLE, RX_TYPE, RX_LEN_LO, RX_LEN_HI, RX_PAYLOAD, RX_CRC } state;
    uint8_t  msg_type;
    uint16_t payload_len;
    uint16_t payload_idx;
    uint8_t  payload[256];
    uint8_t  crc;
} rx;

static void ble_rx_byte(uint8_t b) {
    switch (rx.state) {
    case RX_IDLE:
        if (b == BLE_START_BYTE) rx.state = RX_TYPE;
        break;
    case RX_TYPE:
        rx.msg_type = b;
        rx.state = RX_LEN_LO;
        break;
    case RX_LEN_LO:
        rx.payload_len = b;
        rx.state = RX_LEN_HI;
        break;
    case RX_LEN_HI:
        rx.payload_len |= (uint16_t)(b << 8);
        rx.payload_idx = 0;
        if (rx.payload_len == 0) rx.state = RX_CRC;
        else if (rx.payload_len > sizeof(rx.payload)) rx.state = RX_IDLE;
        else rx.state = RX_PAYLOAD;
        break;
    case RX_PAYLOAD:
        rx.payload[rx.payload_idx++] = b;
        if (rx.payload_idx >= rx.payload_len) rx.state = RX_CRC;
        break;
    case RX_CRC:
        rx.crc = b;
        /* Verify CRC */
        if (crc8(rx.payload, rx.payload_len) == rx.crc) {
            /* Valid frame — handle by type */
            if (rx.msg_type == BLE_MSG_CMD_RECV && rx.payload_len == BLE_CMD_SIZE) {
                /* Enqueue command */
                uint8_t next = (cmd_queue.head + 1) % BLE_CMD_QUEUE_SIZE;
                if (next != cmd_queue.tail) {
                    memcpy(cmd_queue.buf[cmd_queue.head], rx.payload, BLE_CMD_SIZE);
                    cmd_queue.head = next;
                }
            }
        }
        rx.state = RX_IDLE;
        break;
    }
}

/* ---- Public API --------------------------------------------------------- */

int ble_init(void) {
    memset(&cmd_queue, 0, sizeof(cmd_queue));
    memset(&rx, 0, sizeof(rx));

    /* Reset nRF52840 */
    BLE_RST_PORT->BSRR = (1u << (BLE_RST_PIN + 16));  /* low */
    for (volatile int i = 0; i < 100000; i++) { }
    BLE_RST_PORT->BSRR = (1u << BLE_RST_PIN);          /* high */
    for (volatile int i = 0; i < 500000; i++) { }

    /* Enable RX interrupt */
    USART3->CR1 |= USART_CR1_RXNEIE;

    return 0;
}

void ble_send_flow_tile(const uint8_t *tile, uint16_t tile_idx) {
    /* Prepend tile index (2 bytes) to the 128-byte tile data */
    uint8_t payload[BLE_TILE_BYTES + 2];
    payload[0] = (uint8_t)(tile_idx & 0xFF);
    payload[1] = (uint8_t)(tile_idx >> 8);
    memcpy(&payload[2], tile, BLE_TILE_BYTES);
    ble_send_frame(BLE_MSG_FLOW, payload, sizeof(payload));
}

void ble_send_status(uint8_t battery_pct, uint8_t laser_on, uint8_t fps,
                     int8_t temp_c, uint32_t frame_count) {
    uint8_t payload[BLE_STATUS_SIZE + 4];
    payload[0] = battery_pct;
    payload[1] = laser_on;
    payload[2] = fps;
    payload[3] = (uint8_t)temp_c;
    payload[4] = (uint8_t)(frame_count & 0xFF);
    payload[5] = (uint8_t)((frame_count >> 8) & 0xFF);
    payload[6] = (uint8_t)((frame_count >> 16) & 0xFF);
    payload[7] = (uint8_t)((frame_count >> 24) & 0xFF);
    ble_send_frame(BLE_MSG_STATUS, payload, 8);
}

int ble_get_command(uint8_t *cmd) {
    if (cmd_queue.head == cmd_queue.tail) return -1;  /* empty */
    memcpy(cmd, cmd_queue.buf[cmd_queue.tail], BLE_CMD_SIZE);
    cmd_queue.tail = (cmd_queue.tail + 1) % BLE_CMD_QUEUE_SIZE;
    return 0;
}

void ble_isr_rx(void) {
    while (USART3->ISR & USART_ISR_RXNE) {
        uint8_t b = (uint8_t)(USART3->RDR & 0xFF);
        ble_rx_byte(b);
    }
}

int ble_is_tx_busy(void) {
    return ble_tx_busy;
}

/* USART3 RX interrupt handler */
void USART3_IRQHandler(void) {
    if (USART3->ISR & USART_ISR_RXNE) {
        ble_isr_rx();
    }
    if (USART3->ISR & (1u << 1)) {  /* framing error */
        USART3->ICR = 0xFFFFFFFF;
    }
}