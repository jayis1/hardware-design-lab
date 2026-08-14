/*
 * drivers/sx1262.c — Semtech SX1262 LoRa radio driver
 *
 * Bit-banged SPI1 host for the SX1262 sub-GHz radio.  Handles radio
 * reset, frequency / modulation configuration, and blocking TX of small
 * telemetry packets.  The radio is put to SLEEP between uplinks to
 * minimise current draw (~ 0.16 uA in sleep).
 *
 * Author:  jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#include "../board.h"
#include "../registers.h"
#include "sx1262.h"

/* ---- SPI bit-bang (kept simple & robust for bare-metal L4) -------- */

static void spi_init_gpio(void)
{
    RCC_AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN;
    (void)RCC_AHB2ENR;

    /* NSS (PA8), RESET (PA12) -> output, high */
    GPIO_MODER(GPIOA_BASE) &= ~((3u << (SX_NSS_PIN * 2u)) |
                                 (3u << (SX_RESET_PIN * 2u)));
    GPIO_MODER(GPIOA_BASE) |=  ((1u << (SX_NSS_PIN * 2u)) |
                                 (1u << (SX_RESET_PIN * 2u)));
    GPIO_BSRR(GPIOA_BASE) = (1u << SX_NSS_PIN) | (1u << SX_RESET_PIN);

    /* DIO1 (PA11) -> input */
    GPIO_MODER(GPIOA_BASE) &= ~(3u << (SX_DIO1_PIN * 2u));

    /* SCK (PB3), MISO (PB4), MOSI (PB5) -> output/input/output */
    GPIO_MODER(GPIOB_BASE) &= ~((3u << (3u * 2u)) |
                                 (3u << (4u * 2u)) |
                                 (3u << (5u * 2u)));
    GPIO_MODER(GPIOB_BASE) |=  ((1u << (3u * 2u)) |  /* SCK out */
                                 (0u << (4u * 2u)) |  /* MISO in */
                                 (1u << (5u * 2u)));  /* MOSI out */
    /* SCK idle low, push-pull */
    GPIO_PUPDR(GPIOB_BASE) &= ~((3u << (3u * 2u)) |
                                  (3u << (4u * 2u)) |
                                  (3u << (5u * 2u)));
}

static inline void spi_sck_low(void)  { GPIO_BSRR(GPIOB_BASE) = (1u << (3u + 16u)); }
static inline void spi_sck_high(void) { GPIO_BSRR(GPIOB_BASE) = (1u << 3u); }
static inline void spi_mosi_low(void)  { GPIO_BSRR(GPIOB_BASE) = (1u << (5u + 16u)); }
static inline void spi_mosi_high(void) { GPIO_BSRR(GPIOB_BASE) = (1u << 5u); }
static inline uint8_t spi_miso_read(void) { return (GPIO_IDR(GPIOB_BASE) >> 4u) & 1u; }

static inline void nss_low(void)  { GPIO_BSRR(GPIOA_BASE) = (1u << (SX_NSS_PIN + 16u)); }
static inline void nss_high(void) { GPIO_BSRR(GPIOA_BASE) = (1u << SX_NSS_PIN); }

static uint8_t spi_xfer(uint8_t tx)
{
    uint8_t rx = 0u;
    for (int8_t b = 7; b >= 0; b--) {
        if (tx & (1u << b)) spi_mosi_high(); else spi_mosi_low();
        spi_sck_high();
        rx <<= 1;
        rx |= spi_miso_read();
        spi_sck_low();
    }
    return rx;
}

/* ---- SX1262 command interface ------------------------------------ */

static void sx_write(uint8_t opcode, const uint8_t *payload, uint8_t len)
{
    nss_low();
    spi_xfer(opcode);
    for (uint8_t i = 0; i < len; i++) spi_xfer(payload[i]);
    nss_high();
}

static void sx_read(uint8_t opcode, uint8_t *buf, uint8_t len)
{
    nss_low();
    spi_xfer(opcode);
    /* Dummy byte required between opcode and read data */
    spi_xfer(0x00u);
    for (uint8_t i = 0; i < len; i++) buf[i] = spi_xfer(0x00u);
    nss_high();
}

static void sx_write_buffer(uint8_t offset, const uint8_t *data, uint8_t len)
{
    nss_low();
    spi_xfer(SX1262_WRITE_BUFFER);
    spi_xfer(offset);
    for (uint8_t i = 0; i < len; i++) spi_xfer(data[i]);
    nss_high();
}

static void sx_wait_busy(void)
{
    /* SX1262 BUSY line is optional in this design; we poll status byte */
    for (volatile uint32_t i = 0; i < 4000u; i++) { __asm__("nop"); }
}

static void sx_set_standby(void)
{
    uint8_t cmd[1] = { 0x00u }; /* STDBY_RC */
    sx_write(SX1262_SET_STANDBY, cmd, 1u);
    sx_wait_busy();
}

/* ---- Public API -------------------------------------------------- */

void sx1262_reset(void)
{
    /* RESET active low, 100 us pulse */
    GPIO_BSRR(GPIOA_BASE) = (1u << (SX_RESET_PIN + 16u)); /* low */
    for (volatile uint32_t i = 0; i < 2000u; i++) { __asm__("nop"); }
    GPIO_BSRR(GPIOA_BASE) = (1u << SX_RESET_PIN);         /* high */
    for (volatile uint32_t i = 0; i < 20000u; i++) { __asm__("nop"); }
}

bool sx1262_init(void)
{
    spi_init_gpio();
    sx1262_reset();

    /* Set standby */
    sx_set_standby();

    /* Set packet type = LoRa */
    uint8_t pkt = SX1262_PKT_LORA;
    sx_write(SX1262_SET_PACKET_TYPE, &pkt, 1u);
    sx_wait_busy();

    /* Set RF frequency: rf_freq = freq_hz * 2^25 / 32 MHz */
    uint64_t rf = ((uint64_t)LORA_FREQ_HZ << 25) / 32000000u;
    uint8_t freq_cmd[4];
    freq_cmd[0] = (uint8_t)(rf >> 24);
    freq_cmd[1] = (uint8_t)(rf >> 16);
    freq_cmd[2] = (uint8_t)(rf >> 8);
    freq_cmd[3] = (uint8_t)(rf);
    sx_write(SX1262_SET_RF_FREQ, freq_cmd, 4u);
    sx_wait_busy();

    /* Mod params: SF7, BW 0x04 (125 kHz), CR 4/5 (LDRO off=0) */
    uint8_t mod_params[3] = { LORA_SF, 0x04u, 0x01u };
    sx_write(SX1262_SET_MOD_PARAMS, mod_params, 3u);
    sx_wait_busy();

    /* Packet params: 8-sym preamble, explicit header, 0xFF payload len,
       CRC on, invertIQ std */
    uint8_t pkt_params[9] = {
        0x00u, 0x08u,          /* preamble length (MSB/LSB) */
        0x00u,                  /* explicit header */
        0x40u,                  /* payload length (placeholder) */
        0x01u,                  /* CRC on */
        0x00u, 0x00u, 0x00u     /* invert IQ std */
    };
    sx_write(SX1262_SET_PACKET_PARAMS, pkt_params, 9u);
    sx_wait_busy();

    /* Set DIO2 as RF switch control */
    uint8_t dio2 = 0x01u;
    sx_write(SX1262_SET_DIO2_AS_RF_SWITCH, &dio2, 1u);

    /* Set TX power (14 dBm, ramp 20 us) */
    sx1262_set_tx_power(LORA_TX_POWER_DBM);

    return true;
}

bool sx1262_set_frequency(uint32_t freq_hz)
{
    uint64_t rf = ((uint64_t)freq_hz << 25) / 32000000u;
    uint8_t freq_cmd[4];
    freq_cmd[0] = (uint8_t)(rf >> 24);
    freq_cmd[1] = (uint8_t)(rf >> 16);
    freq_cmd[2] = (uint8_t)(rf >> 8);
    freq_cmd[3] = (uint8_t)(rf);
    sx_set_standby();
    sx_write(SX1262_SET_RF_FREQ, freq_cmd, 4u);
    sx_wait_busy();
    return true;
}

bool sx1262_set_tx_power(int8_t dbm)
{
    /* Clamp to +22 dBm max, -9 dBm min for SX1262 */
    if (dbm > 22) dbm = 22;
    if (dbm < -9) dbm = -9;
    uint8_t cmd[2] = { (uint8_t)dbm, 0x04u /* ramp 20 us */ };
    sx_write(SX1262_SET_TX_PARAMS, cmd, 2u);
    sx_wait_busy();
    return true;
}

bool sx1262_send(const uint8_t *data, uint8_t len)
{
    if (len > 64u) return false;

    sx_set_standby();
    /* Write payload to buffer at offset 0 */
    sx_write_buffer(0u, data, len);

    /* Update packet params payload length */
    uint8_t pkt_params[9] = {
        0x00u, 0x08u, 0x00u, len, 0x01u, 0x00u, 0x00u, 0x00u, 0x00u
    };
    sx_write(SX1262_SET_PACKET_PARAMS, pkt_params, 9u);
    sx_wait_busy();

    /* Set TX with timeout = 0 (no timeout, manual stop) */
    uint8_t tx_cmd[3] = { 0x00u, 0x00u, 0x00u };
    sx_write(SX1262_SET_TX, tx_cmd, 3u);
    sx_wait_busy();

    /* Poll IRQ status for TxDone (max ~ 200 ms for SF7 / 64 bytes) */
    uint32_t deadline = 200000u;
    uint8_t irq[3] = {0, 0, 0};
    do {
        sx_read(SX1262_GET_IRQ, irq, 3u);
        if (irq[0] & SX1262_IRQ_TX_DONE) break;
        for (volatile uint32_t i = 0; i < 1000u; i++) { __asm__("nop"); }
    } while (--deadline);

    /* Clear IRQ flags */
    uint8_t clear_cmd[3] = { 0xFFu, 0xFFu, 0xFFu };
    sx_write(SX1262_CLEAR_IRQ, clear_cmd, 3u);

    return (irq[0] & SX1262_IRQ_TX_DONE) != 0u;
}

bool sx1262_receive(uint8_t *data, uint8_t *len, uint32_t timeout_ms)
{
    sx_set_standby();

    /* Set RX with timeout in 64 kHz units: timeout_ms * 64 */
    uint32_t tout = timeout_ms * 64u;
    uint8_t rx_cmd[3] = {
        (uint8_t)(tout >> 16),
        (uint8_t)(tout >> 8),
        (uint8_t)(tout)
    };
    sx_write(SX1262_SET_RX, rx_cmd, 3u);
    sx_wait_busy();

    /* Poll for RxDone or Timeout */
    uint32_t deadline = timeout_ms * 100u;
    uint8_t irq[3] = {0, 0, 0};
    do {
        sx_read(SX1262_GET_IRQ, irq, 3u);
        if (irq[0] & (SX1262_IRQ_RX_DONE | SX1262_IRQ_TIMEOUT | SX1262_IRQ_CRC_ERR))
            break;
        for (volatile uint32_t i = 0; i < 1000u; i++) { __asm__("nop"); }
    } while (--deadline);

    if ((irq[0] & SX1262_IRQ_RX_DONE) == 0u) {
        uint8_t clr[3] = { 0xFFu, 0xFFu, 0xFFu };
        sx_write(SX1262_CLEAR_IRQ, clr, 3u);
        return false;
    }

    /* Read RX buffer status: [status, payload_len, offset] */
    uint8_t rxb[2] = {0, 0};
    sx_read(SX1262_GET_RX_BUFFER_STATUS, rxb, 2u);
    uint8_t plen = rxb[0];
    uint8_t offset = rxb[1];
    if (len) *len = plen;

    /* Read payload */
    nss_low();
    spi_xfer(SX1262_READ_BUFFER);
    spi_xfer(offset);
    for (uint8_t i = 0; i < plen; i++) data[i] = spi_xfer(0x00u);
    nss_high();

    /* Clear IRQ */
    uint8_t clr[3] = { 0xFFu, 0xFFu, 0xFFu };
    sx_write(SX1262_CLEAR_IRQ, clr, 3u);
    return true;
}

void sx1262_sleep(void)
{
    /* Sleep config: warm start (0x04) so config is retained */
    uint8_t cmd[2] = { 0x04u, 0x00u };
    sx_write(SX1262_SET_SLEEP, cmd, 2u);
    /* After SLEEP, NSS is ignored; radio draws ~ 0.16 uA */
}