/*
 * lorawan.c — SX1262 LoRaWAN driver for MycoMesh
 * MycoMesh — Open-Source Fungal Mycelium Electrophysiology & Chemical Sensing Network
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Driver for the Semtech SX1262 sub-GHz LoRa transceiver, providing
 * LoRaWAN Class A uplink/downlink capability.  This implementation
 * handles the SPI communication with the SX1262, LoRa modulation
 * configuration, and a simplified LoRaWAN MAC layer (OTAA join +
 * confirmed/unconfirmed uplinks).
 *
 * A full LoRaWAN stack (e.g., LoRaMAC-node) would be used in production;
 * this driver provides the essential radio interface and frame formatting.
 */

#include "lorawan.h"
#include "board.h"
#include "registers.h"
#include <string.h>

/* ===================================================================== */
/*  SX1262 SPI access                                                     */
/* ===================================================================== */

static void lora_cs_low(void)  { GPIO_RESET(LORA_CS_PORT, LORA_CS_PIN); }
static void lora_cs_high(void) { GPIO_SET(LORA_CS_PORT, LORA_CS_PIN); }

static uint8_t lora_spi_xfer(uint8_t tx)
{
    while (!(SPI3->SR & SPI_SR_TXE));
    *(volatile uint8_t *)&SPI3->DR = tx;
    while (!(SPI3->SR & SPI_SR_RXNE));
    return *(volatile uint8_t *)&SPI3->DR;
}

/* Wait for SX1262 BUSY pin to go low (max 10 ms timeout). */
static int lora_wait_busy(void)
{
    uint32_t timeout = 1200000;  /* ~10 ms at 120 MHz */
    while (GPIO_READ(LORA_BUSY_PORT, LORA_BUSY_PIN) && --timeout);
    return (timeout > 0) ? 0 : -1;
}

/* Write a register on the SX1262. */
static void lora_write_reg(uint16_t addr, uint8_t value)
{
    lora_wait_busy();
    lora_cs_low();
    lora_spi_xfer(0x0D);             /* WriteRegister command */
    lora_spi_xfer((addr >> 8) & 0xFF);
    lora_spi_xfer(addr & 0xFF);
    lora_spi_xfer(value);
    lora_cs_high();
}

/* Read a register from the SX1262. */
static uint8_t lora_read_reg(uint16_t addr)
{
    lora_wait_busy();
    lora_cs_low();
    lora_spi_xfer(0x1D);             /* ReadRegister command */
    lora_spi_xfer((addr >> 8) & 0xFF);
    lora_spi_xfer(addr & 0xFF);
    lora_spi_xfer(0xFF);             /* dummy byte */
    uint8_t val = lora_spi_xfer(0xFF);
    lora_cs_high();
    return val;
}

/* Write a buffer to the SX1262. */
static void lora_write_buffer(uint8_t offset, const uint8_t *data, uint8_t len)
{
    lora_wait_busy();
    lora_cs_low();
    lora_spi_xfer(0x0E);             /* WriteBuffer command */
    lora_spi_xfer(offset);
    for (uint8_t i = 0; i < len; i++) {
        lora_spi_xfer(data[i]);
    }
    lora_cs_high();
}

/* Read a buffer from the SX1262. */
static void lora_read_buffer(uint8_t offset, uint8_t *data, uint8_t len)
{
    lora_wait_busy();
    lora_cs_low();
    lora_spi_xfer(0x1E);             /* ReadBuffer command */
    lora_spi_xfer(offset);
    lora_spi_xfer(0xFF);             /* dummy byte */
    for (uint8_t i = 0; i < len; i++) {
        data[i] = lora_spi_xfer(0xFF);
    }
    lora_cs_high();
}

/* Send a command to the SX1262. */
static void lora_write_cmd(uint8_t cmd, const uint8_t *params, uint8_t len)
{
    lora_wait_busy();
    lora_cs_low();
    lora_spi_xfer(cmd);
    for (uint8_t i = 0; i < len; i++) {
        lora_spi_xfer(params[i]);
    }
    lora_cs_high();
}

/* ===================================================================== */
/*  SX1262 register addresses (subset)                                    */
/* ===================================================================== */

#define SX1262_REG_PKT_TYPE      0x880
#define SX1262_REG_RF_FREQ       0x884
#define SX1262_REG_TX_CFG        0x88A
#define SX1262_REG_MOD_CFG1      0x88E
#define SX1262_REG_MOD_CFG2      0x88F
#define SX1262_REG_TX_PWR        0x895
#define SX1262_REG_PKT_PARAMS1   0x898
#define SX1262_REG_PKT_PARAMS2   0x899
#define SX1262_REG_PKT_PARAMS3   0x89A
#define SX1262_REG_PAYLOAD_LEN   0x89B
#define SX1262_REG_AUTO_TX       0x026
#define SX1262_REG_IRQ_MASK      0x8FF
#define SX1262_REG_IRQ_STATUS    0x8FF
#define SX1262_REG_RX_BUFFER     0x8AD

/* SX1262 commands */
#define SX1262_CMD_SET_STANDBY   0x80
#define SX1262_CMD_SET_PACKET_TYPE 0x01
#define SX1262_CMD_SET_RF_FREQ   0x86
#define SX1262_CMD_SET_TX_PARAMS 0x8E
#define SX1262_CMD_SET_MOD_PARAMS 0x8B
#define SX1262_CMD_SET_PKT_PARAMS 0x8C
#define SX1262_CMD_SET_TX        0x83
#define SX1262_CMD_SET_RX        0x82
#define SX1262_CMD_SET_BUFFER_BASE 0x8F
#define SX1262_CMD_CLEAR_IRQ     0x02
#define SX1262_CMD_GET_RX_BUFFER_STATUS 0x13
#define SX1262_CMD_GET_PACKET_STATUS 0x14

/* Packet types */
#define SX1262_PKT_TYPE_LORA     0x01

/* IRQ flags */
#define SX1262_IRQ_TX_DONE       (1U << 7)
#define SX1262_IRQ_RX_DONE       (1U << 6)
#define SX1262_IRQ_TIMEOUT       (1U << 5)
#define SX1262_IRQ_CRC_ERR       (1U << 4)
#define SX1262_IRQ_HEADER_ERR    (1U << 3)
#define SX1262_IRQ_ALL           0x07FF

/* ===================================================================== */
/*  Driver state                                                          */
/* ===================================================================== */

static uint8_t  g_region = 0;        /* 0 = EU868, 1 = US915 */
static uint32_t g_frequency = LORA_FREQ_EU868;
static uint8_t  g_joined = 0;
static uint8_t  g_dev_eui[8];
static uint8_t  g_app_eui[8];
static uint8_t  g_app_key[16];
static int16_t  g_last_rssi = 0;
static int8_t   g_last_snr = 0;

/* LoRaWAN frame counters. */
static uint32_t g_fcnt_up = 0;
static uint32_t g_fcnt_down = 0;

/* ===================================================================== */
/*  SX1262 initialization                                                 */
/* ===================================================================== */

static void sx1262_reset(void)
{
    GPIO_RESET(LORA_NRST_PORT, LORA_NRST_PIN);
    for (volatile int i = 0; i < 10000; i++);  /* ~100 µs */
    GPIO_SET(LORA_NRST_PORT, LORA_NRST_PIN);
    for (volatile int i = 0; i < 100000; i++);  /* ~1 ms */
}

static void sx1262_set_standby(uint8_t config)
{
    /* config: 0x00 = STDBY_RC, 0x01 = STDBY_XOSC */
    lora_write_cmd(SX1262_CMD_SET_STANDBY, &config, 1);
}

static void sx1262_set_packet_type(uint8_t type)
{
    lora_write_cmd(SX1262_CMD_SET_PACKET_TYPE, &type, 1);
}

static void sx1262_set_rf_frequency(uint32_t freq_hz)
{
    /* Convert Hz to SX1262's 32-bit frequency word:
       freq_word = freq_hz * 2^25 / 32 MHz */
    uint64_t word = ((uint64_t)freq_hz << 25) / 32000000ULL;
    uint8_t params[4];
    params[0] = (uint8_t)(word >> 24);
    params[1] = (uint8_t)(word >> 16);
    params[2] = (uint8_t)(word >> 8);
    params[3] = (uint8_t)(word >> 0);
    lora_write_cmd(SX1262_CMD_SET_RF_FREQ, params, 4);
}

static void sx1262_set_tx_params(int8_t power_dbm, uint8_t ramp)
{
    uint8_t params[2];
    params[0] = (uint8_t)power_dbm;
    params[1] = ramp;  /* 0x00 = 10 µs ramp */
    lora_write_cmd(SX1262_CMD_SET_TX_PARAMS, params, 2);
}

static void sx1262_set_mod_params(uint8_t sf, uint8_t bw, uint8_t cr)
{
    uint8_t params[4];
    params[0] = sf;    /* Spreading factor: 7–12 */
    params[1] = bw;    /* Bandwidth: 0x00=125k, 0x01=250k, 0x02=500k */
    params[2] = cr;    /* Coding rate: 0x01=4/5, 0x02=4/6, 0x03=4/7, 0x04=4/8 */
    params[3] = 0x00;  /* Low data rate optimization off */
    lora_write_cmd(SX1262_CMD_SET_MOD_PARAMS, params, 4);
}

static void sx1262_set_packet_params(uint8_t preamble_len,
                                      uint8_t header_type,
                                      uint8_t payload_len,
                                      uint8_t crc_type)
{
    uint8_t params[6];
    params[0] = preamble_len;
    params[1] = 0x00;  /* preamble MSB (for long preambles) */
    params[2] = header_type;  /* 0x00 = explicit, 0x01 = implicit */
    params[3] = payload_len;
    params[4] = crc_type;     /* 0x00 = off, 0x01 = on */
    params[5] = 0x00;  /* InvertIQ standard */
    lora_write_cmd(SX1262_CMD_SET_PKT_PARAMS, params, 6);
}

static void sx1262_set_tx(uint32_t timeout_ms)
{
    /* Set TX with timeout in SX1262 units (15.625 µs per unit). */
    uint32_t timeout = timeout_ms * 64;  /* ms → SX1262 units */
    uint8_t params[3];
    params[0] = (uint8_t)(timeout >> 16);
    params[1] = (uint8_t)(timeout >> 8);
    params[2] = (uint8_t)(timeout >> 0);
    lora_write_cmd(SX1262_CMD_SET_TX, params, 3);
}

static void sx1262_clear_irq(uint16_t mask)
{
    uint8_t params[2];
    params[0] = (uint8_t)(mask >> 8);
    params[1] = (uint8_t)(mask >> 0);
    lora_write_cmd(SX1262_CMD_CLEAR_IRQ, params, 2);
}

static uint16_t sx1262_get_irq_status(void)
{
    uint8_t status[2];
    lora_wait_busy();
    lora_cs_low();
    lora_spi_xfer(0x12);  /* GetIrqStatus */
    lora_spi_xfer(0xFF);  /* dummy */
    status[0] = lora_spi_xfer(0xFF);
    status[1] = lora_spi_xfer(0xFF);
    lora_cs_high();
    return ((uint16_t)status[0] << 8) | status[1];
}

/* ===================================================================== */
/*  LoRaWAN MAC layer (simplified)                                        */
/* ===================================================================== */

/* AES-128 encryption placeholder.
   In a full LoRaWAN implementation, this would use a hardware AES
   accelerator or software AES-128 to encrypt payloads with the
   AppSKey and compute the MIC with the NwkSKey.  For this open
   implementation, we provide the frame structure but note that
   a production deployment MUST implement AES-128. */

static void lorawan_encrypt_payload(uint8_t *payload, uint8_t len,
                                     const uint8_t *key, uint32_t fcnt,
                                     uint8_t dir, uint8_t port)
{
    /* TODO: Implement AES-128 CTR mode encryption using key, fcnt,
       dir, and port to generate the keystream block.
       For now, payload is sent in plaintext (development mode only).
       Production firmware MUST link against an AES-128 library. */
    (void)key; (void)fcnt; (void)dir; (void)port;
}

/* Build a LoRaWAN unconfirmed uplink frame.
   Format: MHDR(1) | FHDR(7) | FPort(1) | FRMPayload(N) | MIC(4) */
static uint8_t lorawan_build_frame(uint8_t port,
                                    const uint8_t *payload, uint8_t len,
                                    uint8_t *frame, uint8_t max_len)
{
    if (len + 13 > max_len) return 0;  /* frame too large */

    uint8_t pos = 0;

    /* MHDR: MType=0 (unconfirmed up) | Major=0 (LoRaWAN 1.0) */
    frame[pos++] = 0x40;

    /* FHDR: DevAddr(4) | FCtrl(1) | FCnt(2) | FOpts(0) */
    /* DevAddr: for ABP, this is from config; for OTAA, assigned by network.
       Using a placeholder DevAddr (from join accept). */
    frame[pos++] = 0x00;  /* DevAddr bytes (would be real address) */
    frame[pos++] = 0x00;
    frame[pos++] = 0x00;
    frame[pos++] = 0x00;

    /* FCtrl: ADR=0, RFU=0, ACK=0, ClassA=0, FOptsLen=0 */
    frame[pos++] = 0x00;

    /* FCnt (2 bytes, little-endian) */
    frame[pos++] = (uint8_t)(g_fcnt_up & 0xFF);
    frame[pos++] = (uint8_t)((g_fcnt_up >> 8) & 0xFF);

    /* FPort */
    frame[pos++] = port;

    /* FRMPayload (encrypted with AppSKey for data frames) */
    memcpy(&frame[pos], payload, len);
    lorawan_encrypt_payload(&frame[pos], len, g_app_key, g_fcnt_up, 0, port);
    pos += len;

    /* MIC (4 bytes) — computed with NwkSKey over the frame.
       Placeholder: zeros (production MUST compute CMAC-AES128). */
    frame[pos++] = 0x00;
    frame[pos++] = 0x00;
    frame[pos++] = 0x00;
    frame[pos++] = 0x00;

    g_fcnt_up++;

    return pos;  /* total frame length */
}

/* ===================================================================== */
/*  Public API                                                            */
/* ===================================================================== */

int lorawan_init(uint8_t region)
{
    g_region = region;
    g_frequency = (region == 1) ? LORA_FREQ_US915 : LORA_FREQ_EU868;

    /* Hardware reset the SX1262. */
    sx1262_reset();

    /* Set standby mode. */
    sx1262_set_standby(0x01);  /* STDBY_XOSC */

    /* Set packet type to LoRa. */
    sx1262_set_packet_type(SX1262_PKT_TYPE_LORA);

    /* Set RF frequency. */
    sx1262_set_rf_frequency(g_frequency);

    /* Set TX power (+20 dBm with PA boost). */
    sx1262_set_tx_params(LORA_TX_POWER_DBM, 0x00);

    /* Set modulation parameters: SF9, BW=125kHz, CR=4/5. */
    sx1262_set_mod_params(LORA_SF, 0x00, LORA_CR);

    /* Set packet parameters (will be updated per transmission). */
    sx1262_set_packet_params(LORA_PREAMBLE_LEN, 0x00, 0x00, 0x01);

    /* Clear all IRQ flags. */
    sx1262_clear_irq(SX1262_IRQ_ALL);

    /* Set antenna switch to TX (would be configured per region). */
    GPIO_SET(LORA_ANT_SW_PORT, LORA_ANT_SW_PIN);

    return 0;
}

void lorawan_set_region(uint8_t region)
{
    g_region = region;
    g_frequency = (region == 1) ? LORA_FREQ_US915 : LORA_FREQ_EU868;
    sx1262_set_rf_frequency(g_frequency);
}

void lorawan_set_keys(const uint8_t *dev_eui, const uint8_t *app_eui,
                      const uint8_t *app_key)
{
    if (dev_eui) memcpy(g_dev_eui, dev_eui, 8);
    if (app_eui) memcpy(g_app_eui, app_eui, 8);
    if (app_key) memcpy(g_app_key, app_key, 16);
}

int lorawan_join(void)
{
    /* OTAA join procedure:
       1. Build join request frame (MHDR | JoinEUI | DevEUI | DevNonce | MIC)
       2. Send join request
       3. Wait for join accept (downlink within RX1/RX2 windows)
       4. Parse join accept to get DevAddr, AppSKey, NwkSKey

       This is a simplified placeholder — a full OTAA implementation
       requires AES-128 CMAC and key derivation. */

    /* Build join request. */
    uint8_t join_req[23];
    uint8_t pos = 0;

    /* MHDR: MType=0x05 (join request) | Major=0 */
    join_req[pos++] = 0x00;  /* JoinEUI not sent in simplified mode */

    /* JoinEUI (8 bytes, little-endian). */
    for (int i = 7; i >= 0; i--) join_req[pos++] = g_app_eui[i];

    /* DevEUI (8 bytes, little-endian). */
    for (int i = 7; i >= 0; i--) join_req[pos++] = g_dev_eui[i];

    /* DevNonce (2 bytes). */
    join_req[pos++] = (uint8_t)(g_fcnt_up & 0xFF);
    join_req[pos++] = (uint8_t)((g_fcnt_up >> 8) & 0xFF);

    /* MIC (4 bytes) — placeholder. */
    join_req[pos++] = 0;
    join_req[pos++] = 0;
    join_req[pos++] = 0;
    join_req[pos++] = 0;

    /* Send join request via LoRa. */
    sx1262_set_packet_params(LORA_PREAMBLE_LEN, 0x00, pos, 0x01);
    lora_write_buffer(0, join_req, pos);

    /* Set TX (10 second timeout). */
    sx1262_set_tx(10000);

    /* Wait for TX done. */
    uint32_t timeout = 12000000;
    while (!(sx1262_get_irq_status() & SX1262_IRQ_TX_DONE) && --timeout);
    sx1262_clear_irq(SX1262_IRQ_TX_DONE);

    /* In a full implementation, we would now set RX1 (1 second after TX)
       and RX2 (2 seconds after TX) windows to receive the join accept.
       For this simplified version, we assume the join succeeds. */
    g_joined = 1;

    return 0;
}

int lorawan_send(uint8_t port, const uint8_t *payload, uint8_t len)
{
    if (len > LORA_MAX_PAYLOAD) return -1;

    /* Ensure we're joined (or use ABP with pre-configured keys). */
    if (!g_joined) {
        lorawan_join();
        if (!g_joined) return -1;
    }

    /* Build LoRaWAN frame. */
    uint8_t frame[64];
    uint8_t frame_len = lorawan_build_frame(port, payload, len,
                                             frame, sizeof(frame));
    if (frame_len == 0) return -1;

    /* Set packet parameters for this frame. */
    sx1262_set_packet_params(LORA_PREAMBLE_LEN, 0x00, frame_len, 0x01);

    /* Write frame to SX1262 buffer. */
    lora_write_buffer(0, frame, frame_len);

    /* Set buffer base address. */
    uint8_t base_params[4] = {0, 0, 0, 0};
    lora_write_cmd(SX1262_CMD_SET_BUFFER_BASE, base_params, 4);

    /* Initiate TX (5 second timeout). */
    sx1262_set_tx(5000);

    /* Wait for TX done. */
    uint32_t timeout = 6000000;
    while (!(sx1262_get_irq_status() & SX1262_IRQ_TX_DONE) && --timeout);
    sx1262_clear_irq(SX1262_IRQ_TX_DONE);

    if (timeout == 0) return -1;

    /* For Class A, open RX1 window (1 second after TX, same SF). */
    /* Set RX with 1 second timeout. */
    uint8_t rx_params[3] = {0, 0, 64};  /* 1 second */
    lora_write_cmd(SX1262_CMD_SET_RX, rx_params, 3);

    /* Wait for RX done or timeout. */
    timeout = 1200000;
    uint16_t irq;
    do {
        irq = sx1262_get_irq_status();
    } while (!(irq & (SX1262_IRQ_RX_DONE | SX1262_IRQ_TIMEOUT)) && --timeout);

    if (irq & SX1262_IRQ_RX_DONE) {
        sx1262_clear_irq(SX1262_IRQ_RX_DONE);

        /* Get RX buffer status. */
        lora_wait_busy();
        lora_cs_low();
        lora_spi_xfer(SX1262_CMD_GET_RX_BUFFER_STATUS);
        lora_spi_xfer(0xFF);
        uint8_t rx_len = lora_spi_xfer(0xFF);
        uint8_t rx_offset = lora_spi_xfer(0xFF);
        lora_cs_high();

        /* Get packet status (RSSI, SNR). */
        lora_wait_busy();
        lora_cs_low();
        lora_spi_xfer(SX1262_CMD_GET_PACKET_STATUS);
        lora_spi_xfer(0xFF);
        int8_t rssi_raw = (int8_t)lora_spi_xfer(0xFF);
        int8_t snr_raw = (int8_t)lora_spi_xfer(0xFF);
        lora_cs_high();

        g_last_rssi = -rssi_raw / 2;
        g_last_snr = snr_raw / 4;

        (void)rx_len; (void)rx_offset;
    } else {
        sx1262_clear_irq(SX1262_IRQ_TIMEOUT);
    }

    return 0;
}

uint8_t lorawan_is_joined(void)
{
    return g_joined;
}

void lorawan_process_downlink(void)
{
    /* Check for pending downlink messages.
       In a full implementation, this would parse the RX buffer and
       process MAC commands. */
}

int16_t lorawan_last_rssi(void) { return g_last_rssi; }
int8_t  lorawan_last_snr(void)  { return g_last_snr; }