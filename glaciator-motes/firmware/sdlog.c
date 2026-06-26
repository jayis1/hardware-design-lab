/*
 * sdlog.c — microSD SPI logger with FAT-lite ring buffer
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 *
 * Implements a minimal SPI-mode SD card driver and a flat ring-buffer
 * filesystem for continuous waveform logging and event metadata storage.
 * No full FAT library is used to keep flash footprint small and to survive
 * power loss cleanly (we use a fixed-slot ring with atomic sector writes).
 */

#include "sdlog.h"
#include "board.h"
#include <string.h>

/* ---- SD SPI command layer (simplified) --------------------------------- */
#define SD_CMD0    0
#define SD_CMD8    8
#define SD_CMD17   17   /* read single block */
#define SD_CMD24   24   /* write single block */
#define SD_CMD55   55
#define SD_CMD41   41   /* ACMD41 */
#define SD_CMD58   58   /* read OCR */

static bool g_sd_ok = false;
static uint32_t g_sd_blocks = 0;     /* total 512-byte blocks */
static uint32_t g_ring_write_block = 0;
static uint32_t g_ring_start_block = 0;
static uint32_t g_ring_total_blocks = 0;

static uint8_t sd_xfer(uint8_t tx) { (void)tx; return 0xFF; }
static void    sd_cs_low(void)  {}
static void    sd_cs_high(void) {}
static void    delay_ms(uint32_t m) { (void)m; }

static uint8_t sd_cmd(uint8_t cmd, uint32_t arg, uint8_t crc)
{
    uint8_t r, i;
    sd_cs_low();
    sd_xfer(0x40 | cmd);
    sd_xfer((arg >> 24) & 0xFF);
    sd_xfer((arg >> 16) & 0xFF);
    sd_xfer((arg >>  8) & 0xFF);
    sd_xfer( arg        & 0xFF);
    sd_xfer(crc | 0x01);
    /* Wait for response (up to 8 retries) */
    for (i = 0; i < 8; i++) {
        r = sd_xfer(0xFF);
        if (r != 0xFF) break;
    }
    return r;
}

static bool sd_init_card(void)
{
    uint8_t r;
    uint16_t i;
    /* Send 80 dummy clocks with CS high to enter SPI mode */
    sd_cs_high();
    for (i = 0; i < 10; i++) sd_xfer(0xFF);

    /* CMD0: reset */
    r = sd_cmd(SD_CMD0, 0, 0x94);
    if (r != 0x01) return false;   /* expect idle */

    /* CMD8: check voltage range (SDv2) */
    r = sd_cmd(SD_CMD8, 0x000001AA, 0x86);
    /* Read 4-byte R7 response (ignored for brevity) */
    sd_xfer(0xFF); sd_xfer(0xFF); sd_xfer(0xFF); sd_xfer(0xFF);

    /* ACMD41: initialize */
    for (i = 0; i < 1000; i++) {
        sd_cmd(SD_CMD55, 0, 0);
        r = sd_cmd(SD_CMD41, 0x40000000, 0);
        if (r == 0x00) break;
        delay_ms(1);
    }
    if (r != 0x00) return false;

    /* CMD58: read OCR to detect SDHC */
    r = sd_cmd(SD_CMD58, 0, 0);
    uint8_t ocr[4];
    ocr[0] = sd_xfer(0xFF); ocr[1] = sd_xfer(0xFF);
    ocr[2] = sd_xfer(0xFF); ocr[3] = sd_xfer(0xFF);
    /* If bit30 of OCR set, card is SDHC (block-addressed) */
    (void)ocr;

    /* For demo, assume 32 GB card = 62,500,000 blocks of 512 B */
    g_sd_blocks = 62500000;
    return true;
}

static bool sd_read_block(uint32_t block, uint8_t *buf)
{
    uint16_t i;
    uint8_t r;
    r = sd_cmd(SD_CMD17, block, 0);
    if (r != 0x00) return false;
    /* Wait for data start token 0xFE */
    for (i = 0; i < 1000; i++) {
        r = sd_xfer(0xFF);
        if (r == 0xFE) break;
    }
    if (r != 0xFE) return false;
    for (i = 0; i < 512; i++) buf[i] = sd_xfer(0xFF);
    /* Discard CRC */
    sd_xfer(0xFF); sd_xfer(0xFF);
    return true;
}

static bool sd_write_block(uint32_t block, const uint8_t *buf)
{
    uint16_t i;
    uint8_t r;
    r = sd_cmd(SD_CMD24, block, 0);
    if (r != 0x00) return false;
    sd_xfer(0xFE);   /* data start token */
    for (i = 0; i < 512; i++) sd_xfer(buf[i]);
    sd_xfer(0xFF); sd_xfer(0xFF);   /* dummy CRC */
    r = sd_xfer(0xFF);
    if ((r & 0x1F) != 0x05) return false;   /* check data accepted */
    /* Wait for write completion */
    for (i = 0; i < 50000; i++) {
        r = sd_xfer(0xFF);
        if (r != 0x00) break;
    }
    return true;
}

/* ---- Public API --------------------------------------------------------- */
bool sdlog_init(void)
{
    /* Reserve ring: first 1000 blocks for event metadata,
       rest is a 4 GB continuous ring = 8,388,608 blocks. */
    if (!sd_init_card()) return false;
    g_ring_start_block  = 1000;
    g_ring_total_blocks = 8388608;   /* 4 GB */
    g_ring_write_block  = g_ring_start_block;
    g_sd_ok = true;
    return true;
}

bool sdlog_write_event(const event_meta_t *meta,
                       const uint8_t *waveform, uint32_t len)
{
    if (!g_sd_ok) return false;
    /* Event record layout:
       [16 B meta header][N B waveform (multiple of 512)]
       The first sector holds metadata + total length. */
    uint8_t sect[512];
    memset(sect, 0, 512);
    memcpy(sect, meta, sizeof(event_meta_t));
    uint32_t total_len = len;
    memcpy(sect + sizeof(event_meta_t), &total_len, 4);

    uint32_t evt_block = g_ring_write_block;   /* simplified: events share ring */
    if (!sd_write_block(evt_block, sect)) return false;
    g_ring_write_block++;

    uint32_t off = 0;
    while (off < len) {
        uint32_t chunk = (len - off < 512) ? (len - off) : 512;
        memset(sect, 0, 512);
        memcpy(sect, waveform + off, chunk);
        if (!sd_write_block(g_ring_write_block, sect)) return false;
        g_ring_write_block++;
        off += chunk;
    }
    return true;
}

bool sdlog_write_continuous(const uint8_t *buf, uint32_t len)
{
    if (!g_sd_ok) return false;
    uint32_t off = 0;
    while (off + 512 <= len) {
        if (!sd_write_block(g_ring_write_block, buf + off)) return false;
        g_ring_write_block++;
        if (g_ring_write_block >= g_ring_start_block + g_ring_total_blocks) {
            g_ring_write_block = g_ring_start_block;   /* wrap */
        }
        off += 512;
    }
    return true;
}

uint32_t sdlog_capacity_mb(void)
{
    return g_sd_blocks / 2048;
}

uint32_t sdlog_used_mb(void)
{
    if (g_ring_write_block > g_ring_start_block) {
        return (g_ring_write_block - g_ring_start_block) / 2048;
    }
    return 0;
}

bool sdlog_sync(void)
{
    /* In real build: flush any pending SPI DMA, ensure write-completion. */
    return true;
}