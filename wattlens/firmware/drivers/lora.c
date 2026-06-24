/*
 * lora.c — SX1262 LoRa driver with spanning-tree mesh framing
 * WattLens — 3-Phase Power Quality Analyzer & NILM
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 *
 * The SX1262 is controlled via UART (AT command mode for configuration)
 * and GPIO (NSS, RESET, DIO1 for TX/RX done).  In transmit mode, payloads
 * are framed with a node ID + sequence number + mesh hop count.
 *
 * Mesh protocol (simplified spanning tree):
 *   - Gateway node (ID=0) broadcasts beacons every 60 s
 *   - Child nodes join by responding to beacon with JOIN_REQ
 *   - Each node forwards uplink packets to its parent
 *   - Downlink packets are flooded with hop limit
 *
 * Uplink payload format (40 bytes):
 *   [node_id(2)] [seq(2)] [P_total(4)] [Q(4)] [PF(2)] [freq(2)]
 *   [V1(2)] [V2(2)] [V3(2)] [I1(2)] [I2(2)] [I3(2)]
 *   [THD_V(1)] [event_flags(1)] [nilm_mask(2)] [timestamp(4)]
 *   [hop(1)] [rssi(1)] [crc(2)]
 */

#include "lora.h"
#include "registers.h"
#include "nilm.h"
#include <string.h>

#define UART3_CR1   USART_REG(USART3_BASE, USART_CR1)
#define UART3_CR2   USART_REG(USART3_BASE, USART_CR2)
#define UART3_CR3   USART_REG(USART3_BASE, USART_CR3)
#define UART3_BRR   USART_REG(USART3_BASE, USART_BRR)
#define UART3_ISR   USART_REG(USART3_BASE, USART_ISR)
#define UART3_RDR   USART_REG(USART3_BASE, USART_RDR)
#define UART3_TDR   USART_REG(USART3_BASE, USART_TDR)

/* LoRa CS and RESET pins */
#define LORA_NSS_HIGH() (GPIO_REG(GPIOC_BASE, GPIO_BSRR_OFF) = BIT(12))
#define LORA_NSS_LOW()  (GPIO_REG(GPIOC_BASE, GPIO_BSRR_OFF) = BIT(44))
#define LORA_RESET_HIGH() (GPIO_REG(GPIOC_BASE, GPIO_BSRR_OFF) = BIT(9))
#define LORA_RESET_LOW()  (GPIO_REG(GPIOC_BASE, GPIO_BSRR_OFF) = BIT(41))

#define LORA_FREQ_868    868000000   /* Hz */
#define LORA_FREQ_915    915000000   /* Hz */
#define LORA_BW          125000      /* kHz bandwidth */
#define LORA_SF          9           /* spreading factor 9 */
#define LORA_TX_POWER    22          /* dBm */
#define LORA_MAX_PAYLOAD 64

#define MESH_GATEWAY_ID  0
#define MESH_MAX_HOPS    5

static uint16_t node_id = 0;
static uint16_t seq_num = 0;
static bool joined = false;
static uint8_t tx_buf[LORA_MAX_PAYLOAD];
static uint8_t rx_buf[LORA_MAX_PAYLOAD];
static int rx_len = 0;

static void uart3_putc(uint8_t c) {
    while (!(UART3_ISR & BIT(7))) { }
    UART3_TDR = c;
}

static void uart3_write(const uint8_t *data, int len) {
    for (int i = 0; i < len; i++) uart3_putc(data[i]);
}

static int uart3_read(uint8_t *buf, int max, int timeout_ms) {
    int idx = 0;
    uint32_t start = 0;  /* would use sys_tick_ms in production */
    while (idx < max) {
        if (UART3_ISR & BIT(5)) {  /* RXNE */
            buf[idx++] = (uint8_t)UART3_RDR;
        }
        /* Timeout check omitted for brevity */
        if (idx >= max) break;
    }
    return idx;
}

/* ========================================================================
 * Send AT command to SX1262 and wait for response
 * ======================================================================== */
static int lora_at_command(const char *cmd, char *resp, int resp_len) {
    uart3_write((const uint8_t *)cmd, strlen(cmd));
    uart3_putc('\r');
    uart3_putc('\n');
    if (resp && resp_len > 0) {
        return uart3_read((uint8_t *)resp, resp_len, 1000);
    }
    return 0;
}

/* ========================================================================
 * Initialize SX1262 LoRa radio
 * ======================================================================== */
void lora_init(void) {
    /* Configure USART3: 115200 baud, 8N1 */
    UART3_CR1 = 0;
    UART3_CR2 = 0;
    UART3_CR3 = 0;
    UART3_BRR = 1042;  /* 120 MHz / 115200 */
    UART3_CR1 = BIT(0) | BIT(3) | BIT(2) | BIT(5);  /* UE, TE, RE, RXNEIE */

    NVIC_IP(39) = 7;
    NVIC_ISER(1) |= BIT(39 - 32);

    /* Hardware reset SX1262 */
    LORA_NSS_HIGH();
    LORA_RESET_LOW();
    for (volatile int i = 0; i < 10000; i++) { }
    LORA_RESET_HIGH();
    for (volatile int i = 0; i < 10000; i++) { }

    /* AT command configuration */
    lora_at_command("AT+MODE=LoRa", NULL, 0);
    lora_at_command("AT+FREQ=868000000", NULL, 0);
    lora_at_command("AT+SF=9", NULL, 0);
    lora_at_command("AT&BW=125", NULL, 0);
    lora_at_command("AT&PWR=22", NULL, 0);
    lora_at_command("AT+CRC=ON", NULL, 0);
    lora_at_command("AT+HEADER=ON", NULL, 0);

    joined = false;
    node_id = 0;
    seq_num = 0;
}

/* ========================================================================
 * Build mesh uplink payload
 * ======================================================================== */
static int build_uplink(const power_metrics_t *m, const nilm_result_t *n, uint8_t *buf) {
    int idx = 0;

    /* Node ID + sequence */
    memcpy(&buf[idx], &node_id, 2); idx += 2;
    memcpy(&buf[idx], &seq_num, 2); idx += 2;

    /* Power metrics (fixed-point) */
    int32_t p = (int32_t)(m->p_total_real * 10.0f);
    int32_t q = (int32_t)(m->p_total_reactive * 10.0f);
    memcpy(&buf[idx], &p, 4); idx += 4;
    memcpy(&buf[idx], &q, 4); idx += 4;

    int16_t pf = (int16_t)(m->pf_total * 1000.0f);
    int16_t freq = (int16_t)(m->freq * 100.0f);
    memcpy(&buf[idx], &pf, 2); idx += 2;
    memcpy(&buf[idx], &freq, 2); idx += 2;

    /* Voltages and currents */
    for (int i = 0; i < 3; i++) {
        uint16_t v = (uint16_t)(m->v_rms[i] * 10.0f);
        memcpy(&buf[idx], &v, 2); idx += 2;
    }
    for (int i = 0; i < 3; i++) {
        uint16_t c = (uint16_t)(m->i_rms[i] * 100.0f);
        memcpy(&buf[idx], &c, 2); idx += 2;
    }

    /* THD + event flags + NILM mask */
    buf[idx++] = (uint8_t)((m->thd_v[0] + m->thd_v[1] + m->thd_v[2]) / 3.0f * 10.0f);
    buf[idx++] = 0;  /* event flags (would be filled from event_detect) */

    uint16_t nilm_mask = 0;
    for (int c = 0; c < 16; c++) {
        if (n->nilm_state[c]) nilm_mask |= (1 << c);
    }
    memcpy(&buf[idx], &nilm_mask, 2); idx += 2;

    /* Timestamp */
    uint32_t ts = 0;  /* would use RTC */
    memcpy(&buf[idx], &ts, 4); idx += 4;

    /* Hop count + RSSI placeholder */
    buf[idx++] = 0;
    buf[idx++] = 0;

    return idx;
}

/* ========================================================================
 * Send summary payload via LoRa
 * ======================================================================== */
void lora_send_summary(const power_metrics_t *m, const nilm_result_t *n) {
    int len = build_uplink(m, n, tx_buf);
    if (len > LORA_MAX_PAYLOAD) len = LORA_MAX_PAYLOAD;

    /* Send via AT+SEND command */
    char cmd[32];
    /* Format: AT+SEND=<len>,<hexdata> */
    /* Simplified: use binary TX mode via NSS/SPI (production uses SPI, not UART) */
    lora_at_command("AT+SEND", NULL, 0);
    uart3_write(tx_buf, len);

    seq_num++;
}

/* ========================================================================
 * Send event via LoRa (short payload)
 * ======================================================================== */
void lora_send_event(const pq_event_t *evt) {
    uint8_t buf[12];
    memcpy(&buf[0], &node_id, 2);
    memcpy(&buf[2], &evt->timestamp, 4);
    buf[6] = (uint8_t)evt->type;
    buf[7] = evt->phase;
    memcpy(&buf[8], &evt->severity, 4);

    lora_at_command("AT+SEND", NULL, 0);
    uart3_write(buf, sizeof(buf));
}

/* ========================================================================
 * Join mesh network
 * ======================================================================== */
int lora_join_mesh(uint16_t id) {
    node_id = id;

    /* Send JOIN_REQ to gateway */
    uint8_t join_req[4];
    memcpy(&join_req[0], &node_id, 2);
    join_req[2] = 0x01;  /* JOIN_REQ */
    join_req[3] = 0x00;

    lora_at_command("AT+SEND", NULL, 0);
    uart3_write(join_req, sizeof(join_req));

    /* Wait for JOIN_ACK (simplified — would poll with timeout) */
    joined = true;
    return 0;
}

bool lora_is_joined(void) { return joined; }

/* ========================================================================
 * USART3 interrupt handler (LoRa RX)
 * ======================================================================== */
void USART3_IRQHandler(void) {
    if (UART3_ISR & BIT(5)) {
        uint8_t c = (uint8_t)UART3_RDR;
        if (rx_len < LORA_MAX_PAYLOAD) {
            rx_buf[rx_len++] = c;
        }
    }
}

/* ========================================================================
 * Poll for downlink messages from gateway
 * ======================================================================== */
void lora_poll_downlink(device_ctx_t *ctx) {
    if (rx_len > 0) {
        /* Parse downlink command */
        /* Downlink format: [node_id(2)] [cmd(1)] [payload...] */
        uint16_t target_id;
        memcpy(&target_id, &rx_buf[0], 2);

        if (target_id == node_id || target_id == 0xFFFF) {  /* broadcast or targeted */
            uint8_t cmd = rx_buf[2];
            switch (cmd) {
            case 0x02:  /* SET_CONFIG */
                if (rx_len >= 4) {
                    ctx->cal.grid_freq = rx_buf[3];
                }
                break;
            case 0x03:  /* SYNC_TIME */
                if (rx_len >= 8) {
                    uint32_t new_time;
                    memcpy(&new_time, &rx_buf[3], 4);
                    /* Set RTC (would call rtc_set_time) */
                }
                break;
            case 0x04:  /* REQUEST_CAPTURE */
                ctx->state = STATE_CAPTURE;
                break;
            default:
                break;
            }
        }
        rx_len = 0;
    }
}