/*
 * flash_drv.c — W25Q64 external SPI flash driver
 * BreathPrint — Portable Breath VOC Analyzer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 *
 * Manages the W25Q64 (8 MB) external SPI flash used for:
 *   - ML model storage (offset 0x000000, ~128 KB)
 *   - Calibration data (offset 0x020000, 256 bytes)
 *   - Device configuration (offset 0x021000)
 *   - Session metadata (offset 0x022000)
 */

#include "flash_drv.h"
#include "../board.h"

/* ========================================================================
 * Flash memory map
 * ========================================================================
 *
 * 0x000000 - 0x01FFFF  ML Model (128 KB, 2 sectors)
 * 0x020000 - 0x020FFF  Calibration data (4 KB, 1 sector)
 * 0x021000 - 0x021FFF  Device config (4 KB)
 * 0x022000 - 0x022FFF  Session metadata (4 KB)
 * 0x023000 - 0x7FFFFF  Reserved / future use
 *
 * W25Q64: 8 MB = 128 sectors × 64 KB, or 2048 blocks × 4 KB
 * ======================================================================== */

#define FLASH_CALIB_ADDR     0x020000
#define FLASH_CONFIG_ADDR    0x021000
#define FLASH_SESSION_ADDR   0x022000

/* ========================================================================
 * SPI4 low-level flash functions
 * ======================================================================== */

static void flash_spi_select(void) {
    gpio_clear(FLASH_CS_PORT, FLASH_CS_PIN);
}

static void flash_spi_deselect(void) {
    gpio_set(FLASH_CS_PORT, FLASH_CS_PIN);
}

static uint8_t flash_spi_xfer(uint8_t data) {
    SPI4->CR1 |= SPI_CR1_CSTART;
    uint32_t start = millis();
    while (!(SPI4->SR & SPI_SR_RXP)) {
        if (millis() - start > 100) return 0xFF;
    }
    return (uint8_t)SPI4->RXDR;
}

static void flash_spi_send(uint8_t data) {
    (void)flash_spi_xfer(data);
}

static uint8_t flash_spi_recv(void) {
    return flash_spi_xfer(0xFF);
}

static void flash_wait_busy(void) {
    flash_spi_select();
    flash_spi_send(W25Q_CMD_RDSR);
    uint32_t start = millis();
    while (flash_spi_recv() & W25Q_SR_WIP) {
        if (millis() - start > 3000) break;  /* 3s timeout */
    }
    flash_spi_deselect();
}

static void flash_write_enable(void) {
    flash_spi_select();
    flash_spi_send(W25Q_CMD_WREN);
    flash_spi_deselect();
}

/* ========================================================================
 * Initialize flash SPI interface
 * ========================================================================
 */

void flash_spi_init(void) {
    /* SPI4 already configured in gpio_init */
    SPI4->CFG2 = SPI_CFG2_MASTER | SPI_CFG2_SSM | SPI_CFG2_SSI |
                 SPI_CFG2_MBR_DIV4;  /* 312/4 = 78 MHz (W25Q64 supports 104 MHz) */
    SPI4->CFG1 = (8U << SPI_CFG1_DSIZE_SHIFT);
    SPI4->CR1 |= SPI_CR1_SPE;

    /* Wake up flash from power-down */
    flash_spi_select();
    flash_spi_send(W25Q_CMD_PWR_UP);
    flash_spi_deselect();
    delay_ms(1);

    /* Verify flash ID */
    uint8_t id[3];
    flash_read_id(id);
    /* Expected: 0xEF 0x40 0x17 (Winbond W25Q64) */
    if (id[0] != 0xEF || id[1] != 0x40) {
        /* Flash not detected — non-fatal, model will use builtin */
    }
}

/* ========================================================================
 * Read flash ID
 * ========================================================================
 */

void flash_read_id(uint8_t id[3]) {
    flash_spi_select();
    flash_spi_send(W25Q_CMD_RDID);
    id[0] = flash_spi_recv();
    id[1] = flash_spi_recv();
    id[2] = flash_spi_recv();
    flash_spi_deselect();
}

/* ========================================================================
 * Read data from flash
 * ========================================================================
 */

int flash_read(uint32_t addr, uint8_t *data, uint32_t len) {
    if (addr + len > 0x800000) return -1;  /* Out of bounds */

    flash_spi_select();
    flash_spi_send(W25Q_CMD_FAST_READ);
    flash_spi_send((addr >> 16) & 0xFF);
    flash_spi_send((addr >> 8) & 0xFF);
    flash_spi_send(addr & 0xFF);
    flash_spi_send(0x00);  /* Dummy byte for fast read */

    for (uint32_t i = 0; i < len; i++) {
        data[i] = flash_spi_recv();
    }

    flash_spi_deselect();
    return 0;
}

/* ========================================================================
 * Write data to flash (page program, max 256 bytes per write)
 * ========================================================================
 */

int flash_write(uint32_t addr, const uint8_t *data, uint32_t len) {
    if (addr + len > 0x800000) return -1;

    uint32_t offset = 0;
    while (offset < len) {
        /* Calculate bytes remaining in current page */
        uint32_t page_remaining = 256 - (addr % 256);
        uint32_t chunk = MIN(len - offset, page_remaining);

        flash_write_enable();

        flash_spi_select();
        flash_spi_send(W25Q_CMD_PAGE_PROG);
        flash_spi_send((addr >> 16) & 0xFF);
        flash_spi_send((addr >> 8) & 0xFF);
        flash_spi_send(addr & 0xFF);

        for (uint32_t i = 0; i < chunk; i++) {
            flash_spi_send(data[offset + i]);
        }

        flash_spi_deselect();
        flash_wait_busy();

        addr += chunk;
        offset += chunk;
    }

    return 0;
}

/* ========================================================================
 * Erase a 4 KB sector
 * ========================================================================
 */

int flash_erase_sector(uint32_t addr) {
    /* W25Q64 sector erase: 4 KB */
    flash_write_enable();

    flash_spi_select();
    flash_spi_send(W25Q_CMD_SECTOR_ER);
    flash_spi_send((addr >> 16) & 0xFF);
    flash_spi_send((addr >> 8) & 0xFF);
    flash_spi_send(addr & 0xFF);
    flash_spi_deselect();

    flash_wait_busy();
    return 0;
}

/* ========================================================================
 * Erase entire chip
 * ========================================================================
 */

int flash_erase_chip(void) {
    flash_write_enable();

    flash_spi_select();
    flash_spi_send(W25Q_CMD_CHIP_ER);
    flash_spi_deselect();

    flash_wait_busy();  /* Can take up to 20 seconds */
    return 0;
}

/* ========================================================================
 * CRC-32 (for calibration data integrity)
 * ========================================================================
 */

uint32_t flash_crc32(const uint8_t *data, uint32_t len) {
    uint32_t crc = 0xFFFFFFFF;
    for (uint32_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc ^ 0xFFFFFFFF;
}

/* ========================================================================
 * Read/write calibration data
 * ========================================================================
 */

int flash_read_calib(calib_data_t *calib) {
    uint8_t buf[sizeof(calib_data_t)];

    if (flash_read(FLASH_CALIB_ADDR, buf, sizeof(calib_data_t)) < 0) {
        return -1;
    }

    memcpy(calib, buf, sizeof(calib_data_t));

    /* Verify CRC */
    uint32_t expected_crc = flash_crc32(buf, sizeof(calib_data_t) - 4);
    if (calib->crc32 != expected_crc) {
        return -1;  /* CRC mismatch */
    }

    if (!calib->valid) {
        return -1;
    }

    return 0;
}

int flash_write_calib(const calib_data_t *calib) {
    /* Calculate CRC */
    calib_data_t calib_copy = *calib;
    calib_copy.crc32 = flash_crc32((uint8_t *)&calib_copy,
                                   sizeof(calib_data_t) - 4);

    /* Erase sector before writing */
    if (flash_erase_sector(FLASH_CALIB_ADDR) < 0) return -1;

    /* Write calibration data */
    return flash_write(FLASH_CALIB_ADDR, (uint8_t *)&calib_copy,
                       sizeof(calib_data_t));
}

/* ========================================================================
 * Read/write device configuration
 * ======================================================================== */

int flash_read_config(device_config_t *config) {
    uint8_t buf[sizeof(device_config_t)];
    if (flash_read(FLASH_CONFIG_ADDR, buf, sizeof(device_config_t)) < 0) {
        return -1;
    }
    memcpy(config, buf, sizeof(device_config_t));
    return 0;
}

int flash_write_config(const device_config_t *config) {
    if (flash_erase_sector(FLASH_CONFIG_ADDR) < 0) return -1;
    return flash_write(FLASH_CONFIG_ADDR, (uint8_t *)config,
                       sizeof(device_config_t));
}

/* ========================================================================
 * Power down flash for sleep mode
 * ========================================================================
 */

void flash_power_down(void) {
    flash_spi_select();
    flash_spi_send(W25Q_CMD_PWR_DN);
    flash_spi_deselect();
    delay_ms(1);
}

void flash_power_up(void) {
    flash_spi_select();
    flash_spi_send(W25Q_CMD_PWR_UP);
    flash_spi_deselect();
    delay_ms(1);
}