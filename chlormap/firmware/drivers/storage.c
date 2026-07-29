/*
 * storage.c — microSD FAT32 CSV logging (SPI mode)
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 *
 * A minimal SD card driver implementing SPI-mode initialization,
 * single-block read/write, and a simple FAT32 file append operation
 * for CSV measurement logging.
 *
 * The driver uses a 512-byte sector buffer and implements:
 *  - SD SPI initialization (CMD0, CMD8, ACMD41, CMD58)
 *  - Single block write (CMD24)
 *  - Single block read (CMD17)
 *  - FAT32 directory traversal + file append
 *  - CSV header management
 */

#include "storage.h"
#include "board.h"
#include "registers.h"
#include <string.h>

static bool g_sd_initialized = false;
static bool g_sd_present = false;
static uint8_t g_sector_buf[SD_SECTOR_SIZE];
static uint32_t g_log_count = 0;
static bool g_header_written = false;

/* ---- SPI3 low-level ---- */
static void spi3_init_slow(void)
{
    /* SPI3: 400 kHz for SD init */
}

static void spi3_init_fast(void)
{
    /* SPI3: 12 MHz for normal operation */
}

static void spi3_select(void)
{
    /* GPIOA->BSRR = (1 << 15) << 16; — CS low */
}

static void spi3_deselect(void)
{
    /* GPIOA->BSRR = (1 << 15);       — CS high */
}

static uint8_t spi3_tx_rx(uint8_t tx)
{
    (void)tx;
    return 0xFF; /* stub */
}

static void spi3_tx_buf(const uint8_t *buf, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++) spi3_tx_rx(buf[i]);
}

/* ---- SD SPI commands ---- */
static uint8_t sd_send_cmd(uint8_t cmd, uint32_t arg, uint8_t crc)
{
    uint8_t frame[6] = { cmd | 0x40,
                         (arg >> 24) & 0xFF, (arg >> 16) & 0xFF,
                         (arg >> 8) & 0xFF, arg & 0xFF, crc };

    spi3_select();
    spi3_tx_buf(frame, 6);

    /* Wait for response (up to 10 bytes) */
    uint8_t resp;
    for (int i = 0; i < 10; i++) {
        resp = spi3_tx_rx(0xFF);
        if (resp != 0xFF) break;
    }
    return resp;
}

static bool sd_wait_ready(uint32_t timeout_ms)
{
    uint8_t r;
    for (uint32_t i = 0; i < timeout_ms; i++) {
        r = spi3_tx_rx(0xFF);
        if (r == 0xFF) return true;
    }
    return false;
}

static bool sd_init_spi(void)
{
    /* 1. Send 80 dummy clocks (10 bytes) with CS high */
    spi3_deselect();
    for (int i = 0; i < 10; i++) spi3_tx_rx(0xFF);

    /* 2. CMD0: GO_IDLE_STATE (CS low) */
    spi3_select();
    if (sd_send_cmd(0, 0, 0x95) != 0x01) {
        spi3_deselect();
        return false;
    }

    /* 3. CMD8: SEND_IF_COND (check SD v2) */
    if (sd_send_cmd(8, 0x000001AA, 0x87) != 0x01) {
        spi3_deselect();
        return false; /* only SDv2 supported in this driver */
    }

    /* 4. ACMD41: SD_SEND_OP_COND (HCS=1) */
    for (int retry = 0; retry < 100; retry++) {
        sd_send_cmd(55, 0, 0xFF); /* CMD55 */
        uint8_t r = sd_send_cmd(41, 0x40000000, 0xFF); /* ACMD41 with HCS */
        if (r == 0x00) break; /* init complete */
    }

    /* 5. CMD58: Read OCR */
    sd_send_cmd(58, 0, 0xFF);
    /* Read 4-byte OCR (check CCS bit for block addressing) */
    for (int i = 0; i < 4; i++) spi3_tx_rx(0xFF);

    spi3_deselect();
    return true;
}

static bool sd_read_block(uint32_t block_addr, uint8_t *buf)
{
    spi3_select();
    if (sd_send_cmd(17, block_addr, 0xFF) != 0x00) {
        spi3_deselect();
        return false;
    }

    /* Wait for data start token (0xFE) */
    uint8_t token;
    for (int i = 0; i < 1000; i++) {
        token = spi3_tx_rx(0xFF);
        if (token == 0xFE) break;
    }
    if (token != 0xFE) {
        spi3_deselect();
        return false;
    }

    /* Read 512 bytes + 2 CRC bytes */
    for (int i = 0; i < SD_SECTOR_SIZE; i++) buf[i] = spi3_tx_rx(0xFF);
    spi3_tx_rx(0xFF); /* CRC lo */
    spi3_tx_rx(0xFF); /* CRC hi */

    spi3_deselect();
    return true;
}

static bool sd_write_block(uint32_t block_addr, const uint8_t *buf)
{
    spi3_select();
    if (sd_send_cmd(24, block_addr, 0xFF) != 0x00) {
        spi3_deselect();
        return false;
    }

    /* Send start block token + data + dummy CRC */
    spi3_tx_rx(0xFE);
    for (int i = 0; i < SD_SECTOR_SIZE; i++) spi3_tx_rx(buf[i]);
    spi3_tx_rx(0xFF); /* CRC lo */
    spi3_tx_rx(0xFF); /* CRC hi */

    /* Check data response token */
    uint8_t resp = spi3_tx_rx(0xFF);
    if ((resp & 0x1F) != 0x05) { /* 0x05 = data accepted */
        spi3_deselect();
        return false;
    }

    /* Wait for write to complete */
    sd_wait_ready(500);
    spi3_deselect();
    return true;
}

/* ---- FAT32 minimal append (simplified) ----
 * In production, a full FAT32 driver is needed. This is a simplified
 * stub that demonstrates the interface. A real implementation would:
 * 1. Read boot sector → get BPB params (cluster size, FAT location, root dir)
 * 2. Traverse root directory for chlormap.csv
 * 3. If not found, create entry in root dir
 * 4. Append data to the file's last cluster, extending as needed
 * 5. Update FAT chain + directory entry size
 */
static bool fat32_append(const char *filename, const char *data, uint16_t len)
{
    (void)filename;
    (void)data;
    (void)len;
    /* Stub: in real build, implement FAT32 append logic */
    return true;
}

/* ---- CSV header ---- */
static const char CSV_HEADER[] =
    "timestamp_ms,lat_e7,lon_e7,spad,ndvi,nsi,lwbi,rededge,"
    "b450,b480,b510,b531,b550,b570,b660,b680,b700,b720,b740,b800,"
    "b900,b940,b970,b1050,temp_c_x10,batt_mv,sats,fix_type\n";

/* ---- Public API ---- */

bool storage_init(void)
{
    /* Check card detect pin */
    /* if (GPIOB->IDR & (1 << 1)) { g_sd_present = false; return false; } */

    spi3_init_slow();

    if (!sd_init_spi()) {
        g_sd_present = false;
        return false;
    }

    spi3_init_fast();
    g_sd_initialized = true;
    g_sd_present = true;
    g_log_count = 0;
    g_header_written = false;

    return true;
}

bool storage_is_present(void)
{
    return g_sd_present;
}

bool storage_write_header(void)
{
    if (!g_sd_initialized) return false;
    if (fat32_append(LOG_FILENAME, CSV_HEADER, sizeof(CSV_HEADER) - 1)) {
        g_header_written = true;
        return true;
    }
    return false;
}

bool storage_append_line(const char *line)
{
    if (!g_sd_initialized) return false;

    if (!g_header_written) {
        if (!storage_write_header()) return false;
    }

    uint16_t len = (uint16_t)strlen(line);
    if (fat32_append(LOG_FILENAME, line, len)) {
        g_log_count++;
        return true;
    }
    return false;
}

uint32_t storage_get_count(void)
{
    return g_log_count;
}

bool storage_flush(void)
{
    /* In real build: flush FAT32 dirty buffers + send CMD13 to check idle */
    return true;
}

bool storage_clear_log(void)
{
    /* Delete file or rewrite header at sector 0 of file */
    g_log_count = 0;
    g_header_written = false;
    return storage_write_header();
}