/*
 * EAG-7000 — SPI Driver Header
 *
 * Copyright (c) 2026 EAG-7000 Project
 * SPDX-License-Identifier: MIT
 */

#ifndef EAG7000_SPI_H
#define EAG7000_SPI_H

#include <stdint.h>

/* SPI mode definitions */
#define SPI_MODE_0   0   /* CPOL=0, CPHA=0 */
#define SPI_MODE_1   1   /* CPOL=0, CPHA=1 */
#define SPI_MODE_2   2   /* CPOL=1, CPHA=0 */
#define SPI_MODE_3   3   /* CPOL=1, CPHA=1 */

/* Chip select definitions */
#define SPI_CS0      0
#define SPI_CS1      1

/* SPI device handle */
typedef struct {
    uintptr_t base;        /* ECSPI base address */
    uint8_t    mode;        /* SPI mode (0-3) */
    uint32_t   clock_hz;   /* SPI clock frequency */
    uint8_t    cs;          /* Chip select index */
} spi_dev_t;

/**
 * Initialize SPI peripheral.
 * @param dev       SPI device handle
 * @param base_addr ECSPI base address (ECSPI2_BASE or ECSPI3_BASE)
 * @param mode      SPI mode (0-3)
 * @param clock_hz  Desired SPI clock frequency in Hz
 * @param cs        Chip select index (0 or 1)
 * @return 0 on success, negative on error
 */
int spi_init(spi_dev_t *dev, uintptr_t base_addr, uint8_t mode,
             uint32_t clock_hz, uint8_t cs);

/**
 * Perform a full-duplex SPI transfer.
 * @param dev      SPI device handle
 * @param tx_data  Data to transmit (can be NULL for read-only)
 * @param rx_buf   Buffer for received data (can be NULL for write-only)
 * @param len      Number of bytes to transfer
 * @return 0 on success, negative on error
 */
int spi_transfer(spi_dev_t *dev, const uint8_t *tx_data,
                  uint8_t *rx_buf, uint16_t len);

/**
 * Write-only SPI transfer.
 * @param dev   SPI device handle
 * @param data  Data to transmit
 * @param len   Number of bytes to transmit
 * @return 0 on success, negative on error
 */
int spi_write(spi_dev_t *dev, const uint8_t *data, uint16_t len);

/**
 * Read-only SPI transfer (transmits dummy 0xFF bytes).
 * @param dev  SPI device handle
 * @param buf  Buffer for received data
 * @param len  Number of bytes to receive
 * @return 0 on success, negative on error
 */
int spi_read(spi_dev_t *dev, uint8_t *buf, uint16_t len);

#endif /* EAG7000_SPI_H */