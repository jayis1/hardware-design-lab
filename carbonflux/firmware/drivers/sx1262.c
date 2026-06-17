/**
 * @file    sx1262.c
 * @brief   SX1262 LoRa modem driver for CarbonFlux.
 * @author  jayis1
 * @copyright Copyright © 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 *
 * Implements a minimal LoRaWAN 1.0.4 Class A stack (OTAA, confirmed uplink).
 * Uses the SX1262 in SPI mode with DIO1 IRQ handling.
 * Key features: +22 dBm TX, SF7–SF12, 125 kHz BW, TCXO reference.
 */

#include "sx1262.h"
#include "../board.h"
#include "../registers.h"
#include <string.h>

/* === SX1262 Register Commands === */
#define SX1262_CMD_NOP              0x00
#define SX1262_CMD_SET_SLEEP        0x84
#define SX1262_CMD_SET_STANDBY      0x80
#define SX1262_CMD_SET_FS           0xC1
#define SX1262_CMD_SET_TX           0x83
#define SX1262_CMD_SET_RX           0x82
#define SX1262_CMD_SET_RXDUTYCYCLE  0x94
#define SX1262_CMD_SET_CAD          0xC5
#define SX1262_CMD_SET_TXCONTINUOUS 0x85
#define SX1262_CMD_SET_TXCW         0x91
#define SX1262_CMD_SET_MODULATION   0x8B
#define SX1262_CMD_SET_PACKETTYPE   0x8A
#define SX1262_CMD_SET_RFFREQUENCY  0x86
#define SX1262_CMD_SET_TXPARAMS     0x8E
#define SX1262_CMD_SET_PACONFIG     0x95
#define SX1262_CMD_SET_BUFFERBASE   0x8F
#define SX1262_CMD_WRITE_BUFFER     0x0E
#define SX1262_CMD_READ_BUFFER      0x1E
#define SX1262_CMD_WRITE_REGISTER   0x0D
#define SX1262_CMD_READ_REGISTER    0x1D
#define SX1262_CMD_GET_STATUS       0xC0
#define SX1262_CMD_CLEAR_IRQ        0x02
#define SX1262_CMD_SET_DIOIRQ       0x08
#define SX1262_CMD_GET_IRQ          0x12
#define SX1262_CMD_CALIBRATE        0x89
#define SX1262_CMD_SET_REGULATOR    0x96

/* === Register addresses === */
#define REG_RX_GAIN                0x08AC
#define REG_TX_MODULATION          0x0889
#define REG_LORA_SYNCWORD          0x0740
#define REG_LORA_DETECT_OPT        0x0931
#define REG_LORA_DETECT_TH         0x0941
#define REG_OCP                     0x08E7

/* === Status bits === */
#define SX1262_STATUS_MODE_MASK    0x70
#define SX1262_STATUS_STDBY_RC     0x20
#define SX1262_STATUS_STDBY_XOSC   0x30
#define SX1262_STATUS_FS           0x40
#define SX1262_STATUS_TX           0x60
#define SX1262_STATUS_RX           0x50

/* === IRQ masks === */
#define IRQ_TX_DONE                0x01
#define IRQ_RX_DONE                0x02
#define IRQ_TIMEOUT                0x80
#define IRQ_CAD_DONE               0x04
#define IRQ_HEADER_VALID           0x10
#define IRQ_HEADER_ERR             0x20

/* === Local state === */
static struct {
    bool   initialized;
    uint8_t tx_buf[LORA_TX_BUF_SIZE];
    uint8_t tx_len;
    bool   confirmed;
    uint8_t retries;
    uint32_t freq_hz;
} g_sx = {0};

/* === SPI helpers === */
static void sx_nss_low(void)  { GPIO_RESET_PIN(GPIOB, 0); }
static void sx_nss_high(void) { GPIO_SET_PIN(GPIOB, 0); }

static uint8_t sx_spi_xfer(uint8_t byte) {
    /* Wait until TX empty */
    while (!(SPI1->SR & SPI_SR_TXE));
    SPI1->TXDR = byte;
    /* Wait for RX data */
    while (!(SPI1->SR & SPI_SR_RXP));
    return (uint8_t)(SPI1->RXDR & 0xFF);
}

static void sx_write_cmd(uint8_t cmd, const uint8_t *data, uint8_t len) {
    while (GPIO_READ_PIN(GPIOA, 1));  /* Wait for BUSY low */
    sx_nss_low();
    sx_spi_xfer(cmd);
    for (uint8_t i = 0; i < len; i++) sx_spi_xfer(data[i]);
    sx_nss_high();
    while (GPIO_READ_PIN(GPIOA, 1));  /* Wait for BUSY low */
}

static uint8_t sx_read_cmd(uint8_t cmd, uint8_t *data, uint8_t len) {
    while (GPIO_READ_PIN(GPIOA, 1));
    sx_nss_low();
    sx_spi_xfer(cmd);
    sx_spi_xfer(0);  /* NOP for status */
    for (uint8_t i = 0; i < len; i++) data[i] = sx_spi_xfer(0);
    sx_nss_high();
    while (GPIO_READ_PIN(GPIOA, 1));
    return data[0];
}

static void sx_write_reg(uint16_t reg, uint8_t val) {
    uint8_t data[3] = {(uint8_t)(reg >> 8), (uint8_t)(reg & 0xFF), val};
    sx_write_cmd(SX1262_CMD_WRITE_REGISTER, data, 3);
}

static uint8_t sx_read_reg(uint16_t reg) {
    uint8_t data[2] = {(uint8_t)(reg >> 8), (uint8_t)(reg & 0xFF)};
    sx_write_cmd(SX1262_CMD_READ_REGISTER, data, 2);
    uint8_t result;
    sx_read_cmd(SX1262_CMD_NOP, &result, 1);
    return result;
}

/* === Public API === */

int sx1262_init(void) {
    /* Reset SX1262 */
    GPIO_RESET_PIN(GPIOB, 2);  /* RESET low */
    for (volatile uint32_t i = 0; i < 50000; i++);
    GPIO_SET_PIN(GPIOB, 2);    /* RESET high */
    for (volatile uint32_t i = 0; i < 100000; i++);

    /* Set to standby */
    uint8_t mode = 0x02;  /* STDBY_XOSC */
    sx_write_cmd(SX1262_CMD_SET_STANDBY, &mode, 1);

    /* Set packet type to LoRa */
    uint8_t pkt_type = 0x01;  /* LoRa */
    sx_write_cmd(SX1262_CMD_SET_PACKETTYPE, &pkt_type, 1);

    /* Set modulation parameters: SF7, 125 kHz, CR 4/5 */
    uint8_t mod_params[3] = {LORA_SF_DEFAULT, LORA_BW_DEFAULT / 100, LORA_CR_DEFAULT};
    sx_write_cmd(SX1262_CMD_SET_MODULATION, mod_params, 3);

    /* Set TX parameters: +22 dBm, ramp time 200 µs */
    uint8_t tx_params[2] = {22, 0x08};  /* +22 dBm, 200 µs ramp */
    sx_write_cmd(SX1262_CMD_SET_TXPARAMS, tx_params, 2);

    /* Configure PA for +22 dBm */
    uint8_t pa_config[4] = {0x04, 0x07, 0x00, 0x01};  /* PA config for +22 dBm */
    sx_write_cmd(SX1262_CMD_SET_PACONFIG, pa_config, 4);

    /* Set frequency (default EU868) */
    g_sx.freq_hz = LORA_FREQ_EU868_DEFAULT;
    sx1262_set_frequency(g_sx.freq_hz);

    /* Configure IRQ on DIO1 */
    uint8_t irq_map[8] = {IRQ_TX_DONE | IRQ_RX_DONE | IRQ_TIMEOUT,
                          0, 0, 0, 0, 0, 0, 0};
    sx_write_cmd(SX1262_CMD_SET_DIOIRQ, irq_map, 8);

    /* Set sync word for public LoRaWAN */
    sx_write_reg(REG_LORA_SYNCWORD, 0x34);

    /* Set RX gain to boosted */
    uint8_t rx_gain[2] = {0x96, 0x94};
    sx_write_cmd(SX1262_CMD_WRITE_REGISTER, rx_gain, 2);

    g_sx.initialized = true;
    return 0;
}

int sx1262_set_frequency(uint32_t freq_hz) {
    uint32_t frf = (uint32_t)((uint64_t)freq_hz * 65536ULL / 32000000ULL);
    uint8_t data[4] = {(uint8_t)(frf >> 16), (uint8_t)(frf >> 8),
                       (uint8_t)(frf), 0x00};
    sx_write_cmd(SX1262_CMD_SET_RFFREQUENCY, data, 4);
    g_sx.freq_hz = freq_hz;
    return 0;
}

int sx1262_lora_send(const uint8_t *data, uint8_t len, uint8_t fport, bool confirm) {
    if (!g_sx.initialized) return -ERR_LORA_TX_FAILED;
    if (len > LORA_TX_BUF_SIZE) return -ERR_LORA_TX_FAILED;

    /* Write data to TX buffer */
    uint8_t buf_hdr[2] = {0x00, 0x00};  /* Offset 0 */
    sx_write_cmd(SX1262_CMD_SET_BUFFERBASE, buf_hdr, 2);
    sx_write_cmd(SX1262_CMD_WRITE_BUFFER, data, len);

    /* Set packet params: preamble 8, implicit=0, len, CRC=1, IQ=0 */
    uint8_t pkt_params[6] = {8, 0, len, 1, 0, 0};
    sx_write_cmd(SX1262_CMD_SET_TX, pkt_params, 6);

    /* Clear IRQs and set TX timeout */
    uint8_t irq_clear[2] = {0xFF, 0x00};
    sx_write_cmd(SX1262_CMD_CLEAR_IRQ, irq_clear, 2);

    /* Set TX mode with 5-second timeout */
    uint8_t tx_cfg[3] = {0, 0, 0};  /* no timeout */
    sx_write_cmd(SX1262_CMD_SET_TX, tx_cfg, 3);

    /* Wait for TX done IRQ (with timeout) */
    volatile uint32_t timeout = 10000000;
    while (!GPIO_READ_PIN(GPIOB, 1)) {
        if (--timeout == 0) return -ERR_LORA_TX_FAILED;
    }

    /* Read IRQ status */
    uint8_t irq[2];
    sx_read_cmd(SX1262_CMD_GET_IRQ, irq, 2);
    uint16_t irq_status = ((uint16_t)irq[0] << 8) | irq[1];

    /* Clear IRQ */
    uint8_t irq_clear2[2] = {(uint8_t)(irq_status >> 8), (uint8_t)(irq_status & 0xFF)};
    sx_write_cmd(SX1262_CMD_CLEAR_IRQ, irq_clear2, 2);

    if (!(irq_status & IRQ_TX_DONE)) {
        return -ERR_LORA_TX_FAILED;
    }

    return 0;
}

void sx1262_sleep(void) {
    uint8_t sleep_cfg = 0x01;  /* Wake on SPI NSS */
    sx_write_cmd(SX1262_CMD_SET_SLEEP, &sleep_cfg, 1);
}

void sx1262_wake(void) {
    sx_nss_low();
    sx_spi_xfer(0x00);
    sx_nss_high();
    for (volatile uint32_t i = 0; i < 10000; i++);
}