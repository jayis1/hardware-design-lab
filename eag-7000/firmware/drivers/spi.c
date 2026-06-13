/*
 * EAG-7000 — SPI Driver (ECSPI2/3) for Cortex-M4F
 *
 * Low-level SPI master driver for the i.MX8M Plus ECSPI peripheral.
 * Supports ECSPI2 (CAN-FD #0) and ECSPI3 (CAN-FD #1).
 *
 * Copyright (c) 2026 EAG-7000 Project
 * SPDX-License-Identifier: MIT
 */

#include "spi.h"
#include "registers.h"

/* ============================================================================
 * SPI Transfer (blocking, polled)
 * ============================================================================ */

int spi_init(spi_dev_t *dev, uintptr_t base_addr, uint8_t mode,
             uint32_t clock_hz, uint8_t cs)
{
    dev->base = base_addr;
    dev->mode = mode;
    dev->clock_hz = clock_hz;
    dev->cs = cs;

    uint32_t reg;

    /* Soft reset the ECSPI module */
    mmio_write32(dev->base + ECSPI_CONREG, 0);
    for (volatile int i = 0; i < 100; i++);  /* Wait for reset */

    /* Configure ECSPI:
     * - Burst length: 8 bits (default, can be changed per transfer)
     * - Master mode
     * - Channel select via SS signal
     * - Clock phase and polarity from mode parameter
     */
    reg = ECSPI_CONREG_EN;  /* Enable SPI */

    /* Set burst length to 8 bits */
    reg |= (7U << ECSPI_CONREG_BURST_LEN_SHIFT);  /* 8 bits = (8-1) = 7 */

    /* Start Mode Control — master initiated transfers */
    reg |= ECSPI_CONREG_SMC;

    /* Channel 0 select — active low by default */
    mmio_write32(dev->base + ECSPI_CONREG, reg);

    /* Configure clock phase, polarity, and SS polarity per channel */
    uint32_t config = 0;
    uint32_t phase = 0, pol = 0;

    switch (mode) {
    case SPI_MODE_0:  /* CPOL=0, CPHA=0 */
        phase = 0; pol = 0;
        break;
    case SPI_MODE_1:  /* CPOL=0, CPHA=1 */
        phase = 1; pol = 0;
        break;
    case SPI_MODE_2:  /* CPOL=1, CPHA=0 */
        phase = 0; pol = 1;
        break;
    case SPI_MODE_3:  /* CPOL=1, CPHA=1 */
        phase = 1; pol = 1;
        break;
    default:
        return -1;
    }

    /* Set SCLKPHA and SCLKPOL for the selected channel */
    config |= (phase << (ECSPI_CONFIGREG_SCLKPHA_SHIFT + cs));
    config |= (pol << (ECSPI_CONFIGREG_SCLKPOL_SHIFT + cs));

    /* SS polarity: active low for selected channel */
    config &= ~(1U << (ECSPI_CONFIGREG_SSPOL_SHIFT + cs));

    mmio_write32(dev->base + ECSPI_CONFIGREG, config);

    /* Configure clock divider:
     * SPI clock source = PERCLK (66 MHz default for M4)
     * Divide to get target clock: prescaler = source / target
     */
    uint32_t source_clk = BOARD_PERCLK_FREQ_HZ;
    uint32_t div = source_clk / clock_hz;
    if (div < 1) div = 1;
    if (div > 4096) div = 4096;

    /* Period register: pre-divider */
    mmio_write32(dev->base + ECSPI_PERIODREG, div - 1);

    return 0;
}

/**
 * Perform a single SPI transfer (8-bit word, blocking).
 * Asserts CS before transfer and de-asserts after.
 *
 * @param dev   SPI device handle
 * @param tx_data  Data to transmit
 * @param rx_buf   Buffer for received data (can be NULL for write-only)
 * @param len      Number of bytes to transfer
 * @return 0 on success, negative on error
 */
int spi_transfer(spi_dev_t *dev, const uint8_t *tx_data,
                  uint8_t *rx_buf, uint16_t len)
{
    uint16_t i;
    uint32_t reg;

    if (len == 0 || len > 512)
        return -1;

    /* Set burst length to 8 bits for byte-by-byte transfer */
    reg = mmio_read32(dev->base + ECSPI_CONREG);
    reg &= ~(0xFFFU << ECSPI_CONREG_BURST_LEN_SHIFT);
    reg |= (7U << ECSPI_CONREG_BURST_LEN_SHIFT);  /* 8 bits - 1 */
    mmio_write32(dev->base + ECSPI_CONREG, reg);

    /* Enable the exchange (start transfer) */
    reg = mmio_read32(dev->base + ECSPI_CONREG);
    reg |= ECSPI_CONREG_XCH;
    mmio_write32(dev->base + ECSPI_CONREG, reg);

    for (i = 0; i < len; i++) {
        /* Wait for TX FIFO to have space */
        uint32_t timeout = 10000;
        while (!(mmio_read32(dev->base + ECSPI_STATREG) & ECSPI_STATREG_TE)) {
            if (--timeout == 0) return -2;
        }

        /* Write data to TX FIFO */
        uint32_t tx_word = tx_data ? tx_data[i] : 0xFF;
        mmio_write32(dev->base + ECSPI_TXDATA, tx_word);

        /* Wait for RX FIFO to have data */
        timeout = 10000;
        while (!(mmio_read32(dev->base + ECSPI_STATREG) & ECSPI_STATREG_RR)) {
            if (--timeout == 0) return -3;
        }

        /* Read data from RX FIFO */
        if (rx_buf) {
            rx_buf[i] = (uint8_t)(mmio_read32(dev->base + ECSPI_RXDATA) & 0xFF);
        } else {
            mmio_read32(dev->base + ECSPI_RXDATA);  /* Discard */
        }
    }

    /* Wait for transfer complete */
    uint32_t timeout = 10000;
    while (!(mmio_read32(dev->base + ECSPI_STATREG) & ECSPI_STATREG_TC)) {
        if (--timeout == 0) return -4;
    }

    return 0;
}

/**
 * Write-only SPI transfer (no read data needed).
 */
int spi_write(spi_dev_t *dev, const uint8_t *data, uint16_t len)
{
    return spi_transfer(dev, data, NULL, len);
}

/**
 * Read-only SPI transfer (transmits dummy bytes 0xFF).
 */
int spi_read(spi_dev_t *dev, uint8_t *buf, uint16_t len)
{
    return spi_transfer(dev, NULL, buf, len);
}