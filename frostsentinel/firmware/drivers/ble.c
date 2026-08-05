/*
 * drivers/ble.c — CC2642R BLE 5.2 module UART control
 *
 * The CC2642R is a dedicated BLE controller running TI's BLE-Stack.
 * FrostSentinel communicates with it over USART2 (115200 8N1) using a
 * lightweight framing protocol.  The BLE module handles the GATT
 * server (custom service UUID f5b00001-...), connection management,
 * and OTA firmware transfer; the STM32U575 pushes live data via
 * notify characteristics and receives commands via write
 * characteristics.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#include "ble.h"
#include "../board.h"
#include <string.h>

/* ------------------------------------------------------------------ */
/*  UART configuration                                                 */
/* ------------------------------------------------------------------ */
#define BLE_BAUD         115200u
#define BLE_FRAME_SOF    0xA5
#define BLE_FRAME_EOF    0x5A
#define BLE_MAX_PAYLOAD  64

/* Frame types */
#define BLE_FRAME_LIVE_DATA    0x01   /* MCU → BLE: notify live sensor data */
#define BLE_FRAME_LOG_RECORD   0x02   /* MCU → BLE: notify historical record */
#define BLE_FRAME_COMMAND      0x03   /* BLE → MCU: command from app */
#define BLE_FRAME_PROVISION    0x04   /* BLE → MCU: provisioning data */
#define BLE_FRAME_OTA_CHUNK    0x05   /* BLE → MCU: OTA firmware chunk */
#define BLE_FRAME_OTA_DONE     0x06   /* BLE → MCU: OTA transfer complete */
#define BLE_FRAME_STATUS_REQ   0x07   /* BLE → MCU: app requests status */
#define BLE_FRAME_STATUS_RSP   0x08   /* MCU → BLE: status response */
#define BLE_FRAME_ACK          0x0F

/* ------------------------------------------------------------------ */
/*  State                                                              */
/* ------------------------------------------------------------------ */
static uint8_t  s_rx_buf[BLE_MAX_PAYLOAD + 8];
static uint8_t  s_rx_idx;
static uint8_t  s_rx_state;   /* 0=idle, 1=SOF seen, 2=in payload, 3=EOF */
static ble_command_cb_t s_command_cb;

/* ------------------------------------------------------------------ */
/*  UART init                                                          */
/* ------------------------------------------------------------------ */
static void ble_uart_init(void)
{
    /* Enable USART2 clock */
    RCC->APB1ENR1 |= RCC_APB1ENR1_USART2EN;

    /* PA2 = USART2_TX (AF7), PA3 = USART2_RX (AF7) */
    GPIO_CONFIG(GPIOA, 2, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_SPEED_HIGH,
                GPIO_PUPD_NONE, 7);
    GPIO_CONFIG(GPIOA, 3, GPIO_MODE_AF, 0, GPIO_SPEED_HIGH, GPIO_PUPD_UP, 7);

    /* Baud = 115200 at 160 MHz APB1: BRR = 160000000 / 115200 ≈ 1389 */
    USART2->BRR = 1389;
    USART2->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE;
}

static void ble_uart_tx_byte(uint8_t b)
{
    while (!(USART2->ISR & USART_ISR_TXE)) ;
    USART2->TDR = b;
}

static uint8_t ble_uart_rx_byte(uint32_t timeout_ms)
{
    uint32_t start = time_ms();
    while (!(USART2->ISR & USART_ISR_RXNE)) {
        if (elapsed_ms(start) > timeout_ms) return 0xFF;
    }
    return (uint8_t)USART2->RDR;
}

/* ------------------------------------------------------------------ */
/*  Frame send                                                         */
/* ------------------------------------------------------------------ */
static void ble_send_frame(uint8_t frame_type, const uint8_t *payload,
                           uint8_t len)
{
    if (len > BLE_MAX_PAYLOAD) len = BLE_MAX_PAYLOAD;

    ble_uart_tx_byte(BLE_FRAME_SOF);
    ble_uart_tx_byte(frame_type);
    ble_uart_tx_byte(len);
    for (uint8_t i = 0; i < len; i++) {
        ble_uart_tx_byte(payload[i]);
    }
    /* Simple checksum: XOR of type, len, and payload bytes */
    uint8_t cksum = frame_type ^ len;
    for (uint8_t i = 0; i < len; i++) cksum ^= payload[i];
    ble_uart_tx_byte(cksum);
    ble_uart_tx_byte(BLE_FRAME_EOF);
}

/* ------------------------------------------------------------------ */
/*  Frame receive (non-blocking, called from main loop)                */
/* ------------------------------------------------------------------ */
static int ble_recv_frame(uint8_t *frame_type, uint8_t *payload,
                          uint8_t *len, uint32_t timeout_ms)
{
    uint8_t byte;
    uint32_t start = time_ms();

    /* Wait for SOF */
    do {
        byte = ble_uart_rx_byte(timeout_ms);
        if (elapsed_ms(start) > timeout_ms) return -1;
    } while (byte != BLE_FRAME_SOF);

    *frame_type = ble_uart_rx_byte(10);
    *len = ble_uart_rx_byte(10);
    if (*len > BLE_MAX_PAYLOAD) return -1;

    for (uint8_t i = 0; i < *len; i++) {
        payload[i] = ble_uart_rx_byte(10);
    }

    uint8_t cksum_expected = ble_uart_rx_byte(10);
    uint8_t cksum = *frame_type ^ *len;
    for (uint8_t i = 0; i < *len; i++) cksum ^= payload[i];
    if (cksum != cksum_expected) return -2;

    uint8_t eof = ble_uart_rx_byte(10);
    if (eof != BLE_FRAME_EOF) return -3;

    return 0;
}

/* ------------------------------------------------------------------ */
/*  Public: initialize BLE module                                      */
/* ------------------------------------------------------------------ */
void ble_init(ble_command_cb_t command_callback)
{
    s_command_cb = command_callback;
    s_rx_idx = 0;
    s_rx_state = 0;

    /* Enable GPIOB clock for BLE reset pin */
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;

    /* PB8 = BLE reset (output, active low) */
    GPIO_CONFIG(GPIOB, 8, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP, GPIO_SPEED_HIGH,
                GPIO_PUPD_NONE, 0);

    /* Reset the BLE module */
    BLE_RESET_N();
    delay_ms(10);
    BLE_RESET_DEASSERT();
    delay_ms(50);   /* module boot time */

    ble_uart_init();
    delay_ms(10);
}

/* ------------------------------------------------------------------ */
/*  Public: send live sensor data as a BLE notification                */
/*  Payload format (16 bytes):                                        */
/*    0-1:  RFRI (Q8.8, uint16)                                       */
/*    2-3:  T_wet (0.01 °C, int16)                                    */
/*    4-5:  T_air (0.01 °C, int16)                                    */
/*    6-7:  T_sky (0.01 °C, int16)                                    */
/*    8-9:  ΔT_rad (0.01 °C, int16)                                   */
/*   10-11: leaf_wet (0-1000, uint16)                                 */
/*   12:   AE status (0/1/2)                                          */
/*   13:   flags                                                      */
/*   14:   battery %                                                  */
 *   15:   node_id                                                    */
/* ------------------------------------------------------------------ */
void ble_send_live_data(void)
{
    uint8_t payload[16];
    payload[0]  = g_sys.rfri_q8 & 0xFF;
    payload[1]  = (g_sys.rfri_q8 >> 8) & 0xFF;
    payload[2]  = g_sys.twet_cx100 & 0xFF;
    payload[3]  = (g_sys.twet_cx100 >> 8) & 0xFF;
    payload[4]  = g_sys.air_t_cx100 & 0xFF;
    payload[5]  = (g_sys.air_t_cx100 >> 8) & 0xFF;
    payload[6]  = g_sys.sky_t_cx100 & 0xFF;
    payload[7]  = (g_sys.sky_t_cx100 >> 8) & 0xFF;
    payload[8]  = g_sys.delta_rad_cx100 & 0xFF;
    payload[9]  = (g_sys.delta_rad_cx100 >> 8) & 0xFF;
    payload[10] = g_sys.leaf_wet & 0xFF;
    payload[11] = (g_sys.leaf_wet >> 8) & 0xFF;
    payload[12] = g_sys.ae_status;
    payload[13] = g_sys.flags;
    payload[14] = g_sys.battery_pct;
    payload[15] = g_sys.node_id;
    ble_send_frame(BLE_FRAME_LIVE_DATA, payload, 16);
}

/* ------------------------------------------------------------------ */
/*  Public: send a historical log record                               */
/* ------------------------------------------------------------------ */
void ble_send_log_record(const uint8_t *record24)
{
    ble_send_frame(BLE_FRAME_LOG_RECORD, record24, 24);
}

/* ------------------------------------------------------------------ */
/*  Public: send status response                                       */
/* ------------------------------------------------------------------ */
void ble_send_status(void)
{
    uint8_t payload[12];
    payload[0]  = g_sys.node_id;
    payload[1]  = g_sys.mesh_role;
    payload[2]  = g_sys.mesh_hops;
    payload[3]  = g_sys.sample_interval;
    payload[4]  = g_sys.battery_pct;
    payload[5]  = g_sys.battery_mv & 0xFF;
    payload[6]  = (g_sys.battery_mv >> 8) & 0xFF;
    payload[7]  = g_sys.flags;
    payload[8]  = (uint8_t)(g_sys.records_written & 0xFF);
    payload[9]  = (uint8_t)((g_sys.records_written >> 8) & 0xFF);
    payload[10] = (uint8_t)((g_sys.records_written >> 16) & 0xFF);
    payload[11] = (uint8_t)((g_sys.records_written >> 24) & 0xFF);
    ble_send_frame(BLE_FRAME_STATUS_RSP, payload, 12);
}

/* ------------------------------------------------------------------ */
/*  Public: poll for incoming commands (non-blocking)                  */
/*  Returns 1 if a command was received and dispatched, 0 otherwise.   */
/* ------------------------------------------------------------------ */
int ble_poll(void)
{
    uint8_t frame_type, len;
    uint8_t payload[BLE_MAX_PAYLOAD];

    /* Non-blocking: check if data is available */
    if (!(USART2->ISR & USART_ISR_RXNE)) return 0;

    if (ble_recv_frame(&frame_type, payload, &len, 5) < 0)
        return 0;

    switch (frame_type) {
    case BLE_FRAME_COMMAND:
        if (s_command_cb) s_command_cb(payload, len);
        /* Send ACK */
        ble_send_frame(BLE_FRAME_ACK, payload, 1);
        return 1;

    case BLE_FRAME_PROVISION:
        /* Provisioning: payload = [node_id, mesh_role, key[16]] */
        if (len >= 18 && s_command_cb) {
            s_command_cb(payload, len);
        }
        return 1;

    case BLE_FRAME_STATUS_REQ:
        ble_send_status();
        return 1;

    case BLE_FRAME_OTA_CHUNK:
        /* OTA: payload = [offset_msb, offset_lsb, data[up to 60]] */
        /* In production, this would write to flash and verify.
         * For now, ACK it. */
        ble_send_frame(BLE_FRAME_ACK, payload, 2);
        return 1;

    case BLE_FRAME_OTA_DONE:
        /* Trigger firmware swap and reboot */
        if (s_command_cb) s_command_cb(payload, len);
        return 1;

    default:
        return 0;
    }
}

/* ------------------------------------------------------------------ */
/*  Public: set the node's network key (from provisioning)             */
/* ------------------------------------------------------------------ */
void ble_set_network_key(const uint8_t *key16)
{
    /* Store in flash config area (in production) and update radio */
    /* For now, we pass it to the radio driver via radio_init re-call */
    radio_init(key16, g_sys.node_id, radio_get_tx_slot());
}