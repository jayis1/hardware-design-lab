/*
 * lora.c — SX1262 LoRa transceiver driver & LoRaWAN uplink
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 *
 * Drives the Semtech SX1262 sub-GHz LoRa transceiver over a dedicated
 * SPI bus (SPI3/GSPI on the ESP32-C3) at 8 MHz. Implements:
 *
 *   - Hardware init, reset, and calibration
 *   - LoRa modulation parameter configuration (SF7, 125 kHz, 868.1 MHz)
 *   - TX with configurable power (up to +22 dBm)
 *   - IRQ-based completion (DIO1 rising edge)
 *   - A simplified LoRaWAN join (OTAA) — the cryptographic join uses
 *     AES-128 CMAC and AES-CTR, implemented in a minimal crypto module.
 *     In a production build this would use LMIC or LoRaMAC-node.
 *   - 27-byte uplink payload packing for the RainForge DSD data format.
 *
 * The SX1262 is a register-less device: all configuration is via SPI
 * opcodes (defined in registers.h). There are no memory-mapped
 * registers — every operation is a command/response over SPI.
 */
#include <stdint.h>
#include <string.h>
#include "../board.h"
#include "../registers.h"
#include "lora.h"

/* ---- Weak SPI/GPIO stubs ---- */
__attribute__((weak)) void spi3_init(uint32_t hz);
__attribute__((weak)) void spi3_cs(int low);
__attribute__((weak)) void spi3_xfer(const uint8_t *tx, uint8_t *rx, uint16_t len);
__attribute__((weak)) void gpio_dir(int pin, int output);
__attribute__((weak)) void gpio_write(int pin, int high);
__attribute__((weak)) int  gpio_read(int pin);
__attribute__((weak)) void gpio_irq_enable(int pin, int falling_edge);
__attribute__((weak)) void delay_ms(uint32_t ms);
__attribute__((weak)) uint32_t millis(void);

/* ---- State ---- */
static lora_state_t g_state = LORA_IDLE;
static lora_downlink_cb g_downlink_cb = NULL;
static uint8_t g_tx_buf[64];
static uint8_t g_rx_buf[64];

/* ================================================================
 * Low-level SPI transaction
 *
 * Every SX1262 transaction starts with an opcode byte. The BUSY pin
 * must be low before sending. After CS goes high, wait for BUSY low
 * again before the next transaction.
 * ================================================================ */
static void lora_wait_busy(void)
{
    int timeout = 10000;
    while (gpio_read(PIN_LORA_BUSY) == 1) {
        if (--timeout == 0) break;
    }
}

static void lora_xfer(const uint8_t *tx, uint8_t *rx, uint16_t len)
{
    lora_wait_busy();
    spi3_cs(0);
    spi3_xfer(tx, rx, len);
    spi3_cs(1);
    lora_wait_busy();
}

static void lora_write_cmd(uint8_t opcode, const uint8_t *params, uint8_t len)
{
    uint8_t tx[16];
    tx[0] = opcode;
    if (len > 0 && len <= 15)
        memcpy(&tx[1], params, len);
    lora_xfer(tx, NULL, len + 1);
}

static void lora_read_cmd(uint8_t opcode, uint8_t *out, uint8_t out_len)
{
    /* For read commands, we send the opcode + dummy bytes, then read.
     * The SX1262 returns the data after a 1-byte NOP following the opcode
     * and any parameter bytes. For status-type reads (GET_*), the first
     * byte after the opcode is the status byte. */
    uint8_t tx[32] = {0};
    uint8_t rx[32] = {0};
    tx[0] = opcode;
    lora_xfer(tx, rx, out_len + 1);
    memcpy(out, &rx[1], out_len);
}

/* ================================================================
 * Initialize the SX1262
 * ================================================================ */
void lora_init(void)
{
    /* Configure GPIO */
    gpio_dir(PIN_LORA_SCK, 1);
    gpio_dir(PIN_LORA_MISO, 0);
    gpio_dir(PIN_LORA_MOSI, 1);
    gpio_dir(PIN_LORA_CS, 1);
    gpio_dir(PIN_LORA_BUSY, 0);
    gpio_dir(PIN_LORA_DIO1, 0);
    gpio_dir(PIN_LORA_NRST, 1);
    gpio_dir(PIN_LORA_ANT_SW, 1);

    gpio_write(PIN_LORA_CS, 1);    /* CS high = idle */
    gpio_write(PIN_LORA_NRST, 1);
    gpio_write(PIN_LORA_ANT_SW, 0); /* RX mode by default */

    /* Initialize SPI3 at 8 MHz */
    spi3_init(LORA_SPI_SPEED);

    /* Hardware reset */
    gpio_write(PIN_LORA_NRST, 0);
    delay_ms(10);
    gpio_write(PIN_LORA_NRST, 1);
    delay_ms(20);   /* SX1262 boot time */

    lora_wait_busy();

    /* Set packet type to LoRa */
    uint8_t pkt_type = SX1262_PKT_LORA;
    lora_write_cmd(SX1262_SET_PKT_TYPE, &pkt_type, 1);

    /* Set RF frequency to 868.1 MHz.
     * The SX1262 frequency is rf_freq = freq_Hz × 2^25 / 32 MHz.
     * 868100000 × 2^25 / 32000000 = 868100000 × 33554432 / 32000000
     *                             = 0x0B13B000 (approximately) */
    uint8_t freq_cmd[4] = { 0x0B, 0x13, 0xB0, 0x00 };
    lora_write_cmd(SX1262_SET_RXFREQ, freq_cmd, 4);

    /* Set LoRa modulation parameters:
     *   SF7, BW=125 kHz (0x04), CR=4/5 (0x01), LDRO off (0x00)
     *   preamble length = 8 symbols (separate command) */
    uint8_t mod_params[4] = {
        (uint8_t)LORA_SF,   /* spreading factor */
        0x04,               /* BW 125 kHz */
        0x01,               /* CR 4/5 */
        0x00                /* LDRO disabled */
    };
    lora_write_cmd(SX1262_SET_MOD_PARAMS, mod_params, 4);

    /* Set PA config: SX1262 has PA_LUT for >14 dBm.
     *   paDutyCycle=0x04, hpMax=0x07, deviceSel=0x00, paLut=0x01 */
    uint8_t pa_cfg[4] = { 0x04, 0x07, 0x00, 0x01 };
    lora_write_cmd(SX1262_SET_PA_CONFIG, pa_cfg, 4);

    /* Set TX params: power 20 dBm, ramp 200 µs (0x04) */
    uint8_t tx_params[2] = { (uint8_t)LORA_TX_POWER_DBM, 0x04 };
    lora_write_cmd(SX1262_SET_TX_PARAMS, tx_params, 2);

    /* Set DIO2 as RF switch control (for the antenna TX/RX switch) */
    uint8_t dio2_rf = 0x01;
    lora_write_cmd(SX1262_SET_DIO2_AS_RF, &dio2_rf, 1);

    /* Set DIO3 as TCXO control voltage (1.7 V, delay 5 ms).
     * The RainForge board uses a TCXO for frequency stability. */
    uint8_t tcxo_cmd[3] = { 0x00, 0x05, 0x00 };  /* 1.7V, 5ms delay (units of 15.625µs) */
    /* Actually: SET_DIO3_AS_TCXO takes 4 bytes: voltage(1) + delay(3) */
    uint8_t tcxo[4] = { 0x00, 0x00, 0x05, 0x00 };
    lora_write_cmd(SX1262_SET_DIO3_AS_TC, tcxo, 4);

    /* Set regulator mode: DC-DC + LDO (0x01 = DCDC enabled) */
    uint8_t reg_mode = 0x01;
    lora_write_cmd(SX1262_SET_REGULATOR, &reg_mode, 1);

    /* Calibrate all blocks */
    uint8_t cal_param = 0x7F;   /* calibrate all */
    lora_write_cmd(SX1262_CALIBRATE, &cal_param, 1);
    delay_ms(5);

    /* Configure IRQ: TX_DONE + RX_DONE + TIMEOUT + CRC_ERR */
    uint8_t irq_cfg[8] = {
        (uint8_t)((SX1262_IRQ_TX_DONE | SX1262_IRQ_RX_DONE | SX1262_IRQ_TIMEOUT
                  | SX1262_IRQ_CRC_ERR) >> 8),
        (uint8_t)((SX1262_IRQ_TX_DONE | SX1262_IRQ_RX_DONE | SX1262_IRQ_TIMEOUT
                  | SX1262_IRQ_CRC_ERR) & 0xFF),
        0x00, 0x00,   /* DIO1 mask (same) */
        (uint8_t)((SX1262_IRQ_TX_DONE | SX1262_IRQ_RX_DONE | SX1262_IRQ_TIMEOUT
                  | SX1262_IRQ_CRC_ERR) >> 8),
        (uint8_t)((SX1262_IRQ_TX_DONE | SX1262_IRQ_RX_DONE | SX1262_IRQ_TIMEOUT
                  | SX1262_IRQ_CRC_ERR) & 0xFF),
        0x00, 0x00
    };
    lora_write_cmd(SX1262_SET_DIO_IRQ, irq_cfg, 8);

    /* Clear any pending IRQs */
    uint8_t clear_cmd[2] = { 0x03, 0xFF };
    lora_write_cmd(SX1262_CLEAR_IRQ, clear_cmd, 2);

    /* Set standby mode (STDBY_XOSC = 0x01, uses TCXO) */
    uint8_t stdby = 0x01;
    lora_write_cmd(SX1262_SET_STDBY, &stdby, 1);

    g_state = LORA_IDLE;
}

/* ================================================================
 * Reset
 * ================================================================ */
void lora_reset(void)
{
    gpio_write(PIN_LORA_NRST, 0);
    delay_ms(10);
    gpio_write(PIN_LORA_NRST, 1);
    delay_ms(20);
    lora_init();
}

/* ================================================================
 * Status & IRQ
 * ================================================================ */
int lora_read_status(uint8_t *status)
{
    lora_read_cmd(SX1262_GET_STATUS, status, 1);
    return 0;
}

int lora_check_irq(uint16_t *irq_flags)
{
    uint8_t buf[2] = {0};
    lora_read_cmd(SX1262_GET_IRQ_STATUS, buf, 2);
    *irq_flags = ((uint16_t)buf[0] << 8) | buf[1];
    return (*irq_flags != 0) ? 1 : 0;
}

void lora_clear_irq(uint16_t mask)
{
    uint8_t cmd[3] = { SX1262_CLEAR_IRQ,
                       (uint8_t)(mask >> 8),
                       (uint8_t)(mask & 0xFF) };
    lora_write_cmd(SX1262_CLEAR_IRQ, &cmd[1], 2);
}

/* ================================================================
 * Sleep / wake
 * ================================================================ */
void lora_sleep(void)
{
    /* Set sleep with warm start (0x04 = sleep + RTC + warm start) */
    uint8_t sleep_cfg = 0x04;
    lora_write_cmd(SX1262_SET_SLEEP, &sleep_cfg, 1);
}

void lora_wake(void)
{
    /* Toggling CS wakes the SX1262 from sleep (warm start preserves config) */
    spi3_cs(0);
    delay_ms(1);
    spi3_cs(1);
    lora_wait_busy();
}

/* ================================================================
 * Simplified LoRaWAN join (OTAA)
 *
 * A full OTAA join requires AES-128-CMAC for the join-request MIC and
 * AES-128-CTR for decrypting the join-accept. This implementation
 * provides the frame construction and the state machine; the crypto
 * is stubbed as a weak function that would link against LMIC's AES
 * in a production build.
 * ================================================================ */
__attribute__((weak)) void lora_aes_cmac(const uint8_t *key, const uint8_t *msg,
                                          uint8_t len, uint8_t *mic_out);

int lora_join(const uint8_t *app_eui, const uint8_t *app_key,
              const uint8_t *dev_eui)
{
    (void)app_eui; (void)app_key; (void)dev_eui;

    g_state = LORA_JOINING;

    /* Build JoinReq frame:
     *   MHDR(1) | JoinEUI(8) | DevEUI(8) | DevNonce(2) | MIC(4) = 23 bytes
     * MHDR = 0x00 (JoinRequest, LoRaWAN 1.0.4) */
    uint8_t join_req[23];
    join_req[0] = 0x00;
    memcpy(&join_req[1], app_eui, 8);
    memcpy(&join_req[9], dev_eui, 8);
    /* DevNonce: random 2-byte (would use ESP32 hardware RNG) */
    join_req[17] = 0x12; join_req[18] = 0x34;

    /* Compute MIC = AES-CMAC(AppKey, MHDR | JoinEUI | DevEUI | DevNonce) */
    uint8_t mic[4];
    lora_aes_cmac(app_key, join_req, 19, mic);
    memcpy(&join_req[19], mic, 4);

    /* Transmit JoinReq on the join channel (RX1 window 5 sec, RX2 6 sec) */
    /* Set buffer base address */
    uint8_t buf_base[4] = { 0x00, 0x00, 0x00, 0x00 };  /* txBase=0, rxBase=0 */
    lora_write_cmd(SX1262_SET_BUFFER_BASE, buf_base, 4);

    /* Write payload to SX1262 buffer */
    uint8_t write_cmd[24] = { SX1262_WRITE_BUFFER, 0x00 };
    memcpy(&write_cmd[2], join_req, 23);
    lora_xfer(write_cmd, NULL, 24);

    /* Set TX with timeout: 5000 ms → timeout = 5000 × 64 = 320000 (units of 15.625µs)
     * timeout = 0x0004E200 (3 bytes) */
    uint8_t tx_cmd[3] = { 0x00, 0x4E, 0x20 };
    lora_write_cmd(SX1262_SET_TX, tx_cmd, 3);

    /* Wait for TX_DONE (poll BUSY + DIO1 in a real build) */
    uint16_t irq = 0;
    int timeout = 6000;  /* 6 seconds max */
    while (!lora_check_irq(&irq) && timeout > 0) {
        delay_ms(1);
        timeout--;
    }

    if (irq & SX1262_IRQ_TX_DONE) {
        lora_clear_irq(SX1262_IRQ_TX_DONE);
        /* Now open RX1 window for JoinAccept (5 sec after TX) */
        /* In a full implementation we'd wait 5 sec, then set RX with
         * RX1 parameters (SF7, 868.1 MHz), then RX2 (SF12, 869.525 MHz). */
        /* For this driver we leave the state machine ready for the
         * application layer to handle the join-accept. */
        g_state = LORA_JOINED;
        return 0;
    }

    if (irq & SX1262_IRQ_TIMEOUT) {
        lora_clear_irq(SX1262_IRQ_TIMEOUT);
        g_state = LORA_ERROR;
        return -2;
    }

    g_state = LORA_ERROR;
    return -1;
}

/* ================================================================
 * Transmit an uplink payload
 * ================================================================ */
int lora_tx(const uint8_t *payload, uint8_t len, uint8_t port)
{
    (void)port;   /* port is part of the LoRaWAN MAC layer, handled above */

    if (len > LORA_PAYLOAD_MAX) return -1;

    g_state = LORA_TX_PENDING;

    /* Write payload to SX1262 buffer at offset 0 */
    uint8_t write_cmd[64] = { SX1262_WRITE_BUFFER, 0x00 };
    memcpy(&write_cmd[2], payload, len);
    lora_xfer(write_cmd, NULL, len + 2);

    /* Set TX with timeout (10 seconds max for SF7 at 125 kHz) */
    uint8_t tx_cmd[3] = { 0x00, 0x27, 0x10 };  /* 10 sec in 15.625µs units */
    lora_write_cmd(SX1262_SET_TX, tx_cmd, 3);

    /* Wait for TX_DONE */
    uint16_t irq = 0;
    int timeout = 12000;
    while (!lora_check_irq(&irq) && timeout > 0) {
        delay_ms(1);
        timeout--;
    }

    if (irq & SX1262_IRQ_TX_DONE) {
        lora_clear_irq(SX1262_IRQ_TX_DONE);
        g_state = LORA_TX_DONE;

        /* Open RX1/RX2 windows for downlink (simplified: single 6-sec RX) */
        uint8_t rx_cmd[3] = { 0x00, 0x93, 0x80 };  /* 6 sec */
        lora_write_cmd(SX1262_SET_RX, rx_cmd, 3);

        /* Check for downlink */
        delay_ms(100);
        if (lora_check_irq(&irq)) {
            if (irq & SX1262_IRQ_RX_DONE) {
                lora_clear_irq(SX1262_IRQ_RX_DONE);
                /* Read received data */
                uint8_t rx_status[2] = {0};
                lora_read_cmd(SX1262_GET_RX_BUF_STATUS, rx_status, 2);
                uint8_t rx_len = rx_status[0];
                uint8_t rx_offset = rx_status[1];

                if (rx_len > 0 && rx_len <= 64 && g_downlink_cb) {
                    uint8_t read_cmd[66] = { SX1262_READ_BUFFER, rx_offset };
                    lora_xfer(read_cmd, g_rx_buf, rx_len + 2);
                    g_downlink_cb(port, &g_rx_buf[2], rx_len);
                }
                g_state = LORA_RX_DOWNLINK;
            }
            if (irq & SX1262_IRQ_TIMEOUT) {
                lora_clear_irq(SX1262_IRQ_TIMEOUT);
            }
        }
        return 0;
    }

    if (irq & SX1262_IRQ_TIMEOUT) {
        lora_clear_irq(SX1262_IRQ_TIMEOUT);
        g_state = LORA_ERROR;
        return -2;
    }

    g_state = LORA_ERROR;
    return -1;
}

/* ================================================================
 * Build the 27-byte RainForge uplink payload from DSD statistics
 * ================================================================ */
int lora_build_payload(const dsd_t *dsd, float scap_v, float temp_c,
                       uint8_t *out)
{
    memset(out, 0, LORA_PAYLOAD_LEN);

    /* Byte 0: status byte (bits: 0-2 state, 3 vok, 4 low, 5-7 reserved) */
    out[0] = (uint8_t)(g_state & 0x07);

    /* Rainfall rate × 100 (uint16) */
    uint16_t r_q100 = (uint16_t)CLAMP(dsd->rainfall_rate * 100.0f, 0, 65535);
    out[1] = r_q100 & 0xFF;
    out[2] = (r_q100 >> 8) & 0xFF;

    /* Reflectivity Z (uint32, mm^6/m^3) */
    uint32_t z = (uint32_t)CLAMP(dsd->reflectivity, 0, 0xFFFFFFFF);
    out[3] = z & 0xFF;
    out[4] = (z >> 8) & 0xFF;
    out[5] = (z >> 16) & 0xFF;
    out[6] = (z >> 24) & 0xFF;

    /* LWC × 100 (uint16) */
    uint16_t lwc = (uint16_t)CLAMP(dsd->liquid_water_content * 100.0f, 0, 65535);
    out[7] = lwc & 0xFF;
    out[8] = (lwc >> 8) & 0xFF;

    /* Mean diameter × 256 (uint16) */
    uint16_t dm = (uint16_t)CLAMP(dsd->mean_diameter * 256.0f, 0, 65535);
    out[9] = dm & 0xFF;
    out[10] = (dm >> 8) & 0xFF;

    /* Median diameter × 256 */
    uint16_t d0 = (uint16_t)CLAMP(dsd->median_diameter * 256.0f, 0, 65535);
    out[11] = d0 & 0xFF;
    out[12] = (d0 >> 8) & 0xFF;

    /* Z-R a × 10 (uint16) */
    uint16_t zra = (uint16_t)CLAMP(dsd->zr_a * 10.0f, 0, 65535);
    out[13] = zra & 0xFF;
    out[14] = (zra >> 8) & 0xFF;

    /* Z-R b × 10 (uint8) */
    out[15] = (uint8_t)CLAMP(dsd->zr_b * 10.0f, 0, 255);

    /* Total droplets (uint16, capped at 65535) */
    uint16_t total = (uint16_t)MIN(dsd->total_droplets, 65535);
    out[16] = total & 0xFF;
    out[17] = (total >> 8) & 0xFF;

    /* Positive charge count */
    out[18] = dsd->pos_count & 0xFF;
    out[19] = (dsd->pos_count >> 8) & 0xFF;

    /* Negative charge count */
    out[20] = dsd->neg_count & 0xFF;
    out[21] = (dsd->neg_count >> 8) & 0xFF;

    /* Average charge × 16 (int16) */
    int16_t avg_q = dsd->avg_charge_pc;
    out[22] = avg_q & 0xFF;
    out[23] = (avg_q >> 8) & 0xFF;

    /* Supercap voltage × 1000 (uint16, V) */
    uint16_t v = (uint16_t)CLAMP(scap_v * 1000.0f, 0, 65535);
    out[24] = v & 0xFF;
    out[25] = (v >> 8) & 0xFF;

    /* Temperature + 100 (int8) */
    out[26] = (int8_t)CLAMP(temp_c + 100.0f, -128, 127);

    return LORA_PAYLOAD_LEN;
}

/* ================================================================
 * State & downlink callback
 * ================================================================ */
lora_state_t lora_get_state(void) { return g_state; }

void lora_set_downlink_cb(lora_downlink_cb cb) { g_downlink_cb = cb; }