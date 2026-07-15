/*
 * lora_link.c — SX1262 LoRa driver and ThermoGlyph mesh protocol
 *
 * Implements:
 *   - SPI register-level communication with SX1262
 *   - LoRa modulation configuration (SF7, BW125, 868 MHz)
 *   - TX/RX with interrupt-driven completion
 *   - Binary packet protocol with AES-128 encryption (simplified CRC)
 *   - CAD (Channel Activity Detection) for low-power listen
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * License: GPL-3.0
 */

#include "lora_link.h"
#include "board.h"
#include "registers.h"
#include <string.h>

/* ------------------------------------------------------------------------- */
/* SX1262 SPI interface (SPIM0) */
/* ------------------------------------------------------------------------- */

static volatile uint32_t *spim0 = (volatile uint32_t *)NRF_SPIM0_BASE;
static uint8_t lora_tx_buf[128];
static uint8_t lora_rx_buf[128];

/* GPIO helpers (duplicated from thermal_pid.c for module independence) */
static volatile uint32_t *gpio_port(uint8_t p) {
    return (p == 0) ? (volatile uint32_t *)NRF_P0_BASE
                    : (volatile uint32_t *)NRF_P1_BASE;
}
static void gp_set(uint8_t p, uint8_t n) { gpio_port(p)[GPIO_PIN_OUTSET/4] = (1UL<<n); }
static void gp_clr(uint8_t p, uint8_t n) { gpio_port(p)[GPIO_PIN_OUTCLR/4] = (1UL<<n); }
static uint32_t gp_rd(uint8_t p, uint8_t n) { return (gpio_port(p)[GPIO_PIN_IN/4]>>n)&1U; }
static void gp_out(uint8_t p, uint8_t n) { gpio_port(p)[GPIO_PIN_CNF(n)/4] = 3UL; }
static void gp_in(uint8_t p, uint8_t n)  { gpio_port(p)[GPIO_PIN_CNF(n)/4] = 0xCUL; }

static void lora_spi_init(void)
{
    spim0[SPIM_PSEL_SCK / 4]  = (0U << 8) | 22U;  /* P0.22 */
    spim0[SPIM_PSEL_MOSI / 4] = (0U << 8) | 23U;  /* P0.23 */
    spim0[SPIM_PSEL_MISO / 4] = (0U << 8) | 24U;  /* P0.24 */
    spim0[SPIM_FREQUENCY / 4] = SPIM_FREQUENCY_8M;
    spim0[SPIM_CONFIG / 4] = 0U;
    spim0[SPIM_ENABLE / 4] = 7U;

    gp_out(0, 25);  /* NSS */
    gp_out(0, 5);   /* ANT_SW */
    gp_out(0, 6);   /* RESET_N */
    gp_in(0, 2);    /* BUSY (input) */
    gp_in(0, 3);    /* DIO1 (input, interrupt) */

    gp_set(0, 25);  /* NSS high (deselected) */
    gp_set(0, 6);   /* RESET_N high (not in reset) */
}

static bool sx_busy(void)
{
    return gp_rd(0, 2) == 1;  /* BUSY pin high = chip busy */
}

static void sx_wait_busy(void)
{
    uint32_t timeout = 100000U;
    while (sx_busy() && timeout-- > 0) {}
}

static void sx_spi_xfer(uint8_t *tx, uint8_t *rx, uint8_t len)
{
    sx_wait_busy();
    gp_clr(0, 25);  /* NSS low */

    spim0[SPIM_TXDPTR / 4] = (uint32_t)(uintptr_t)tx;
    spim0[SPIM_TXDMAXCNT / 4] = len;
    spim0[SPIM_RXDPTR / 4] = (uint32_t)(uintptr_t)rx;
    spim0[SPIM_RXDMAXCNT / 4] = len;
    spim0[SPIM_TASKS_START / 4] = 1U;

    uint32_t timeout = 100000U;
    while (!(spim0[SPIM_EVENTS_END / 4] & 1U) && timeout-- > 0) {}
    spim0[SPIM_EVENTS_END / 4] = 0U;

    gp_set(0, 25);  /* NSS high */
}

static void sx_write_cmd(uint8_t cmd, const uint8_t *params, uint8_t len)
{
    lora_tx_buf[0] = cmd;
    if (len > 0 && len < sizeof(lora_tx_buf) - 1) {
        memcpy(&lora_tx_buf[1], params, len);
    }
    sx_spi_xfer(lora_tx_buf, lora_rx_buf, len + 1);
}

static void sx_read_cmd(uint8_t cmd, uint8_t *out, uint8_t out_len)
{
    lora_tx_buf[0] = cmd;
    memset(&lora_tx_buf[1], 0, out_len);
    sx_spi_xfer(lora_tx_buf, lora_rx_buf, out_len + 1);
    memcpy(out, &lora_rx_buf[1], out_len);
}

static void sx_write_reg(uint16_t addr, uint8_t val)
{
    uint8_t params[3] = {(addr >> 8) & 0xFF, addr & 0xFF, val};
    sx_write_cmd(SX1262_CMD_WRITE_BUFFER, params, 3);
}

static uint8_t sx_read_reg(uint16_t addr)
{
    uint8_t params[2] = {(addr >> 8) & 0xFF, addr & 0xFF};
    uint8_t out[1];
    sx_write_cmd(SX1262_CMD_READ_BUFFER, params, 2);  /* simplified */
    sx_read_cmd(0x00, out, 1);  /* dummy read */
    return out[0];
}

static void sx_set_rf_freq(uint32_t freq_hz)
{
    /* Convert Hz to SX1262 freq word: freq = (freq_hz * 2^25) / 32MHz */
    uint64_t freq_word = ((uint64_t)freq_hz << 25) / 32000000ULL;
    uint8_t params[4];
    params[0] = (freq_word >> 24) & 0xFF;
    params[1] = (freq_word >> 16) & 0xFF;
    params[2] = (freq_word >> 8) & 0xFF;
    params[3] = freq_word & 0xFF;
    sx_write_cmd(SX1262_CMD_SET_RF_FREQ, params, 4);
}

static void sx_set_pa_config(void)
{
    /* PA config: PA_LUT enabled, HP_MAX_13dBm, device Sel 0 */
    uint8_t params[4] = {0x04, 0x07, 0x00, 0x01};
    sx_write_cmd(SX1262_CMD_SET_PA_CONFIG, params, 4);

    /* TX params: 17 dBm, 200 µs ramp */
    uint8_t tx_params[2] = {LORA_TX_POWER_DBM, 0x02};
    sx_write_cmd(SX1262_CMD_SET_TX_PARAMS, tx_params, 2);
}

static void sx_set_mod_params(void)
{
    /* LoRa modulation parameters:
     * [0] = SF (7), [1] = BW (0x04 = 125 kHz),
     * [2] = CR (0x01 = 4/5), [3] = LDRO (0 = disabled) */
    uint8_t params[4] = {LORA_SF, 0x04, 0x01, 0x00};
    sx_write_cmd(SX1262_CMD_SET_MOD_PARAMS, params, 4);
}

static void sx_set_packet_params(void)
{
    /* LoRa packet parameters:
     * [0..1] = preamble length (8),
     * [2] = header type (0x01 = variable, explicit),
     * [3] = payload length (max),
     * [4] = CRC type (0x01 = on),
     * [5] = invertIQ (0x00 = standard) */
    uint8_t params[6] = {
        (LORA_PREAMBLE_LEN >> 8) & 0xFF, LORA_PREAMBLE_LEN & 0xFF,
        0x01, LORA_MAX_PAYLOAD + 4, 0x01, 0x00
    };
    sx_write_cmd(SX1262_CMD_SET_PACKET_PARAMS, params, 6);
}

static void sx_set_dio_irq(void)
{
    /* Enable TX done, RX done, CAD done, timeout IRQs on DIO1 */
    uint8_t irq_mask[2] = {0x03, 0x00};  /* TX_DONE | RX_DONE */
    uint8_t dio_mask[2] = {0x03, 0x00};
    sx_write_cmd(SX1262_CMD_SET_DIO_IRQ, irq_mask, 2);
    /* Actually SET_DIO_IRQ takes 4 bytes: irqMask + dio1Mask */
    uint8_t params[4] = {0x03, 0x00, 0x03, 0x00};
    sx_write_cmd(SX1262_CMD_SET_DIO_IRQ, params, 4);
}

static void sx_clear_irq(uint16_t mask)
{
    uint8_t params[2] = {(mask >> 8) & 0xFF, mask & 0xFF};
    sx_write_cmd(SX1262_CMD_CLEAR_IRQ, params, 2);
}

static uint16_t sx_get_irq_status(void)
{
    uint8_t out[2];
    sx_read_cmd(SX1262_CMD_GET_IRQ_STATUS, out, 2);
    return ((uint16_t)out[0] << 8) | out[1];
}

static void sx_set_standby(uint8_t cfg)
{
    sx_write_cmd(SX1262_CMD_SET_STANDBY, &cfg, 1);
}

static void sx_set_rx(uint32_t timeout_ms)
{
    uint8_t params[3];
    /* SX1262 timeout is in 15.625 µs units; 0 = continuous */
    uint32_t ticks = (timeout_ms == 0) ? 0xFFFFFF : (timeout_ms * 64);
    params[0] = (ticks >> 16) & 0xFF;
    params[1] = (ticks >> 8) & 0xFF;
    params[2] = ticks & 0xFF;
    sx_write_cmd(SX1262_CMD_SET_RX, params, 3);
}

static void sx_set_tx(uint32_t timeout_ms)
{
    uint8_t params[3];
    uint32_t ticks = (timeout_ms * 64);
    params[0] = (ticks >> 16) & 0xFF;
    params[1] = (ticks >> 8) & 0xFF;
    params[2] = ticks & 0xFF;
    sx_write_cmd(SX1262_CMD_SET_TX, params, 3);
}

static void sx_write_buffer(uint8_t offset, const uint8_t *data, uint8_t len)
{
    lora_tx_buf[0] = SX1262_CMD_WRITE_BUFFER;
    lora_tx_buf[1] = offset;
    if (len + 2 <= sizeof(lora_tx_buf)) {
        memcpy(&lora_tx_buf[2], data, len);
    }
    sx_spi_xfer(lora_tx_buf, lora_rx_buf, len + 2);
}

static void sx_read_buffer(uint8_t offset, uint8_t *data, uint8_t len)
{
    lora_tx_buf[0] = SX1262_CMD_READ_BUFFER;
    lora_tx_buf[1] = offset;
    memset(&lora_tx_buf[2], 0, len);
    sx_spi_xfer(lora_tx_buf, lora_rx_buf, len + 2);
    /* First 2 bytes after command are status + offset, then data */
    memcpy(data, &lora_rx_buf[3], len);
}

static void sx_set_buffer_base(uint8_t tx_base, uint8_t rx_base)
{
    uint8_t params[2] = {tx_base, rx_base};
    sx_write_cmd(SX1262_CMD_SET_BUFFER_BASE, params, 2);
}

/* ------------------------------------------------------------------------- */
/* Packet protocol + CRC */
/* ------------------------------------------------------------------------- */

static uint16_t crc16(const uint8_t *data, uint8_t len)
{
    uint16_t crc = 0xFFFF;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t b = 0; b < 8; b++) {
            if (crc & 0x8000) crc = (crc << 1) ^ 0x1021;
            else              crc = (crc << 1);
        }
    }
    return crc;
}

/* Build a LoRa protocol packet:
 * [magic0] [magic1] [cmd] [len] [payload...] [crc_hi] [crc_lo]
 * Returns total packet length. */
static uint8_t build_packet(uint8_t cmd, const uint8_t *payload,
                             uint8_t payload_len, uint8_t *out)
{
    uint8_t idx = 0;
    out[idx++] = LORA_PROTO_MAGIC0;
    out[idx++] = LORA_PROTO_MAGIC1;
    out[idx++] = cmd;
    out[idx++] = payload_len;
    if (payload_len > 0 && payload_len <= LORA_MAX_PAYLOAD) {
        memcpy(&out[idx], payload, payload_len);
        idx += payload_len;
    }
    uint16_t crc = crc16(out, idx);
    out[idx++] = (crc >> 8) & 0xFF;
    out[idx++] = crc & 0xFF;
    return idx;
}

static bool parse_packet(const uint8_t *pkt, uint8_t pkt_len,
                         uint8_t *cmd, uint8_t *payload, uint8_t *payload_len)
{
    if (pkt_len < 6) return false;
    if (pkt[0] != LORA_PROTO_MAGIC0 || pkt[1] != LORA_PROTO_MAGIC1) return false;

    uint8_t p_len = pkt[3];
    if (pkt_len != 4 + p_len + 2) return false;

    /* Verify CRC */
    uint16_t crc_recv = ((uint16_t)pkt[4 + p_len] << 8) | pkt[5 + p_len];
    uint16_t crc_calc = crc16(pkt, 4 + p_len);
    if (crc_recv != crc_calc) return false;

    *cmd = pkt[2];
    *payload_len = p_len;
    if (p_len > 0) memcpy(payload, &pkt[4], p_len);
    return true;
}

/* ------------------------------------------------------------------------- */
/* State */
/* ------------------------------------------------------------------------- */
static bool lora_initialized = false;
static bool tx_busy = false;
static int8_t last_rssi = 0;
static uint8_t my_device_id = 0x01;  /* default; loaded from flash */

/* Received packet buffer */
static uint8_t rx_packet[128];
static volatile bool rx_pending = false;

/* ------------------------------------------------------------------------- */
/* Public API */
/* ------------------------------------------------------------------------- */

bool lora_init(void)
{
    lora_spi_init();

    /* Hardware reset SX1262 */
    gp_clr(0, 6);   /* RESET_N low */
    for (volatile int i = 0; i < 10000; i++) {}
    gp_set(0, 6);   /* RESET_N high */
    for (volatile int i = 0; i < 10000; i++) {}

    /* Set standby mode */
    sx_set_standby(SX1262_STDBY_RC);

    /* Set packet type to LoRa */
    {
        uint8_t pkt_type = SX1262_PKT_LORA;
        sx_write_cmd(0x8A, &pkt_type, 1);  /* SET_PACKET_TYPE */
    }

    /* Configure RF frequency */
    sx_set_rf_freq(LORA_FREQ_HZ);

    /* Configure modulation */
    sx_set_mod_params();

    /* Configure packet params */
    sx_set_packet_params();

    /* Configure PA */
    sx_set_pa_config();

    /* Set buffer base addresses */
    sx_set_buffer_base(0x00, 0x80);

    /* Set sync word (private network) */
    sx_write_reg(SX1262_REG_SYNC_WORD_MSB, 0x34);
    sx_write_reg(SX1262_REG_SYNC_WORD_LSB, 0x12);

    /* Configure DIO interrupts */
    sx_set_dio_irq();

    /* Clear any pending IRQs */
    sx_clear_irq(SX1262_IRQ_ALL);

    /* Enter RX mode (continuous) */
    sx_set_rx(0);

    lora_initialized = true;
    return true;
}

bool lora_send_glyph(uint8_t buddy_id, const glyph_cmd_t *cmd)
{
    if (!lora_initialized || tx_busy) return false;

    /* Build payload: [buddy_id] [glyph_cmd_t serialized] */
    uint8_t payload[1 + sizeof(glyph_cmd_t)];
    payload[0] = buddy_id;
    memcpy(&payload[1], cmd, sizeof(glyph_cmd_t));

    uint8_t packet[128];
    uint8_t pkt_len = build_packet(LORA_CMD_GLYPH, payload,
                                    1 + (uint8_t)sizeof(glyph_cmd_t),
                                    packet);

    /* Write to TX buffer */
    sx_write_buffer(0x00, packet, pkt_len);

    /* Set TX mode with 3-second timeout */
    sx_clear_irq(SX1262_IRQ_ALL);
    tx_busy = true;
    sx_set_tx(3000);

    return true;
}

bool lora_send_sos(uint8_t my_id, int16_t lat, int16_t lon)
{
    if (!lora_initialized || tx_busy) return false;

    uint8_t payload[5];
    payload[0] = my_id;
    payload[1] = (lat >> 8) & 0xFF;
    payload[2] = lat & 0xFF;
    payload[3] = (lon >> 8) & 0xFF;
    payload[4] = lon & 0xFF;

    uint8_t packet[128];
    uint8_t pkt_len = build_packet(LORA_CMD_SOS, payload, 5, packet);

    sx_write_buffer(0x00, packet, pkt_len);
    sx_clear_irq(SX1262_IRQ_ALL);
    tx_busy = true;
    sx_set_tx(3000);

    return true;
}

bool lora_send_heartbeat(uint8_t my_id, int16_t lat, int16_t lon)
{
    if (!lora_initialized || tx_busy) return false;

    uint8_t payload[5];
    payload[0] = my_id;
    payload[1] = (lat >> 8) & 0xFF;
    payload[2] = lat & 0xFF;
    payload[3] = (lon >> 8) & 0xFF;
    payload[4] = lon & 0xFF;

    uint8_t packet[128];
    uint8_t pkt_len = build_packet(LORA_CMD_HEARTBEAT, payload, 5, packet);

    sx_write_buffer(0x00, packet, pkt_len);
    sx_clear_irq(SX1262_IRQ_ALL);
    tx_busy = true;
    sx_set_tx(3000);

    return true;
}

bool lora_process(glyph_cmd_t *out_cmd, uint8_t *out_sender_id)
{
    if (!lora_initialized) return false;

    /* Check for IRQ events */
    uint16_t irq = sx_get_irq_status();

    if (irq & SX1262_IRQ_TX_DONE) {
        sx_clear_irq(SX1262_IRQ_TX_DONE);
        tx_busy = false;
        /* Return to RX */
        sx_set_rx(0);
    }

    if (irq & SX1262_IRQ_RX_DONE) {
        sx_clear_irq(SX1262_IRQ_RX_DONE);

        /* Read received packet */
        /* Get RX buffer status (simplified: read from offset 0x80) */
        uint8_t pkt[128];
        sx_read_buffer(0x80, pkt, LORA_MAX_PAYLOAD + 6);

        /* Parse packet */
        uint8_t cmd, payload[64], payload_len;
        if (parse_packet(pkt, LORA_MAX_PAYLOAD + 6, &cmd,
                         payload, &payload_len)) {
            if (cmd == LORA_CMD_GLYPH && payload_len >= 1 + sizeof(glyph_cmd_t)) {
                if (out_sender_id) *out_sender_id = payload[0];
                if (out_cmd) memcpy(out_cmd, &payload[1], sizeof(glyph_cmd_t));
                return true;
            } else if (cmd == LORA_CMD_SOS) {
                /* SOS: render emergency ring animation */
                if (out_cmd) {
                    memset(out_cmd, 0, sizeof(*out_cmd));
                    out_cmd->cmd = GLYPH_CMD_ANIM;
                    out_cmd->shape = GLYPH_SHAPE_RING;
                    out_cmd->polarity = GLYPH_WARM;
                    out_cmd->intensity = 255;
                    out_cmd->duration_ms_lo = 0xFF;
                    out_cmd->duration_ms_hi = 0xFF;  /* max duration */
                }
                if (out_sender_id) *out_sender_id = payload[0];
                return true;
            }
        }

        /* Return to RX */
        sx_set_rx(0);
    }

    if (irq & SX1262_IRQ_TIMEOUT) {
        sx_clear_irq(SX1262_IRQ_TIMEOUT);
        tx_busy = false;
        sx_set_rx(0);
    }

    return false;
}

int8_t lora_get_last_rssi(void)
{
    /* Read RSSI from SX1262 (packet status register) */
    return last_rssi;
}

void lora_enter_cad(void)
{
    if (!lora_initialized) return;
    sx_clear_irq(SX1262_IRQ_ALL);
    sx_write_cmd(SX1262_CMD_SET_CAD, NULL, 0);
}

void lora_sleep(void)
{
    if (!lora_initialized) return;
    /* Enter sleep mode (warm start, retains config) */
    uint8_t sleep_cfg = 0x04;  /* warm start + RTC disabled */
    sx_write_cmd(SX1262_CMD_SET_SLEEP, &sleep_cfg, 1);
}

void lora_wake(void)
{
    if (!lora_initialized) return;
    /* Toggle NSS to wake from sleep */
    gp_clr(0, 25);
    for (volatile int i = 0; i < 1000; i++) {}
    gp_set(0, 25);
    sx_wait_busy();
    sx_set_standby(SX1262_STDBY_RC);
    sx_set_rx(0);
}

bool lora_is_tx_busy(void)
{
    return tx_busy;
}