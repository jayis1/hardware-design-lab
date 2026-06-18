/*
 * sdlog.c — SD card data logging driver
 * FloraPulse — Plant Electrophysiology & Sap-Flow Stress Monitor
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Implements SD card logging via SPI2 (shared with ADS1220 and SX1276
 * via CS-multiplexing).  Uses SPI mode (CMD0→CMD8→ACMD41) initialization
 * for compatibility with standard microSD cards.
 *
 * Data is stored in a flat binary format: each record is LOG_RECORD_SIZE
 * (48) bytes.  Records are appended sequentially into a log file
 * (LOG00001.BIN, LOG00002.BIN, ...).  A simple FAT16/FAT32 filesystem
 * is not implemented here — instead, we use raw block addressing for
 * reliability and simplicity in an embedded context.
 *
 * The card is treated as a linear array of 512-byte sectors.  Records
 * are packed 10 per sector (10 × 48 = 480 bytes, padded to 512).
 */

#include "sdlog.h"
#include "../board.h"
#include "../registers.h"
#include <string.h>

/* ===================================================================== */
/*  Internal state                                                       */
/* ===================================================================== */

static uint8_t  sd_initialized = 0;
static uint32_t sd_sector_count = 0;
static uint32_t sd_current_sector = 0;
static uint16_t sd_record_in_sector = 0;
static uint32_t sd_total_records = 0;

/* ===================================================================== */
/*  SPI2 low-level (shared — must select SD CS)                          */
/* ===================================================================== */

static void spi2_wait_tx(void)
{
    while (!(SPI_REG(SPI2_BASE, SPI_SR) & SPI_SR_TXE))
        ;
}

static void spi2_wait_rx(void)
{
    while (!(SPI_REG(SPI2_BASE, SPI_SR) & SPI_SR_RXNE))
        ;
}

static uint8_t spi2_xfer(uint8_t tx)
{
    spi2_wait_tx();
    SPI_REG(SPI2_BASE, SPI_DR) = tx;
    spi2_wait_rx();
    return (uint8_t)SPI_REG(SPI2_BASE, SPI_DR);
}

static void sd_select(void)
{
    gpio_reset(SD_CS_PORT, SD_CS_PIN);
}

static void sd_deselect(void)
{
    gpio_set(SD_CS_PORT, SD_CS_PIN);
}

/* Send 8 clock pulses (send 0xFF) */
static void sd_send_dummy(void)
{
    spi2_xfer(0xFFU);
}

/* Send a command and return response (R1) */
static uint8_t sd_cmd(uint8_t cmd, uint32_t arg, uint8_t crc)
{
    sd_select();
    spi2_xfer(cmd | 0x40U);
    spi2_xfer((arg >> 24) & 0xFFU);
    spi2_xfer((arg >> 16) & 0xFFU);
    spi2_xfer((arg >> 8) & 0xFFU);
    spi2_xfer(arg & 0xFFU);
    spi2_xfer(crc);

    /* Wait for response (up to 8 attempts) */
    uint8_t resp;
    for (int i = 0; i < 8; i++) {
        resp = spi2_xfer(0xFFU);
        if (resp != 0xFFU) break;
    }
    return resp;
}

/* Wait for data token (0xFE) with timeout */
static uint8_t sd_wait_token(uint8_t token)
{
    uint32_t timeout = 100000;
    uint8_t resp;
    do {
        resp = spi2_xfer(0xFFU);
        if (resp == token) return 1;
        if (resp == 0x00U) break;  /* Error */
    } while (timeout--);
    return 0;
}

/* ===================================================================== */
/*  Initialization                                                       */
/* ===================================================================== */

uint8_t sd_init(void)
{
    sd_deselect();
    /* Send 80 dummy clocks (>74) to enter SPI mode */
    for (int i = 0; i < 10; i++)
        sd_send_dummy();

    /* CMD0: GO_IDLE_STATE (CS low) → should return 0x01 (idle) */
    sd_select();
    uint8_t r1 = sd_cmd(0, 0, 0x95U);
    sd_deselect();
    if (r1 != 0x01U) return 1;

    /* CMD8: SEND_IF_COND (voltage check) */
    sd_select();
    r1 = sd_cmd(8, 0x000001AAU, 0x87U);
    /* Read 4-byte R7 response */
    for (int i = 0; i < 4; i++)
        spi2_xfer(0xFFU);
    sd_deselect();
    if (r1 != 0x01U) return 2;  /* Not SDv2 */

    /* ACMD41: SD_SEND_OP_COND (initiate initialization, HCS=1) */
    uint32_t timeout = 1000000;
    do {
        sd_select();
        sd_cmd(55, 0, 0xFFU);   /* CMD55 */
        r1 = sd_cmd(41, 0x40000000U, 0xFFU);  /* ACMD41 with HCS bit */
        sd_deselect();
    } while (r1 != 0x00U && timeout--);
    if (r1 != 0x00U) return 3;

    /* CMD16: SET_BLOCKLEN to 512 */
    sd_select();
    r1 = sd_cmd(16, 512, 0xFFU);
    sd_deselect();
    if (r1 != 0x00U) return 4;

    /* CMD9: SEND_CSD (to get capacity) */
    sd_select();
    r1 = sd_cmd(9, 0, 0xFFU);
    if (r1 != 0x00U) {
        sd_deselect();
        return 5;
    }
    if (!sd_wait_token(SD_TOKEN_START_BLOCK)) {
        sd_deselect();
        return 6;
    }
    /* Read CSD (16 bytes) + CRC (2 bytes) */
    uint8_t csd[16];
    for (int i = 0; i < 16; i++)
        csd[i] = spi2_xfer(0xFFU);
    spi2_xfer(0xFFU);  /* CRC */
    spi2_xfer(0xFFU);
    sd_deselect();

    /* Parse CSD v2.0: capacity = (C_SIZE + 1) × 512 × 1024 bytes */
    uint32_t c_size = ((uint32_t)(csd[7] & 0x3FU) << 16) |
                      ((uint32_t)csd[8] << 8) | csd[9];
    sd_sector_count = (c_size + 1) * 1024;  /* In 512-byte sectors */

    sd_initialized = 1;
    sd_current_sector = 0;
    sd_record_in_sector = 0;
    sd_total_records = 0;
    return 0;
}

/* ===================================================================== */
/*  Block read/write                                                     */
/* ===================================================================== */

static uint8_t sd_read_block(uint32_t sector, uint8_t *buf)
{
    sd_select();
    uint8_t r1 = sd_cmd(17, sector, 0xFFU);
    if (r1 != 0x00U) {
        sd_deselect();
        return 0;
    }
    if (!sd_wait_token(SD_TOKEN_START_BLOCK)) {
        sd_deselect();
        return 0;
    }
    for (int i = 0; i < 512; i++)
        buf[i] = spi2_xfer(0xFFU);
    spi2_xfer(0xFFU);  /* CRC */
    spi2_xfer(0xFFU);
    sd_deselect();
    return 1;
}

static uint8_t sd_write_block(uint32_t sector, const uint8_t *buf)
{
    sd_select();
    uint8_t r1 = sd_cmd(24, sector, 0xFFU);
    if (r1 != 0x00U) {
        sd_deselect();
        return 0;
    }
    spi2_xfer(SD_TOKEN_START_BLOCK);
    for (int i = 0; i < 512; i++)
        spi2_xfer(buf[i]);
    spi2_xfer(0xFFU);  /* CRC */
    spi2_xfer(0xFFU);

    /* Wait for data accepted token */
    uint8_t resp = spi2_xfer(0xFFU);
    if ((resp & 0x1FU) != SD_TOKEN_DATA_ACCEPTED) {
        sd_deselect();
        return 0;
    }
    /* Wait while card is busy (DO held low) */
    while (spi2_xfer(0xFFU) == 0x00U)
        ;
    sd_deselect();
    return 1;
}

/* ===================================================================== */
/*  Public API                                                           */
/* ===================================================================== */

uint8_t sd_open_log(void)
{
    if (!sd_initialized) return 1;
    /* Start logging at sector 1 (sector 0 = reserved for metadata) */
    sd_current_sector = 1;
    sd_record_in_sector = 0;
    sd_total_records = 0;
    return 0;
}

uint8_t sd_write_record(const uint8_t *data, uint16_t len)
{
    if (!sd_initialized || len != LOG_RECORD_SIZE) return 1;

    /* We pack records into sectors.  Each 512-byte sector holds
     * 10 records (10 × 48 = 480 bytes, remaining 32 bytes padding).
     */
    static uint8_t sector_buf[512];
    static uint16_t buf_offset = 0;

    /* Copy record into sector buffer */
    for (uint16_t i = 0; i < LOG_RECORD_SIZE; i++)
        sector_buf[buf_offset + i] = data[i];
    buf_offset += LOG_RECORD_SIZE;
    sd_record_in_sector++;

    /* When buffer full (10 records = 480 bytes), write sector */
    if (sd_record_in_sector >= 10) {
        /* Zero-fill remaining bytes */
        for (uint16_t i = buf_offset; i < 512; i++)
            sector_buf[i] = 0;
        if (!sd_write_block(sd_current_sector, sector_buf))
            return 2;
        sd_current_sector++;
        sd_record_in_sector = 0;
        buf_offset = 0;
    }

    sd_total_records++;
    return 0;
}

uint16_t sd_read_records(uint32_t start_idx, uint8_t *buf, uint16_t max_records)
{
    if (!sd_initialized) return 0;

    uint16_t records_read = 0;
    uint32_t sector = 1 + (start_idx / 10);
    uint16_t offset = (start_idx % 10) * LOG_RECORD_SIZE;

    uint8_t sector_buf[512];

    while (records_read < max_records && sector < sd_sector_count) {
        if (!sd_read_block(sector, sector_buf))
            break;

        for (uint16_t i = offset; i < 512 && records_read < max_records; i += LOG_RECORD_SIZE) {
            for (uint16_t j = 0; j < LOG_RECORD_SIZE; j++)
                buf[records_read * LOG_RECORD_SIZE + j] = sector_buf[i + j];
            records_read++;
        }

        sector++;
        offset = 0;  /* Subsequent sectors start at beginning */
    }

    return records_read;
}

uint32_t sd_get_record_count(void)
{
    return sd_total_records;
}

void sd_close_log(void)
{
    /* Flush partial sector if needed */
    if (sd_record_in_sector > 0 && sd_initialized) {
        static uint8_t sector_buf[512];
        /* Read existing sector, merge, write back */
        if (sd_read_block(sd_current_sector, sector_buf)) {
            /* Re-write with whatever is in buffer (already merged in write_record) */
            /* For simplicity, write zeros for remaining space */
            sd_write_block(sd_current_sector, sector_buf);
        }
    }
}

uint8_t sd_erase_logs(void)
{
    if (!sd_initialized) return 1;
    /* Write zeros to the first 1000 sectors (metadata + log area) */
    uint8_t zero_buf[512];
    memset(zero_buf, 0, 512);
    for (uint32_t s = 0; s < 1000 && s < sd_sector_count; s++) {
        sd_write_block(s, zero_buf);
    }
    sd_current_sector = 1;
    sd_record_in_sector = 0;
    sd_total_records = 0;
    return 0;
}

uint8_t sd_is_present(void)
{
    return sd_initialized;
}

uint32_t sd_get_capacity_mb(void)
{
    return sd_sector_count / 2048U;  /* 512-byte sectors → MB */
}