/*
 * sdlog.c — microSD binary data logger for MycoMesh
 * MycoMesh — Open-Source Fungal Mycelium Electrophysiology & Chemical Sensing Network
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Implements a binary data logger for microSD cards using SPI mode.
 * The SD card is accessed via SPI2 at 12 MHz with a simple block-based
 * write protocol.  No FAT filesystem is implemented in firmware — the
 * card is written with a custom sequential binary format that the
 * companion app's binaryToCsv.js converter can parse.
 *
 * For production deployments, a FAT32 filesystem would be preferable
 * (via FatFS), but the custom format is simpler, faster, and avoids
 * filesystem corruption risk from unexpected power loss in field
 * deployments.
 */

#include "sdlog.h"
#include "registers.h"
#include <string.h>

/* ===================================================================== */
/*  SD card SPI access                                                    */
/* ===================================================================== */

#define SD_CMD0     0       /* GO_IDLE_STATE          */
#define SD_CMD8     8       /* SEND_IF_COND            */
#define SD_CMD9     9       /* SEND_CSD                */
#define SD_CMD10    10      /* SEND_CID                */
#define SD_CMD12    12      /* STOP_TRANSMISSION       */
#define SD_CMD13    13      /* SEND_STATUS             */
#define SD_CMD17    17      /* READ_SINGLE_BLOCK       */
#define SD_CMD24    24      /* WRITE_SINGLE_BLOCK      */
#define SD_CMD55    55      /* APP_CMD                 */
#define SD_ACMD41   0x80|41 /* SEND_OP_COND (app)      */
#define SD_CMD58    58      /* READ_OCR                */

#define SD_R1_IDLE  0x01
#define SD_TOKEN_START_BLOCK  0xFE
#define SD_TOKEN_DATA_ACCEPTED 0x05

static void sd_cs_low(void)  { GPIO_RESET(SD_CS_PORT, SD_CS_PIN); }
static void sd_cs_high(void) { GPIO_SET(SD_CS_PORT, SD_CS_PIN); }

static uint8_t sd_spi_xfer(uint8_t tx)
{
    while (!(SPI2->SR & SPI_SR_TXE));
    *(volatile uint8_t *)&SPI2->DR = tx;
    while (!(SPI2->SR & SPI_SR_RXNE));
    return *(volatile uint8_t *)&SPI2->DR;
}

static void sd_spi_clock_delay(void)
{
    for (volatile int i = 0; i < 100; i++);
}

/* Send a command and return R1 response. */
static uint8_t sd_send_cmd(uint8_t cmd, uint32_t arg)
{
    uint8_t r1;

    /* For ACMD commands, first send CMD55. */
    if (cmd & 0x80) {
        cmd &= 0x7F;
        sd_send_cmd(SD_CMD55, 0);
    }

    sd_spi_xfer(0xFF);  /* dummy byte */

    sd_spi_xfer(0x40 | cmd);
    sd_spi_xfer((arg >> 24) & 0xFF);
    sd_spi_xfer((arg >> 16) & 0xFF);
    sd_spi_xfer((arg >> 8) & 0xFF);
    sd_spi_xfer(arg & 0xFF);

    /* CRC (only valid for CMD0 and CMD8; ignored otherwise). */
    sd_spi_xfer(0x95);

    /* Wait for response (up to 8 retries). */
    for (uint8_t i = 0; i < 8; i++) {
        r1 = sd_spi_xfer(0xFF);
        if (r1 != 0xFF) break;
    }

    return r1;
}

/* ===================================================================== */
/*  SD card initialization (SPI mode)                                     */
/* ===================================================================== */

static int sd_init_card(void)
{
    /* Send 80 clock pulses with CS high to enter SPI mode. */
    sd_cs_high();
    for (int i = 0; i < 10; i++) sd_spi_xfer(0xFF);

    /* CMD0: reset and enter idle state. */
    sd_cs_low();
    uint8_t r1 = sd_send_cmd(SD_CMD0, 0);
    if (r1 != SD_R1_IDLE) {
        sd_cs_high();
        return -1;
    }

    /* CMD8: check voltage range (SDv2 only). */
    r1 = sd_send_cmd(SD_CMD8, 0x000001AA);
    if (r1 & SD_R1_IDLE) {
        /* Read remaining R7 response (4 bytes). */
        for (int i = 0; i < 4; i++) sd_spi_xfer(0xFF);
    }

    /* ACMD41: initialize card (HCS = 1 for SDHC support). */
    uint32_t timeout = 0;
    do {
        r1 = sd_send_cmd(SD_ACMD41, 0x40000000);
        if (++timeout > 100000) {
            sd_cs_high();
            return -1;
        }
    } while (r1 != 0x00);

    /* CMD58: read OCR to confirm SDHC. */
    r1 = sd_send_cmd(SD_CMD58, 0);
    (void)sd_spi_xfer(0xFF);  /* OCR byte 1 (CCS bit in bit 6 of byte 1) */
    for (int i = 0; i < 3; i++) sd_spi_xfer(0xFF);

    sd_cs_high();
    sd_spi_xfer(0xFF);  /* 8 clocks to release bus */
    return 0;
}

/* ===================================================================== */
/*  Block read/write                                                      */
/* ===================================================================== */

static int sd_write_block(uint32_t block_addr, const uint8_t *data)
{
    sd_cs_low();

    uint8_t r1 = sd_send_cmd(SD_CMD24, block_addr);
    if (r1 != 0x00) {
        sd_cs_high();
        return -1;
    }

    /* Send start block token. */
    sd_spi_xfer(SD_TOKEN_START_BLOCK);

    /* Send 512 bytes of data. */
    for (int i = 0; i < 512; i++) {
        sd_spi_xfer(data[i]);
    }

    /* Send dummy CRC (2 bytes). */
    sd_spi_xfer(0xFF);
    sd_spi_xfer(0xFF);

    /* Read data response token. */
    uint8_t resp;
    for (int i = 0; i < 10; i++) {
        resp = sd_spi_xfer(0xFF);
        if ((resp & 0x1F) == SD_TOKEN_DATA_ACCEPTED) break;
    }

    if ((resp & 0x1F) != SD_TOKEN_DATA_ACCEPTED) {
        sd_cs_high();
        return -1;
    }

    /* Wait for card to finish programming (DO held low = busy). */
    uint32_t timeout = 1000000;
    while (sd_spi_xfer(0xFF) == 0x00 && --timeout);

    sd_cs_high();
    sd_spi_xfer(0xFF);  /* release bus */
    return 0;
}

/* ===================================================================== */
/*  Logger state                                                          */
/* ===================================================================== */

static uint32_t g_current_block = 0;    /* next SD block address to write */
static uint8_t  g_block_buffer[512];     /* 512-byte write buffer */
static uint16_t g_buffer_pos = 0;        /* current position in buffer */
static uint32_t g_blocks_written = 0;
static uint32_t g_file_size = 0;
static uint8_t  g_initialized = 0;

/* ===================================================================== */
/*  Initialization                                                        */
/* ===================================================================== */

int sdlog_init(void)
{
    /* Check card presence (active-low detect). */
    if (GPIO_READ(SD_DETECT_PORT, SD_DETECT_PIN)) {
        return -1;  /* no card */
    }

    /* Initialize SD card in SPI mode. */
    if (sd_init_card() != 0) {
        return -1;
    }

    /* Start logging at block 1 (block 0 reserved for header/metadata). */
    g_current_block = 1;
    g_buffer_pos = 0;
    g_blocks_written = 0;
    g_file_size = 0;
    g_initialized = 1;

    /* Write a header block with metadata. */
    memset(g_block_buffer, 0, 512);
    uint32_t magic = SD_LOG_MAGIC;
    memcpy(&g_block_buffer[0], &magic, 4);
    g_block_buffer[4] = SD_LOG_VERSION;
    g_block_buffer[5] = ADS1298_TOTAL_CHANS;
    uint16_t sr = ADS1298_SAMPLE_RATE;
    memcpy(&g_block_buffer[6], &sr, 2);
    /* Author string. */
    const char *author = "jayis1";
    memcpy(&g_block_buffer[8], author, 6);
    /* Device name. */
    const char *name = "MycoMesh";
    memcpy(&g_block_buffer[16], name, 8);

    sd_write_block(0, g_block_buffer);

    return 0;
}

/* ===================================================================== */
/*  Buffer management                                                     */
/* ===================================================================== */

static int sdlog_flush_buffer(void)
{
    if (g_buffer_pos == 0) return 0;

    /* Pad remaining buffer with 0xFF. */
    for (uint16_t i = g_buffer_pos; i < 512; i++) {
        g_block_buffer[i] = 0xFF;
    }

    if (sd_write_block(g_current_block, g_block_buffer) != 0) {
        return -1;
    }

    g_current_block++;
    g_blocks_written++;
    g_file_size += 512;
    g_buffer_pos = 0;

    return 0;
}

static void sdlog_append(const void *data, uint16_t len)
{
    const uint8_t *src = (const uint8_t *)data;
    uint16_t offset = 0;

    while (offset < len) {
        uint16_t space = 512 - g_buffer_pos;
        uint16_t to_copy = (len - offset < space) ? (len - offset) : space;

        memcpy(&g_block_buffer[g_buffer_pos], &src[offset], to_copy);
        g_buffer_pos += to_copy;
        offset += to_copy;

        if (g_buffer_pos >= 512) {
            sdlog_flush_buffer();
        }
    }
}

/* ===================================================================== */
/*  Public API                                                            */
/* ===================================================================== */

int sdlog_write_block(int16_t filtered[ADS1298_TOTAL_CHANS][DSP_BLOCK_SIZE],
                      uint16_t n_samples, uint32_t timestamp_ms)
{
    if (!g_initialized) return -1;

    /* Write block header: magic(4) + version(2) + timestamp(4) +
       n_channels(2) + sample_rate(2) + n_samples(2) = 16 bytes. */
    uint32_t magic = SD_LOG_MAGIC;
    sdlog_append(&magic, 4);
    uint16_t version = SD_LOG_VERSION;
    sdlog_append(&version, 2);
    sdlog_append(&timestamp_ms, 4);
    uint16_t n_ch = ADS1298_TOTAL_CHANS;
    sdlog_append(&n_ch, 2);
    uint16_t sr = ADS1298_SAMPLE_RATE;
    sdlog_append(&sr, 2);
    sdlog_append(&n_samples, 2);

    /* Write sample data: each sample is 3 bytes (24-bit signed).
       We store as 16-bit (2 bytes) for space efficiency. */
    for (uint16_t s = 0; s < n_samples; s++) {
        for (uint8_t ch = 0; ch < ADS1298_TOTAL_CHANS; ch++) {
            int16_t val = filtered[ch][s];
            sdlog_append(&val, 2);
        }
    }

    /* Compute and append CRC16. */
    uint16_t crc = 0;
    /* Simple CRC16-CCITT over the sample data. */
    for (uint16_t s = 0; s < n_samples; s++) {
        for (uint8_t ch = 0; ch < ADS1298_TOTAL_CHANS; ch++) {
            crc ^= (uint16_t)filtered[ch][s];
            for (int b = 0; b < 16; b++) {
                if (crc & 1) crc = (crc >> 1) ^ 0x8408;
                else crc >>= 1;
            }
        }
    }
    sdlog_append(&crc, 2);

    return 0;
}

int sdlog_write_epoch(const epoch_summary_t *epoch)
{
    if (!g_initialized) return -1;

    /* Write epoch record with a marker byte to distinguish from data blocks. */
    uint8_t marker = 0xEE;  /* epoch record marker */
    sdlog_append(&marker, 1);
    sdlog_append(epoch, sizeof(epoch_summary_t));

    return 0;
}

int sdlog_flush(void)
{
    if (!g_initialized) return -1;
    return sdlog_flush_buffer();
}

int sdlog_close(void)
{
    if (!g_initialized) return -1;
    sdlog_flush();
    g_initialized = 0;
    return 0;
}

uint32_t sdlog_file_size(void)
{
    return g_file_size + g_buffer_pos;
}

uint32_t sdlog_block_count(void)
{
    return g_blocks_written;
}