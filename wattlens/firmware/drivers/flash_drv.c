/*
 * flash_drv.c — QSPI flash and calibration storage
 * WattLens — 3-Phase Power Quality Analyzer & NILM
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 *
 * Uses a W25Q128 external QSPI flash (16 MB) for:
 *   - Calibration data (256 bytes at offset 0x000000)
 *   - NILM model weights (~200 KB at offset 0x001000)
 *   - Appliance names (256 bytes at offset 0x040000)
 *
 * Calibration is validated with CRC32.  The QSPI interface uses the
 * STM32H7's QUADSPI peripheral in indirect mode (simplified here).
 */

#include "flash_drv.h"
#include "registers.h"
#include <string.h>

/* QSPI flash layout */
#define FLASH_ADDR_CAL       0x000000
#define FLASH_ADDR_MODEL     0x001000
#define FLASH_ADDR_NAMES     0x040000
#define FLASH_SECTOR_SIZE    4096

/* W25Q128 commands */
#define W25Q_CMD_READ        0x03
#define W25Q_CMD_WRITE_EN    0x06
#define W25Q_CMD_PAGE_PROG   0x02
#define W25Q_CMD_SECTOR_ER   0x20
#define W25Q_CMD_RD_STATUS   0x05

static bool flash_ready = false;

/* QSPI register accessors (simplified) */
#define QSPI_BASE_ADDR       0x48003000UL
#define QSPI_CR              (*(volatile uint32_t *)(QSPI_BASE_ADDR + 0x00))
#define QSPI_DCR             (*(volatile uint32_t *)(QSPI_BASE_ADDR + 0x04))
#define QSPI_SR              (*(volatile uint32_t *)(QSPI_BASE_ADDR + 0x08))
#define QSPI_DLR             (*(volatile uint32_t *)(QSPI_BASE_ADDR + 0x10))
#define QSPI_CCR             (*(volatile uint32_t *)(QSPI_BASE_ADDR + 0x14))
#define QSPI_AR              (*(volatile uint32_t *)(QSPI_BASE_ADDR + 0x18))
#define QSPI_DR              (*(volatile uint32_t *)(QSPI_BASE_ADDR + 0x20))

#define QSPI_CS_HIGH()       (QSPI_CR |= (1 << 1))
#define QSPI_CS_LOW()        (QSPI_CR &= ~(1 << 1))

/* ========================================================================
 * CRC32 (IEEE 802.3 polynomial)
 * ======================================================================== */
uint32_t flash_crc32(const void *data, int len) {
    const uint8_t *p = (const uint8_t *)data;
    uint32_t crc = 0xFFFFFFFF;
    for (int i = 0; i < len; i++) {
        crc ^= p[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 1) crc = (crc >> 1) ^ 0xEDB88320;
            else crc >>= 1;
        }
    }
    return crc ^ 0xFFFFFFFF;
}

/* ========================================================================
 * Initialize QSPI flash
 * ======================================================================== */
void flash_drv_init(void) {
    /* Enable QSPI clock (would be in RCC AHB4) */
    /* Configure QSPI in indirect mode */

    /* Wait for flash to be ready */
    for (volatile int i = 0; i < 100000; i++) { }

    flash_ready = true;
}

/* ========================================================================
 * Read from QSPI flash (indirect mode)
 * ======================================================================== */
static void flash_read(uint32_t addr, uint8_t *buf, int len) {
    /* Configure indirect read */
    QSPI_DLR = len - 1;                /* data length */
    QSPI_CCR = (W25Q_CMD_READ << 24)   /* instruction */
             | (0 << 16)              /* mode: no mode byte */
             | (0 << 12)              /* dummy cycles */
             | (3 << 8)               /* address: 3 bytes */
             | (1 << 0);              /* data: enable */
    QSPI_AR = addr;

    /* Read data */
    for (int i = 0; i < len; i++) {
        buf[i] = (uint8_t)QSPI_DR;
    }

    /* Wait for completion */
    while (QSPI_SR & 0x20) { }  /* BUSY */
    QSPI_SR = 0x20;             /* clear busy flag */
}

/* ========================================================================
 * Write to QSPI flash (page program, 256 bytes max per page)
 * ======================================================================== */
static void flash_write_page(uint32_t addr, const uint8_t *buf, int len) {
    if (len > 256) len = 256;

    /* Write enable */
    QSPI_CCR = (W25Q_CMD_WRITE_EN << 24) | (0 << 0);
    while (QSPI_SR & 0x20) { }

    /* Page program */
    QSPI_DLR = len - 1;
    QSPI_CCR = (W25Q_CMD_PAGE_PROG << 24) | (3 << 8) | (1 << 0);
    QSPI_AR = addr;
    for (int i = 0; i < len; i++) {
        QSPI_DR = buf[i];
    }

    while (QSPI_SR & 0x20) { }

    /* Wait for write complete (poll status register) */
    uint8_t status;
    do {
        QSPI_CCR = (W25Q_CMD_RD_STATUS << 24) | (1 << 0);
        status = (uint8_t)QSPI_DR;
    } while (status & 0x01);  /* WIP bit */
}

/* ========================================================================
 * Erase a 4 KB sector
 * ======================================================================== */
static void flash_erase_sector(uint32_t addr) {
    QSPI_CCR = (W25Q_CMD_WRITE_EN << 24);
    while (QSPI_SR & 0x20) { }

    QSPI_CCR = (W25Q_CMD_SECTOR_ER << 24) | (3 << 8);
    QSPI_AR = addr;
    while (QSPI_SR & 0x20) { }

    /* Wait for completion */
    uint8_t status;
    do {
        QSPI_CCR = (W25Q_CMD_RD_STATUS << 24) | (1 << 0);
        status = (uint8_t)QSPI_DR;
    } while (status & 0x01);
}

/* ========================================================================
 * Read calibration data
 * ======================================================================== */
int flash_drv_read_cal(cal_data_t *cal) {
    if (!flash_ready) return -1;
    flash_read(FLASH_ADDR_CAL, (uint8_t *)cal, sizeof(cal_data_t));
    return 0;
}

/* ========================================================================
 * Write calibration data
 * ======================================================================== */
int flash_drv_write_cal(const cal_data_t *cal) {
    if (!flash_ready) return -1;

    /* Compute CRC before writing */
    cal_data_t cal_copy = *cal;
    cal_copy.crc = flash_crc32(cal, offsetof(cal_data_t, crc));

    /* Erase sector, then write */
    flash_erase_sector(FLASH_ADDR_CAL);
    flash_write_page(FLASH_ADDR_CAL, (const uint8_t *)&cal_copy, sizeof(cal_data_t));

    return 0;
}

/* ========================================================================
 * Validate calibration CRC
 * ======================================================================== */
bool flash_drv_validate_cal(const cal_data_t *cal) {
    uint32_t expected = flash_crc32(cal, offsetof(cal_data_t, crc));
    return (cal->crc == expected);
}

/* ========================================================================
 * Read NILM model header
 * ======================================================================== */
int flash_drv_read_model_header(uint8_t *buf, int len) {
    if (!flash_ready) return -1;
    flash_read(FLASH_ADDR_MODEL, buf, len);
    return 0;
}

/* ========================================================================
 * Write NILM model to flash
 * ======================================================================== */
int flash_drv_write_model(const uint8_t *data, int len) {
    if (!flash_ready) return -1;

    /* Erase sectors covering the model (each 4 KB) */
    int sectors = (len + FLASH_SECTOR_SIZE - 1) / FLASH_SECTOR_SIZE;
    for (int s = 0; s < sectors; s++) {
        flash_erase_sector(FLASH_ADDR_MODEL + s * FLASH_SECTOR_SIZE);
    }

    /* Write pages (256 bytes each) */
    int offset = 0;
    while (offset < len) {
        int chunk = len - offset;
        if (chunk > 256) chunk = 256;
        flash_write_page(FLASH_ADDR_MODEL + offset, &data[offset], chunk);
        offset += chunk;
    }

    return 0;
}

/* ========================================================================
 * Read model weight matrices from flash
 * (Simplified — would read each matrix at its offset within the model blob)
 * ======================================================================== */
int flash_drv_read_model_weights(void *w1, void *b1, void *w2, void *b2,
                                   void *w3, void *b3, void *w4, void *b4) {
    if (!flash_ready) return -1;
    /* Read model blob starting at offset 16 (after header) */
    /* In production: read each weight matrix from its known offset */
    (void)w1; (void)b1; (void)w2; (void)b2;
    (void)w3; (void)b3; (void)w4; (void)b4;
    return 0;
}