/*
 * storage.c — W25R80 SPI flash data logging
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 *
 * Uses the W25R80 8 MB SPI NOR flash as a ring-buffer log.
 * Records are 32 bytes each, fitting ~262,144 records (30 days at
 * 15-min rate = 2,880 records — far within capacity).
 * Writes proceed sequentially; erases happen per-sector when the
 * write pointer wraps.
 */

#include "storage.h"
#include "../board.h"
#include "../registers.h"

/* The log occupies the last 1 MB of the 8 MB flash (offset 0x700000). */
#define LOG_BASE_ADDR    0x700000
#define LOG_REGION_SIZE   0x100000   /* 1 MB */
#define LOG_RECORD_SIZE  32
#define LOG_MAX_RECORDS  (LOG_REGION_SIZE / LOG_RECORD_SIZE)

static uint32_t write_ptr = 0;   /* next record index to write */
static uint32_t record_count = 0;

/* ---- SPI flash low-level ---- */
static void spi_cs_low(void) {
    GPIOA->BSRR = (1 << (PA4__SPI_NCS_FLASH + 16));
}
static void spi_cs_high(void) {
    GPIOA->BSRR = (1 << PA4__SPI_NCS_FLASH);
}

static uint8_t spi_xfer(uint8_t byte) {
    SPI1->CR1 |= (1 << 6);  /* SPE */
    while (!(SPI1->SR & SPI_SR_TXE)) { }
    SPI1->DR = byte;
    while (SPI1->SR & SPI_SR_BSY) { }
    while (!(SPI1->SR & SPI_SR_RXNE)) { }
    return (uint8_t)SPI1->DR;
}

static void flash_write_enable(void) {
    spi_cs_low();
    spi_xfer(W25R80_CMD_WRITE_ENABLE);
    spi_cs_high();
}

static void flash_wait_ready(void) {
    uint8_t status;
    do {
        spi_cs_low();
        spi_xfer(W25R80_CMD_READ_STATUS);
        status = spi_xfer(0x00);
        spi_cs_high();
    } while (status & 0x01);  /* WIP bit */
}

static void flash_read_id(uint8_t id[3]) {
    spi_cs_low();
    spi_xfer(W25R80_CMD_READ_ID);
    id[0] = spi_xfer(0x00);
    id[1] = spi_xfer(0x00);
    id[2] = spi_xfer(0x00);
    spi_cs_high();
}

static void flash_sector_erase(uint32_t addr) {
    flash_write_enable();
    spi_cs_low();
    spi_xfer(W25R80_CMD_SECTOR_ERASE);
    spi_xfer((uint8_t)(addr >> 16));
    spi_xfer((uint8_t)(addr >> 8));
    spi_xfer((uint8_t)(addr));
    spi_cs_high();
    flash_wait_ready();
}

static void flash_page_program(uint32_t addr, const uint8_t *data, uint16_t len) {
    flash_write_enable();
    spi_cs_low();
    spi_xfer(W25R80_CMD_PAGE_PROGRAM);
    spi_xfer((uint8_t)(addr >> 16));
    spi_xfer((uint8_t)(addr >> 8));
    spi_xfer((uint8_t)(addr));
    for (uint16_t i = 0; i < len; i++) {
        spi_xfer(data[i]);
    }
    spi_cs_high();
    flash_wait_ready();
}

static void flash_read(uint32_t addr, uint8_t *buf, uint16_t len) {
    spi_cs_low();
    spi_xfer(W25R80_CMD_READ);
    spi_xfer((uint8_t)(addr >> 16));
    spi_xfer((uint8_t)(addr >> 8));
    spi_xfer((uint8_t)(addr));
    for (uint16_t i = 0; i < len; i++) {
        buf[i] = spi_xfer(0x00);
    }
    spi_cs_high();
}

/* ---- Public API ---- */

int storage_init(void) {
    /* Configure PA4 as output (CS), PA5-7 as SPI AF (done in mesh_init) */
    GPIOA->MODER = (GPIOA->MODER & ~(0x3 << (PA4__SPI_NCS_FLASH * 2)))
                  | (GPIO_MODE_OUTPUT << (PA4__SPI_NCS_FLASH * 2));
    spi_cs_high();

    /* Check JEDEC ID: expect 0xEF, 0x60, 0x14 (Winbond W25R80) */
    uint8_t id[3];
    flash_read_id(id);
    if (id[0] != 0xEF || id[1] != 0x60) {
        /* Not the expected flash; return error but continue */
        return -1;
    }

    /* Determine write pointer: scan for first erased record (0xFF) */
    write_ptr = 0;
    uint8_t buf[32];
    for (uint32_t i = 0; i < LOG_MAX_RECORDS; i++) {
        flash_read(LOG_BASE_ADDR + i * LOG_RECORD_SIZE, buf, 32);
        if (buf[0] == 0xFF && buf[1] == 0xFF) {
            write_ptr = i;
            break;
        }
    }
    if (write_ptr >= LOG_MAX_RECORDS) write_ptr = 0;  /* full -> wrap */

    record_count = (write_ptr < LOG_MAX_RECORDS) ? write_ptr : LOG_MAX_RECORDS;

    return 0;
}

int storage_append(const log_record_t *rec) {
    uint32_t addr = LOG_BASE_ADDR + write_ptr * LOG_RECORD_SIZE;

    /* If we're at a sector boundary, erase the upcoming sector */
    if ((addr % W25R80_SECTOR_SIZE) == 0) {
        flash_sector_erase(addr);
    }

    /* Write 32 bytes (fits in one page since 32 < 256) */
    flash_page_program(addr, (const uint8_t *)rec, LOG_RECORD_SIZE);

    write_ptr = (write_ptr + 1) % LOG_MAX_RECORDS;
    if (record_count < LOG_MAX_RECORDS) record_count++;
    return 0;
}

int storage_read(uint32_t index, log_record_t *out, uint16_t count) {
    if (index >= LOG_MAX_RECORDS) return -1;
    for (uint16_t i = 0; i < count; i++) {
        uint32_t idx = (index + i) % LOG_MAX_RECORDS;
        flash_read(LOG_BASE_ADDR + idx * LOG_RECORD_SIZE,
                   (uint8_t *)&out[i], LOG_RECORD_SIZE);
    }
    return 0;
}

uint32_t storage_get_count(void) {
    return record_count;
}

int storage_erase_all(void) {
    /* Erase the entire 1 MB log region (256 sectors) */
    for (uint32_t s = 0; s < LOG_REGION_SIZE / W25R80_SECTOR_SIZE; s++) {
        flash_sector_erase(LOG_BASE_ADDR + s * W25R80_SECTOR_SIZE);
    }
    write_ptr = 0;
    record_count = 0;
    return 0;
}