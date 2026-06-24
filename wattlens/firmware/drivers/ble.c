/*
 * ble.c — BLE communication via nRF52840 UART bridge
 * WattLens — 3-Phase Power Quality Analyzer & NILM
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 *
 * The nRF52840 module runs a UART-to-BLE bridge firmware.  The STM32H743
 * sends and receives framed binary messages over USART2.  The nRF52840
 * exposes GATT characteristics to the mobile app and translates between
 * BLE notify/write and UART frames.
 *
 * UART frame format:
 *   [0xAA] [LEN] [CMD] [PAYLOAD...] [CRC8] [0x55]
 *
 * GATT service: 0000FF10-1212-EF12-1234-56789ABCDEF0
 * Characteristics:
 *   FF11 — Notify — real-time metrics (1 Hz)
 *   FF12 — Read   — harmonic spectrum (on demand)
 *   FF13 — Notify — NILM state (0.5 Hz)
 *   FF14 — Notify — event notifications
 *   FF15 — Write  — command/control
 *   FF16 — Write  — NILM model upload (chunked)
 *   FF17 — Read   — device status
 */

#include "ble.h"
#include "registers.h"
#include "nilm.h"
#include "event_detect.h"
#include <string.h>

#define UART2_CR1   USART_REG(USART2_BASE, USART_CR1)
#define UART2_CR2   USART_REG(USART2_BASE, USART_CR2)
#define UART2_CR3   USART_REG(USART2_BASE, USART_CR3)
#define UART2_BRR   USART_REG(USART2_BASE, USART_BRR)
#define UART2_ISR   USART_REG(USART2_BASE, USART_ISR)
#define UART2_RDR   USART_REG(USART2_BASE, USART_RDR)
#define UART2_TDR   USART_REG(USART2_BASE, USART_TDR)

#define BLE_FRAME_SOF    0xAA
#define BLE_FRAME_EOF    0x55
#define BLE_MAX_PAYLOAD  128
#define BLE_RX_BUF_SIZE  256

/* CRC-8 (Dallas/Maxim polynomial) */
static uint8_t crc8(const uint8_t *data, int len) {
    uint8_t crc = 0;
    for (int i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x80) crc = (crc << 1) ^ 0x07;
            else crc <<= 1;
        }
    }
    return crc;
}

/* RX state machine */
static uint8_t rx_buf[BLE_RX_BUF_SIZE];
static int rx_idx = 0;
static bool rx_in_frame = false;
static uint8_t rx_len = 0;

/* TX helper */
static void uart2_putc(uint8_t c) {
    while (!(UART2_ISR & BIT(7))) { }  /* wait TXE */
    UART2_TDR = c;
}

static void uart2_write(const uint8_t *data, int len) {
    for (int i = 0; i < len; i++) uart2_putc(data[i]);
}

/* ========================================================================
 * Send a framed BLE message
 * ======================================================================== */
static void ble_send_frame(uint8_t cmd, const uint8_t *payload, int len) {
    uint8_t buf[BLE_MAX_PAYLOAD + 6];
    buf[0] = BLE_FRAME_SOF;
    buf[1] = (uint8_t)len;
    buf[2] = cmd;
    if (len > 0) memcpy(&buf[3], payload, len);
    buf[3 + len] = crc8(&buf[2], 1 + len);
    buf[3 + len + 1] = BLE_FRAME_EOF;
    uart2_write(buf, 5 + len);
}

/* ========================================================================
 * Initialize BLE UART
 * ======================================================================== */
void ble_init(void) {
    /* Configure USART2: 115200 baud, 8N1, enable TX/RX */
    UART2_CR1 = 0;  /* disable */
    UART2_CR2 = 0;  /* 1 stop bit */
    UART2_CR3 = 0;

    /* BRR = APB1_CLK / 115200 = 120 MHz / 115200 = 1041.67 → 1042 */
    UART2_BRR = 1042;

    /* Enable UART, TX, RX, RXNE interrupt */
    UART2_CR1 = BIT(0)   /* UE */
              | BIT(3)   /* TE */
              | BIT(2)   /* RE */
              | BIT(5);  /* RXNEIE */

    /* Enable USART2 interrupt in NVIC */
    NVIC_IP(38) = 6;
    NVIC_ISER(1) |= BIT(38 - 32);

    rx_idx = 0;
    rx_in_frame = false;
}

/* ========================================================================
 * USART2 interrupt handler (BLE RX)
 * ======================================================================== */
void USART2_IRQHandler(void) {
    if (UART2_ISR & BIT(5)) {  /* RXNE */
        uint8_t c = (uint8_t)UART2_RDR;

        if (!rx_in_frame) {
            if (c == BLE_FRAME_SOF) {
                rx_in_frame = true;
                rx_idx = 0;
            }
        } else {
            if (rx_idx < BLE_RX_BUF_SIZE) {
                rx_buf[rx_idx++] = c;
            }

            if (rx_idx == 2) {
                rx_len = rx_buf[0];
            }

            /* Frame complete: len + cmd + payload + crc + eof */
            if (rx_idx >= 2 + rx_len + 2) {
                if (rx_buf[rx_idx - 1] == BLE_FRAME_EOF) {
                    /* Validate CRC */
                    uint8_t crc = crc8(&rx_buf[1], 1 + rx_len);
                    if (crc == rx_buf[rx_idx - 2]) {
                        /* Frame is valid — process in ble_poll_commands */
                    }
                }
                rx_in_frame = false;
                rx_idx = 0;
            }
        }
    }
}

/* ========================================================================
 * Pack power metrics into binary payload (for FF11 notify)
 * Format: [P_total_real(4)] [Q(4)] [S(4)] [PF(2)] [freq(2)]
 *         [V1(2)] [V2(2)] [V3(2)] [I1(2)] [I2(2)] [I3(2)]
 *         [THD_V(1)] [THD_I(1)]
 * All floats → scaled integers (e.g., P in 0.1 W units)
 * ======================================================================== */
void ble_send_metrics(const power_metrics_t *m, const nilm_result_t *n) {
    uint8_t buf[40];
    int idx = 0;

    /* Total power (fixed-point 0.1 W) */
    int32_t p = (int32_t)(m->p_total_real * 10.0f);
    int32_t q = (int32_t)(m->p_total_reactive * 10.0f);
    int32_t s = (int32_t)(m->p_total_apparent * 10.0f);
    memcpy(&buf[idx], &p, 4); idx += 4;
    memcpy(&buf[idx], &q, 4); idx += 4;
    memcpy(&buf[idx], &s, 4); idx += 4;

    /* Power factor (fixed-point 0.001) */
    int16_t pf = (int16_t)(m->pf_total * 1000.0f);
    memcpy(&buf[idx], &pf, 2); idx += 2;

    /* Frequency (fixed-point 0.01 Hz) */
    int16_t freq = (int16_t)(m->freq * 100.0f);
    memcpy(&buf[idx], &freq, 2); idx += 2;

    /* Voltages (0.1 V) */
    for (int i = 0; i < 3; i++) {
        uint16_t v = (uint16_t)(m->v_rms[i] * 10.0f);
        memcpy(&buf[idx], &v, 2); idx += 2;
    }

    /* Currents (0.01 A) */
    for (int i = 0; i < 3; i++) {
        uint16_t c = (uint16_t)(m->i_rms[i] * 100.0f);
        memcpy(&buf[idx], &c, 2); idx += 2;
    }

    /* THD (0.1%) */
    buf[idx++] = (uint8_t)((m->thd_v[0] + m->thd_v[1] + m->thd_v[2]) / 3.0f * 10.0f);
    buf[idx++] = (uint8_t)((m->thd_i[0] + m->thd_i[1] + m->thd_i[2]) / 3.0f * 10.0f);

    ble_send_frame(0x11, buf, idx);

    /* Also send NILM state (FF13) */
    uint8_t nilm_buf[2 + NILM_MAX_CLASSES * 2];
    nilm_buf[0] = (uint8_t)nilm_get_num_classes();
    nilm_buf[1] = 0;  /* reserved */
    for (int c = 0; c < NILM_MAX_CLASSES; c++) {
        nilm_buf[2 + c * 2] = n->nilm_state[c];
        nilm_buf[2 + c * 2 + 1] = (uint8_t)(n->nilm_probs[c] * 255.0f);
    }
    ble_send_frame(0x13, nilm_buf, sizeof(nilm_buf));
}

/* ========================================================================
 * Send harmonic spectrum (FF12)
 * Format: [phase(1)] [50 × magnitude(2)] — 102 bytes total
 * ======================================================================== */
void ble_send_harmonics(const harmonic_data_t *h) {
    for (int phase = 0; phase < 3; phase++) {
        uint8_t buf[1 + HARMONIC_MAX_ORDER * 2];
        buf[0] = (uint8_t)phase;
        for (int n = 1; n <= HARMONIC_MAX_ORDER; n++) {
            uint16_t mag = (uint16_t)(h->v_mag[phase][n] * 100.0f);  /* 0.01 V units */
            buf[1 + (n - 1) * 2] = mag & 0xFF;
            buf[1 + (n - 1) * 2 + 1] = (mag >> 8) & 0xFF;
        }
        ble_send_frame(0x12, buf, sizeof(buf));
    }
}

/* ========================================================================
 * Send event notification (FF14)
 * ======================================================================== */
void ble_send_event(const pq_event_t *evt) {
    uint8_t buf[12];
    memcpy(&buf[0], &evt->timestamp, 4);
    buf[4] = (uint8_t)evt->type;
    buf[5] = evt->phase;
    memcpy(&buf[6], &evt->severity, 4);
    memcpy(&buf[10], &evt->duration_ms, 2);
    ble_send_frame(0x14, buf, sizeof(buf));
}

/* ========================================================================
 * Send device status (FF17)
 * ======================================================================== */
void ble_send_status(const device_ctx_t *ctx) {
    uint8_t buf[16];
    memcpy(&buf[0], &ctx->uptime_s, 4);
    buf[4] = ctx->battery_pct;
    buf[5] = ctx->sd_present ? 1 : 0;
    buf[6] = ctx->ble_connected ? 1 : 0;
    buf[7] = ctx->lora_joined ? 1 : 0;
    memcpy(&buf[8], &ctx->error_flags, 2);
    memcpy(&buf[10], &ctx->sample_count, 4);
    buf[14] = (uint8_t)nilm_get_num_classes();
    buf[15] = 0;  /* reserved */
    ble_send_frame(0x17, buf, sizeof(buf));
}

/* ========================================================================
 * Poll for and process BLE commands from the app
 * ======================================================================== */
void ble_poll_commands(device_ctx_t *ctx) {
    if (!rx_in_frame && rx_idx > 0) {
        /* A complete frame is in rx_buf (simplified — see ISR for full state machine) */
        uint8_t cmd = rx_buf[1];
        uint8_t *payload = &rx_buf[2];
        uint8_t len = rx_buf[0];

        switch (cmd) {
        case BLE_CMD_START:
            ctx->state = STATE_MEASURE;
            break;
        case BLE_CMD_STOP:
            ctx->state = STATE_IDLE;
            break;
        case BLE_CMD_SET_CT:
            if (len >= 5) {
                uint8_t ch = payload[0];
                float ratio;
                memcpy(&ratio, &payload[1], 4);
                if (ch < 4) {
                    ctx->cal.ct_ratio[ch] = ratio;
                }
            }
            break;
        case BLE_CMD_SET_GRID:
            if (len >= 1) {
                ctx->cal.grid_freq = payload[0];
            }
            break;
        case BLE_CMD_GET_STATUS:
            ble_send_status(ctx);
            break;
        case BLE_CMD_SET_NAME:
            if (len >= 2) {
                nilm_set_appliance_name(payload[0], (const char *)&payload[1]);
            }
            break;
        default:
            break;
        }

        rx_idx = 0;
    }
}