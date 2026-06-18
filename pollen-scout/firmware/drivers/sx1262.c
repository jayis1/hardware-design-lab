/*
 * sx1262.c - Semtech SX1262 sub-GHz LoRa radio driver (SPI1)
 * Minimal register-level driver for LoRaWAN class-A uplink.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 */
#include "sx1262.h"
#include "board.h"
#include "registers.h"
#include <string.h>

#define SX1262_CMD_SET_STANDBY     0x80
#define SX1262_CMD_SET_PKT_TYPE    0x01
#define SX1262_CMD_SET_RF_FREQ     0x86
#define SX1262_CMD_SET_TX_PARAMS   0x8E
#define SX1262_CMD_SET_MOD_PARAMS  0x8B
#define SX1262_CMD_SET_BUFFER_BASE 0x8F
#define SX1262_CMD_WRITE_BUFFER    0x0E
#define SX1262_CMD_SET_TX          0x83
#define SX1262_CMD_SET_RX          0x82
#define SX1262_CMD_GET_STATUS      0xC0
#define SX1262_CMD_CLEAR_IRQ       0x02
#define SX1262_CMD_SET_DIO_IRQ     0x08
#define SX1262_PKT_TYPE_LORA       0x01

static void cs_low(void)  { GPIO_REG(SX1262_CS_PORT, GPIO_BSRR_OFFSET) = (1U << (SX1262_CS_PIN + 16)); }
static void cs_high(void) { GPIO_REG(SX1262_CS_PORT, GPIO_BSRR_OFFSET) = (1U << SX1262_CS_PIN); }

static int busy(void)
{
    return (GPIO_REG(SX1262_BUSY_PORT, GPIO_IDR_OFFSET) >> SX1262_BUSY_PIN) & 1U;
}

static void spi_xfer(const uint8_t *tx, uint8_t *rx, int n)
{
    while (busy()) { }
    cs_low();
    for (int i = 0; i < n; i++) {
        while (!(SPI_SR(SX1262_SPI) & SPI_SR_TXE)) { }
        SPI_DR(SX1262_SPI) = tx[i];
        while (!(SPI_SR(SX1262_SPI) & SPI_SR_RXNE)) { }
        rx[i] = (uint8_t)SPI_DR(SX1262_SPI);
    }
    while (SPI_SR(SX1262_SPI) & SPI_SR_BSY) { }
    cs_high();
}

static void write_cmd(uint8_t cmd, const uint8_t *payload, int len)
{
    uint8_t tx[16] = {0}, rx[16] = {0};
    tx[0] = cmd;
    if (len > 0 && len <= 15) memcpy(&tx[1], payload, len);
    spi_xfer(tx, rx, len + 1);
}

static void write_reg(uint16_t addr, uint8_t val)
{
    uint8_t tx[4] = { 0x0D, (addr >> 8) & 0xFF, addr & 0xFF, val };
    uint8_t rx[4];
    spi_xfer(tx, rx, 4);
}

int sx1262_init(void)
{
    /* Configure SPI1: master, CPOL=0, CPHA=0, 8-bit, /16 baud -> 15 MHz */
    SPI_CR1(SX1262_SPI) = (1U << 2) | (0b11 << 3) | (1U << 0); /* MASTER, BR, SPE */

    /* CS as output, high idle */
    GPIO_REG(SX1262_CS_PORT, GPIO_MODER_OFFSET) &= ~(3U << (SX1262_CS_PIN * 2));
    GPIO_REG(SX1262_CS_PORT, GPIO_MODER_OFFSET) |=  (1U << (SX1262_CS_PIN * 2));
    cs_high();

    /* Hardware reset */
    GPIO_REG(SX1262_RST_PORT, GPIO_BSRR_OFFSET) = (1U << (SX1262_RST_PIN + 16));
    for (volatile int i = 0; i < 10000; i++) { }
    GPIO_REG(SX1262_RST_PORT, GPIO_BSRR_OFFSET) = (1U << SX1262_RST_PIN);
    for (volatile int i = 0; i < 50000; i++) { }

    /* Standby + LoRa packet type */
    uint8_t stby = 0x01;  /* STDBY_RC */
    write_cmd(SX1262_CMD_SET_STANDBY, &stby, 1);
    uint8_t pkt = SX1262_PKT_TYPE_LORA;
    write_cmd(SX1262_CMD_SET_PKT_TYPE, &pkt, 1);

    /* Clear all IRQs, enable TxDone + RxDone */
    uint8_t irq_mask[7] = { 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00 };
    write_cmd(SX1262_CMD_SET_DIO_IRQ, irq_mask, 7);
    uint8_t clr[3] = { 0xFF, 0xFF, 0xFF };
    write_cmd(SX1262_CMD_CLEAR_IRQ, clr, 3);

    return 0;
}

void sx1262_set_frequency(uint32_t hz)
{
    /* rf_freq = hz * 2^25 / 32 MHz */
    uint64_t frf = ((uint64_t)hz << 25) / 32000000ULL;
    uint8_t buf[4] = {
        (uint8_t)(frf >> 24), (uint8_t)(frf >> 16),
        (uint8_t)(frf >> 8),  (uint8_t)(frf)
    };
    write_cmd(SX1262_CMD_SET_RF_FREQ, buf, 4);
}

void sx1262_set_tx_power(int8_t dbm)
{
    /* power in 2's-complement, ramp time 200 µs */
    uint8_t buf[2] = { (uint8_t)dbm, 0x04 };
    write_cmd(SX1262_CMD_SET_TX_PARAMS, buf, 2);
}

void sx1262_set_lora_params(uint8_t sf, uint8_t bw, uint8_t cr)
{
    /* SF, BW (0=125k,1=250k,2=500k), CR (1..4), LDRO auto */
    uint8_t ldro = (sf >= 11) ? 1 : 0;
    uint8_t buf[4] = { sf, bw, cr, ldro };
    write_cmd(SX1262_CMD_SET_MOD_PARAMS, buf, 4);
}

int sx1262_send(const uint8_t *data, int len, int timeout_ms)
{
    if (len > 255) return -1;
    /* Set buffer base addr = 0 */
    uint8_t base[2] = { 0, 0 };
    write_cmd(SX1262_CMD_SET_BUFFER_BASE, base, 2);
    /* Write payload */
    uint8_t buf[256] = { SX1262_CMD_WRITE_BUFFER, 0 };
    memcpy(&buf[2], data, len);
    while (busy()) { }
    cs_low();
    for (int i = 0; i < len + 2; i++) {
        while (!(SPI_SR(SX1262_SPI) & SPI_SR_TXE)) { }
        SPI_DR(SX1262_SPI) = buf[i];
        while (!(SPI_SR(SX1262_SPI) & SPI_SR_RXNE)) { }
        (void)SPI_DR(SX1262_SPI);
    }
    while (SPI_SR(SX1262_SPI) & SPI_SR_BSY) { }
    cs_high();

    /* Set TX with timeout */
    uint32_t tout = (uint32_t)timeout_ms * 64; /* 15.625 µs units */
    uint8_t tx_cmd[3] = {
        (uint8_t)(tout >> 16), (uint8_t)(tout >> 8), (uint8_t)tout
    };
    write_cmd(SX1262_CMD_SET_TX, tx_cmd, 3);
    return 0;
}

void sx1262_receive(uint32_t timeout_ms)
{
    uint32_t tout = timeout_ms * 64;
    uint8_t rx_cmd[3] = {
        (uint8_t)(tout >> 16), (uint8_t)(tout >> 8), (uint8_t)tout
    };
    write_cmd(SX1262_CMD_SET_RX, rx_cmd, 3);
}

void sx1262_sleep(void)
{
    uint8_t stby = 0x00;
    write_cmd(SX1262_CMD_SET_STANDBY, &stby, 1);
}