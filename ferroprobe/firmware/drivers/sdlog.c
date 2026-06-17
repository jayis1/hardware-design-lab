/*
 * sdlog.c — FAT32 SD card data logger
 * FerroProbe — Open-Source 3-Axis Fluxgate Magnetometer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Implements a minimal FAT32 write-only logger on a microSD card
 * via SPI1.  Does not use a full filesystem library — instead, it
 * performs raw sector writes to pre-allocated files for maximum
 * reliability and minimum flash usage.
 *
 * The log format is a sequence of 32-byte binary records (see board.h
 * for the record structure).  Each record contains a timestamped,
 * GPS-tagged magnetic field measurement with orientation data.
 */

#include "sdlog.h"
#include "../board.h"
#include "../registers.h"
#include <string.h>

/* ===================================================================== */
/*  SPI1 SD card low-level access (shares SPI1 with ADS1256)             */
/* ===================================================================== */

/* SD card SPI commands (via SPI1, CS = PD1) */
#define SD_CMD_GO_IDLE         0x40U   /* CMD0 */
#define SD_CMD_SEND_IF_COND    0x48U   /* CMD8 */
#define SD_CMD_SEND_CSD        0x09U   /* CMD9 */
#define SD_CMD_SEND_CID        0x0AU   /* CMD10 */
#define SD_CMD_STOP_TRAN       0x0CU   /* CMD12 */
#define SD_CMD_SET_BLOCKLEN    0x10U   /* CMD16 */
#define SD_CMD_READ_SINGLE     0x11U   /* CMD17 */
#define SD_CMD_WRITE_SINGLE    0x18U   /* CMD24 */
#define SD_CMD_APP_CMD         0x25U   /* CMD55 */
#define SD_CMD_ACMD_SD_INIT    0x29U   /* ACMD41 */
#define SD_CMD_READ_OCR        0x3AU   /* CMD58 */

#define SD_BLOCK_SIZE  512U

static uint8_t sd_spi_transfer(uint8_t tx)
{
    /* Select SD CS */
    gpio_reset(SD_CS_PORT, SD_CS_PIN);
    /* Use the same SPI1 transfer function as the ADC driver */
    /* Since SPI1 is shared, we implement a local transfer here.
     * The SPI1 is already initialized by adc_init().
     */
    while (!(SPI_REG(SPI1_BASE, SPI_SR) & SPI_SR_TXP))
        ;
    SPI_REG(SPI1_BASE, SPI_TXDR) = tx;
    while (!(SPI_REG(SPI1_BASE, SPI_SR) & SPI_SR_RXP))
        ;
    uint8_t rx = (uint8_t)SPI_REG(SPI1_BASE, SPI_RXDR);
    gpio_set(SD_CS_PORT, SD_CS_PIN);
    return rx;
}

/* Send a command with 4-byte argument and read R1 response */
static uint8_t sd_send_cmd(uint8_t cmd, uint32_t arg)
{
    gpio_reset(SD_CS_PORT, SD_CS_PIN);
    delay_us(10);

    /* Send command byte */
    sd_spi_transfer(cmd | 0x40U);
    /* Send argument (big-endian) */
    sd_spi_transfer((arg >> 24) & 0xFF);
    sd_spi_transfer((arg >> 16) & 0xFF);
    sd_spi_transfer((arg >> 8) & 0xFF);
    sd_spi_transfer(arg & 0xFF);
    /* CRC (only valid for CMD0 and CMD8, dummy otherwise) */
    sd_spi_transfer(0x95U);

    /* Read R1 response (up to 8 attempts) */
    uint8_t response;
    uint8_t retries = 8;
    do {
        response = sd_spi_transfer(0xFF);
    } while ((response & 0x80) && --retries);

    gpio_set(SD_CS_PORT, SD_CS_PIN);
    return response;
}

/* Wait for SD card to be ready (0xFF on data line) */
static uint8_t sd_wait_ready(uint32_t timeout)
{
    uint8_t status;
    do {
        status = sd_spi_transfer(0xFF);
        if (timeout-- == 0)
            return 0xFF;
    } while (status != 0xFF);
    return status;
}

/* Initialize SD card in SPI mode */
static uint8_t sd_init_card(void)
{
    uint8_t response;

    /* Send 80 clock pulses with CS high to enter SPI mode */
    gpio_set(SD_CS_PORT, SD_CS_PIN);
    for (int i = 0; i < 10; i++)
        sd_spi_transfer(0xFF);

    /* CMD0: GO_IDLE_STATE → should return 0x01 (idle) */
    response = sd_send_cmd(SD_CMD_GO_IDLE, 0);
    if (response != 0x01)
        return 0;  /* No card or not in idle */

    /* CMD8: SEND_IF_COND (check for SD v2) */
    response = sd_send_cmd(SD_CMD_SEND_IF_COND, 0x000001AA);
    /* Read 4-byte OCR response (we don't use it here) */
    for (int i = 0; i < 4; i++)
        sd_spi_transfer(0xFF);

    /* ACMD41: SD_SEND_OP_COND (initialize, repeat until not idle) */
    uint32_t timeout = 1000;
    do {
        sd_send_cmd(SD_CMD_APP_CMD, 0);
        response = sd_send_cmd(SD_CMD_ACMD_SD_INIT, 0x40000000);
        if (timeout-- == 0)
            return 0;
    } while (response != 0x00);

    return 1;  /* Card initialized */
}

/* Write a single 512-byte block */
static uint8_t sd_write_block(uint32_t block_addr, const uint8_t *data)
{
    /* CMD24: WRITE_SINGLE_BLOCK */
    uint8_t response = sd_send_cmd(SD_CMD_WRITE_SINGLE, block_addr);
    if (response != 0x00)
        return 0;

    gpio_reset(SD_CS_PORT, SD_CS_PIN);
    /* Send data start token */
    sd_spi_transfer(0xFEU);
    /* Send 512 bytes of data */
    for (uint16_t i = 0; i < SD_BLOCK_SIZE; i++)
        sd_spi_transfer(data[i]);
    /* Send dummy CRC */
    sd_spi_transfer(0xFF);
    sd_spi_transfer(0xFF);

    /* Read data response token (xxx0DDD1, D=data accepted) */
    uint8_t token = sd_spi_transfer(0xFF);
    if ((token & 0x1FU) != 0x05U) {
        gpio_set(SD_CS_PORT, SD_CS_PIN);
        return 0;  /* Write failed */
    }

    /* Wait for write to complete */
    sd_wait_ready(50000);
    gpio_set(SD_CS_PORT, SD_CS_PIN);
    return 1;
}

/* Read a single 512-byte block */
static uint8_t sd_read_block(uint32_t block_addr, uint8_t *data)
{
    uint8_t response = sd_send_cmd(SD_CMD_READ_SINGLE, block_addr);
    if (response != 0x00)
        return 0;

    gpio_reset(SD_CS_PORT, SD_CS_PIN);
    /* Wait for data start token (0xFE) */
    uint32_t timeout = 50000;
    uint8_t token;
    do {
        token = sd_spi_transfer(0xFF);
        if (timeout-- == 0) {
            gpio_set(SD_CS_PORT, SD_CS_PIN);
            return 0;
        }
    } while (token != 0xFEU);

    /* Read 512 bytes */
    for (uint16_t i = 0; i < SD_BLOCK_SIZE; i++)
        data[i] = sd_spi_transfer(0xFF);
    /* Read and discard CRC */
    sd_spi_transfer(0xFF);
    sd_spi_transfer(0xFF);
    gpio_set(SD_CS_PORT, SD_CS_PIN);
    return 1;
}

/* ===================================================================== */
/*  Log file management                                                    */
/* ===================================================================== */

/* We use a simple flat-file approach: the SD card is pre-formatted as
 * FAT32 on a host computer.  We find the first free sector area by
 * reading the FAT32 boot sector to determine the data start region.
 *
 * For simplicity in this open-source design, we write log data starting
 * at a fixed sector offset (sector 8192 = 4 MB into the card, past the
 * FAT tables).  Each survey session creates a new "file" by writing
 * sequential blocks from the start offset.
 *
 * A manifest sector at sector 8191 records the session index and
 * starting sector of each survey.
 */

#define SD_MANIFEST_SECTOR  8191U
#define SD_DATA_START_SECTOR 8192U
#define SD_MAX_SESSIONS     100U
#define SD_MAX_BLOCKS       100000U  /* ~50 MB max per card */

static char current_filename[16];
static uint32_t current_block = 0;
static uint32_t record_count = 0;
static uint32_t session_start_block = 0;
static uint8_t block_buffer[SD_BLOCK_SIZE];
static uint16_t block_offset = 0;
static uint8_t sd_initialized = 0;
static uint8_t logging_active = 0;

/* Manifest record (per session) */
typedef struct {
    char filename[16];
    uint32_t start_block;
    uint32_t block_count;
    uint32_t record_count;
} session_manifest_t;

static session_manifest_t manifest_entries[SD_MAX_SESSIONS];
static uint8_t manifest_block_buffer[SD_BLOCK_SIZE];

uint8_t sdlog_init(void)
{
    /* SD card uses SPI1 (already initialized by adc_init).
     * Just need to ensure SD CS is configured.
     */
    sd_initialized = sd_init_card();
    if (!sd_initialized)
        return 0;

    /* Load manifest from sector 8191 */
    if (!sd_read_block(SD_MANIFEST_SECTOR, manifest_block_buffer)) {
        /* First time: zero the manifest */
        memset(manifest_block_buffer, 0, SD_BLOCK_SIZE);
        memset(manifest_entries, 0, sizeof(manifest_entries));
    } else {
        memcpy(manifest_entries, manifest_block_buffer, sizeof(manifest_entries));
    }

    /* Find the next free data block */
    uint32_t max_block = SD_DATA_START_SECTOR;
    for (int i = 0; i < SD_MAX_SESSIONS; i++) {
        if (manifest_entries[i].filename[0] != '\0') {
            uint32_t end = manifest_entries[i].start_block + manifest_entries[i].block_count;
            if (end > max_block)
                max_block = end;
        }
    }
    session_start_block = max_block;
    current_block = max_block;

    return 1;
}

uint8_t sdlog_start(const char *filename)
{
    if (!sd_initialized)
        return 0;

    strncpy(current_filename, filename, sizeof(current_filename) - 1);
    current_filename[sizeof(current_filename) - 1] = '\0';

    /* Find a free manifest slot */
    int slot = -1;
    for (int i = 0; i < SD_MAX_SESSIONS; i++) {
        if (manifest_entries[i].filename[0] == '\0') {
            slot = i;
            break;
        }
    }
    if (slot < 0)
        return 0;  /* No free slots */

    /* Record the session in the manifest */
    strncpy(manifest_entries[slot].filename, current_filename, 16);
    manifest_entries[slot].start_block = session_start_block;
    manifest_entries[slot].block_count = 0;
    manifest_entries[slot].record_count = 0;

    /* Write manifest */
    memcpy(manifest_block_buffer, manifest_entries, sizeof(manifest_entries));
    sd_write_block(SD_MANIFEST_SECTOR, manifest_block_buffer);

    current_block = session_start_block;
    block_offset = 0;
    record_count = 0;
    logging_active = 1;

    /* Zero the block buffer */
    memset(block_buffer, 0, SD_BLOCK_SIZE);

    return 1;
}

uint8_t sdlog_write(const field_measurement_t *field,
                     const gps_data_t *gps,
                     const imu_data_t *imu,
                     uint8_t flags)
{
    if (!logging_active || !sd_initialized)
        return 0;

    /* Pack a 32-byte binary record (see board.h for format) */
    uint8_t rec[LOG_RECORD_SIZE];
    memset(rec, 0, sizeof(rec));

    /* Timestamp (ms since boot — would use a global tick counter) */
    uint32_t ts = field->timestamp_ms;
    rec[0] = ts & 0xFF; rec[1] = (ts >> 8) & 0xFF;
    rec[2] = (ts >> 16) & 0xFF; rec[3] = (ts >> 24) & 0xFF;

    /* GPS position */
    int32_t lat = gps->latitude_e7;
    int32_t lon = gps->longitude_e7;
    int32_t alt = gps->altitude_mm;
    memcpy(&rec[4], &lat, 4);
    memcpy(&rec[8], &lon, 4);
    memcpy(&rec[12], &alt, 4);

    /* Magnetic field (×10 = 0.1 nT resolution) */
    int32_t bx = (int32_t)(field->bx_nt * 10.0f);
    int32_t by = (int32_t)(field->by_nt * 10.0f);
    int32_t bz = (int32_t)(field->bz_nt * 10.0f);
    memcpy(&rec[16], &bx, 4);
    memcpy(&rec[20], &by, 4);
    memcpy(&rec[24], &bz, 4);

    /* Orientation */
    int8_t roll = (int8_t)imu->roll_deg;
    int8_t pitch = (int8_t)imu->pitch_deg;
    uint8_t heading = (uint8_t)(imu->heading_deg / 2.0f);  /* 0-180 → 0-360 at 2° res */
    rec[28] = (uint8_t)roll;
    rec[29] = (uint8_t)pitch;
    rec[30] = heading;
    rec[31] = flags;

    /* Copy record into block buffer */
    if (block_offset + LOG_RECORD_SIZE > SD_BLOCK_SIZE) {
        /* Flush current block */
        if (!sd_write_block(current_block, block_buffer))
            return 0;
        current_block++;
        block_offset = 0;
        memset(block_buffer, 0, SD_BLOCK_SIZE);
    }

    memcpy(&block_buffer[block_offset], rec, LOG_RECORD_SIZE);
    block_offset += LOG_RECORD_SIZE;
    record_count++;

    return 1;
}

void sdlog_stop(void)
{
    if (!logging_active)
        return;

    /* Flush remaining data in buffer */
    if (block_offset > 0) {
        sd_write_block(current_block, block_buffer);
        current_block++;
    }

    /* Update manifest with final block count */
    for (int i = 0; i < SD_MAX_SESSIONS; i++) {
        if (strcmp(manifest_entries[i].filename, current_filename) == 0) {
            manifest_entries[i].block_count = current_block - session_start_block;
            manifest_entries[i].record_count = record_count;
            break;
        }
    }

    /* Write manifest */
    memcpy(manifest_block_buffer, manifest_entries, sizeof(manifest_entries));
    sd_write_block(SD_MANIFEST_SECTOR, manifest_block_buffer);

    logging_active = 0;
}

uint32_t sdlog_record_count(void)
{
    return record_count;
}

uint8_t sdlog_read_record(uint32_t index, uint8_t *buf, uint8_t len)
{
    if (!sd_initialized || len < LOG_RECORD_SIZE)
        return 0;

    /* Calculate which block and offset the record is in */
    uint32_t records_per_block = SD_BLOCK_SIZE / LOG_RECORD_SIZE;  /* 16 */
    uint32_t block_index = index / records_per_block;
    uint32_t offset = (index % records_per_block) * LOG_RECORD_SIZE;

    uint32_t sector = session_start_block + block_index;
    static uint8_t read_buf[SD_BLOCK_SIZE];

    if (!sd_read_block(sector, read_buf))
        return 0;

    memcpy(buf, &read_buf[offset], LOG_RECORD_SIZE);
    return 1;
}

const char *sdlog_get_filename(void)
{
    return current_filename;
}

uint8_t sdlog_erase_all(void)
{
    /* Zero the manifest and reset */
    memset(manifest_entries, 0, sizeof(manifest_entries));
    memset(manifest_block_buffer, 0, SD_BLOCK_SIZE);
    sd_write_block(SD_MANIFEST_SECTOR, manifest_block_buffer);
    session_start_block = SD_DATA_START_SECTOR;
    current_block = session_start_block;
    return 1;
}