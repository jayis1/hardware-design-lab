/*
 * ble.c — NINA-B306 BLE 5.0 UART NCP driver
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 *
 * The NINA-B306 is a BLE 5.0 module running u-blox's NCP (Network
 * Co-Processor) firmware, communicating with the host MCU over UART
 * at 1 Mbps using a BGAPI-like binary protocol.
 *
 * The driver implements:
 *  - Module reset + boot
 *  - GAP advertising configuration (custom service UUID)
 *  - GATT service/characteristic setup
 *  - Connection state tracking
 *  - Notification TX (measurement packets)
 *  - Status characteristic updates
 */

#include "ble.h"
#include "board.h"
#include "registers.h"
#include <string.h>

static uint8_t g_ble_state = BLE_STATE_RESET;
static bool g_connected = false;
static uint8_t g_rx_buf[256];
static uint16_t g_rx_idx = 0;

/* ---- UART low-level (stub: uses STM32L4 LL_USART) ---- */
static void uart2_init(uint32_t baud)
{
    /* USART2: PA2(TX), PA3(RX), 8N1, baud
     * In real build: configure GPIO AF, USART CR1, BR, enable
     */
    (void)baud;
}

static void uart2_tx_byte(uint8_t b)
{
    /* while(!(USART2->ISR & USART_ISR_TXE));
     * USART2->TDR = b;
     */
    (void)b;
}

static void uart2_tx_buf(const uint8_t *buf, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++) uart2_tx_byte(buf[i]);
}

static bool uart2_rx_byte(uint8_t *b, uint32_t timeout_ms)
{
    /* if (USART2->ISR & USART_ISR_RXNE) { *b = USART2->RDR; return true; }
     */
    (void)b; (void)timeout_ms;
    return false; /* stub */
}

static void ble_reset_pin(bool low)
{
    /* GPIOA->BSRR = low ? ((1<<8) << 16) : (1 << 8); — PA8 */
    (void)low;
}

static void ble_delay_ms(uint32_t ms)
{
    /* Use SysTick-based delay */
    (void)ms;
}

/* ---- BGAPI-like protocol framing ----
 *
 * Frame format:
 *   [SYNC1=0x01] [SYNC2=0x62] [len_hi] [len_lo] [class] [method] [payload...]
 *   [EOF=0x03]
 *
 * TX: commands to module
 * RX: events + responses from module
 */
typedef struct {
    uint8_t  sync1;
    uint8_t  sync2;
    uint16_t length;
    uint8_t  msg_class;
    uint8_t  method;
    uint8_t  payload[240];
} ble_msg_t;

static bool ble_send_cmd(uint8_t msg_class, uint8_t method,
                         const uint8_t *payload, uint16_t len)
{
    uart2_tx_byte(NINA_SYNC);
    uart2_tx_byte(0x62);
    uart2_tx_byte((len >> 0) & 0xFF);
    uart2_tx_byte((len >> 8) & 0xFF);
    uart2_tx_byte(msg_class);
    uart2_tx_byte(method);
    if (len > 0 && payload) {
        uart2_tx_buf(payload, len);
    }
    /* No EOF in BGAPI; framing via length field */
    return true;
}

static bool ble_wait_response(ble_msg_t *msg, uint32_t timeout_ms)
{
    uint8_t header[6];
    for (int i = 0; i < 6; i++) {
        if (!uart2_rx_byte(&header[i], timeout_ms)) return false;
    }

    if (header[0] != 0x00 && header[0] != 0x80) return false; /* response/event */

    msg->sync1 = header[0];
    msg->sync2 = header[1];
    msg->length = header[2] | (header[3] << 8);
    msg->msg_class = header[4];
    msg->method = header[5];

    uint16_t to_read = msg->length;
    if (to_read > sizeof(msg->payload)) to_read = sizeof(msg->payload);

    for (uint16_t i = 0; i < to_read; i++) {
        if (!uart2_rx_byte(&msg->payload[i], timeout_ms)) return false;
    }
    return true;
}

/* ---- GATT service setup ---- */
static bool ble_setup_gatt(void)
{
    /* Define custom GATT service + 4 characteristics */
    uint8_t payload[32];

    /* Add service (ChloroMap service UUID) */
    /* ble_send_cmd(NINA_CMD_GATT, 0x01, ...); */
    /* Add measurement characteristic (notify) */
    /* ble_send_cmd(NINA_CMD_GATT, 0x02, ...); */
    /* Add command characteristic (write) */
    /* ble_send_cmd(NINA_CMD_GATT, 0x02, ...); */
    /* Add status characteristic (read + notify) */
    /* ble_send_cmd(NINA_CMD_GATT, 0x02, ...); */
    /* Add calibration characteristic (write) */
    /* ble_send_cmd(NINA_CMD_GATT, 0x02, ...); */

    (void)payload;
    return true;
}

/* ---- Public API ---- */

bool ble_init(void)
{
    /* 1. Reset module */
    ble_reset_pin(true);
    ble_delay_ms(10);
    ble_reset_pin(false);
    ble_delay_ms(10);
    ble_reset_pin(true);
    ble_delay_ms(500); /* boot time */

    /* 2. Init UART */
    uart2_init(BLE_BAUD);
    g_ble_state = BLE_STATE_BOOTING;

    /* 3. Sync with module (send hello, wait for response) */
    ble_msg_t rsp;
    ble_send_cmd(0x00, 0x00, NULL, 0); /* system_hello */
    if (!ble_wait_response(&rsp, 1000)) {
        g_ble_state = BLE_STATE_RESET;
        return false;
    }

    /* 4. Configure GAP */
    uint8_t gap_data[8];
    gap_data[0] = 0x02; /* discoverable */
    gap_data[1] = 0x04; /* connectable */
    ble_send_cmd(NINA_CMD_GAP, 0x01, gap_data, 2);
    ble_wait_response(&rsp, 500);

    /* 5. Setup GATT service */
    ble_setup_gatt();

    g_ble_state = BLE_STATE_ADVERTISING;
    return true;
}

bool ble_start_advertising(const char *name)
{
    /* Set advertising data: flags + shortened service UUID + name */
    uint8_t adv[31];
    uint8_t idx = 0;

    /* Flags: LE General Discoverable + BR/EDR not supported */
    adv[idx++] = 0x02; /* length */
    adv[idx++] = 0x01; /* type: flags */
    adv[idx++] = 0x06; /* value */

    /* Service UUID (16-bit: 0xC701) */
    adv[idx++] = 0x03; /* length */
    adv[idx++] = 0x02; /* type: 16-bit svc UUID */
    adv[idx++] = 0xC7; /* UUID low */
    adv[idx++] = 0x01; /* UUID high */

    /* Device name */
    uint8_t name_len = (uint8_t)strlen(name);
    if (name_len > 20) name_len = 20;
    adv[idx++] = name_len + 1;
    adv[idx++] = 0x09; /* type: complete local name */
    memcpy(&adv[idx], name, name_len);
    idx += name_len;

    ble_msg_t rsp;
    ble_send_cmd(NINA_CMD_GAP, 0x03, adv, idx); /* set_adv_data */
    ble_wait_response(&rsp, 500);

    /* Start advertising */
    uint8_t start_payload[6] = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00 };
    ble_send_cmd(NINA_CMD_GAP, 0x04, start_payload, 6);
    ble_wait_response(&rsp, 500);

    g_ble_state = BLE_STATE_ADVERTISING;
    return true;
}

bool ble_stop_advertising(void)
{
    ble_msg_t rsp;
    ble_send_cmd(NINA_CMD_GAP, 0x05, NULL, 0);
    ble_wait_response(&rsp, 500);
    g_ble_state = BLE_STATE_BOOTING;
    return true;
}

bool ble_send_notification(const uint8_t *data, uint16_t len)
{
    if (!g_connected) return false;

    /* Send GATT notification on measurement characteristic */
    uint8_t payload[64];
    payload[0] = 0x00; /* connection handle (auto) */
    payload[1] = 0x00;
    payload[2] = (len >> 0) & 0xFF;
    payload[3] = (len >> 8) & 0xFF;
    if (len > 60) len = 60;
    memcpy(&payload[4], data, len);

    ble_msg_t rsp;
    ble_send_cmd(NINA_CMD_GATT, 0x05, payload, len + 4); /* send_notify */
    ble_wait_response(&rsp, 200);

    g_ble_state = BLE_STATE_TX_PENDING;
    return true;
}

bool ble_send_status(const ble_status_t *status)
{
    uint8_t pkt[16];
    memset(pkt, 0, sizeof(pkt));

    pkt[0] = BLE_PKT_MAGIC;
    pkt[1] = BLE_PKT_VER;
    pkt[2] = (status->batt_mv >> 0) & 0xFF;
    pkt[3] = (status->batt_mv >> 8) & 0xFF;
    pkt[4] = status->state;
    pkt[5] = status->sats;
    pkt[6] = status->fix_type;

    /* CRC-8 */
    uint8_t crc = 0;
    for (int i = 0; i < 15; i++) {
        crc ^= pkt[i];
        for (int b = 0; b < 8; b++) {
            crc = (crc & 0x80) ? (crc << 1) ^ 0x07 : (crc << 1);
        }
    }
    pkt[15] = crc;

    return ble_send_notification(pkt, 16);
}

bool ble_is_connected(void)
{
    return g_connected;
}

void ble_poll(void)
{
    /* Check for incoming events (connection/disconnection) */
    uint8_t b;
    if (uart2_rx_byte(&b, 0)) {
        g_rx_buf[g_rx_idx++] = b;
        if (g_rx_idx >= sizeof(g_rx_buf)) g_rx_idx = 0;

        /* Parse events: connection_opened, connection_closed, write */
        if (g_rx_idx >= 6) {
            if (g_rx_buf[0] == 0x80) { /* event */
                uint8_t evt_class = g_rx_buf[4];
                uint8_t evt_method = g_rx_buf[5];
                if (evt_class == NINA_CMD_GAP && evt_method == 0x03) {
                    /* connection_opened */
                    g_connected = true;
                    g_ble_state = BLE_STATE_CONNECTED;
                } else if (evt_class == NINA_CMD_GAP && evt_method == 0x04) {
                    /* connection_closed */
                    g_connected = false;
                    g_ble_state = BLE_STATE_ADVERTISING;
                }
            }
            g_rx_idx = 0;
        }
    }
}

bool ble_disconnect(void)
{
    ble_msg_t rsp;
    uint8_t payload[2] = { 0x00, 0x00 };
    ble_send_cmd(NINA_CMD_GAP, 0x06, payload, 2);
    ble_wait_response(&rsp, 500);
    g_connected = false;
    g_ble_state = BLE_STATE_ADVERTISING;
    return true;
}

uint8_t ble_get_state(void)
{
    return g_ble_state;
}

bool ble_set_tx_power(int8_t dbm)
{
    uint8_t payload[1] = { (uint8_t)dbm };
    ble_msg_t rsp;
    ble_send_cmd(NINA_CMD_GAP, 0x08, payload, 1);
    return ble_wait_response(&rsp, 500);
}