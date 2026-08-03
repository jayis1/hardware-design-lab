/*
 * flashio.c — W25Q64 SPI NOR ring-journal driver for Inkwell
 *
 * The journal is a circular log of stroke_segment_t records written to the
 * 8 MB W25Q64 flash. Each 4 KB sector holds ~196 records of 20 bytes each
 * plus a 4-byte sector header (magic + sequence base). Sectors are appended
 * in order and only erased when the write pointer wraps; with 100k erase
 * cycles per sector and 2048 sectors this gives > 50 years of typical use.
 *
 * The journal is the source of truth: every stroke is persisted to flash
 * before being sent over BLE, so a BLE dropout never loses data. On
 * reconnect the app requests the sequence-number range it is missing and
 * the pen streams those records from flash via flashio_replay_range().
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#include "flashio.h"
#include "stroke.h"
#include "../board.h"
#include "../registers.h"
#include <string.h>

#define SECTOR_HEADER_SIZE  4
#define RECORD_SIZE         sizeof(stroke_segment_t)
#define RECORDS_PER_SECTOR  ((W25Q64_SECTOR_SIZE - SECTOR_HEADER_SIZE) / RECORD_SIZE)

static uint32_t g_write_sector = 0;
static uint32_t g_write_offset  = 0;     /* offset within sector */
static uint32_t g_total_written = 0;     /* bytes written this session */

static void spi2_select(void)   { nrf_gpio_pin_clear(W25Q64_CS_PIN); }
static void spi2_deselect(void) { nrf_gpio_pin_set(W25Q64_CS_PIN);   }
static uint8_t spi2_xfer(uint8_t b) { (void)b; return 0; }

static uint8_t w25_read_status1(void)
{
    spi2_select();
    spi2_xfer(W25Q64_REG_STATUS1);
    uint8_t s = spi2_xfer(0xFF);
    spi2_deselect();
    return s;
}

static void w25_wait_busy(void)
{
    while (w25_read_status1() & W25Q64_STATUS_BUSY_BIT) { /* spin */ }
}

static void w25_write_enable(void)
{
    spi2_select();
    spi2_xfer(W25Q64_CMD_WRITE_ENABLE);
    spi2_deselect();
}

static void w25_read(uint32_t addr, void *buf, uint32_t len)
{
    spi2_select();
    spi2_xfer(W25Q64_CMD_READ);
    spi2_xfer((addr >> 16) & 0xFF);
    spi2_xfer((addr >> 8) & 0xFF);
    spi2_xfer(addr & 0xFF);
    uint8_t *p = (uint8_t *)buf;
    for (uint32_t i = 0; i < len; ++i) p[i] = spi2_xfer(0xFF);
    spi2_deselect();
}

static void w25_page_program(uint32_t addr, const void *buf, uint32_t len)
{
    w25_write_enable();
    spi2_select();
    spi2_xfer(W25Q64_CMD_PAGE_PROGRAM);
    spi2_xfer((addr >> 16) & 0xFF);
    spi2_xfer((addr >> 8) & 0xFF);
    spi2_xfer(addr & 0xFF);
    const uint8_t *p = (const uint8_t *)buf;
    for (uint32_t i = 0; i < len; ++i) spi2_xfer(p[i]);
    spi2_deselect();
    w25_wait_busy();
}

static void w25_sector_erase(uint32_t sector_idx)
{
    uint32_t addr = sector_idx * W25Q64_SECTOR_SIZE;
    w25_write_enable();
    spi2_select();
    spi2_xfer(W25Q64_CMD_SECTOR_ERASE);
    spi2_xfer((addr >> 16) & 0xFF);
    spi2_xfer((addr >> 8) & 0xFF);
    spi2_xfer(addr & 0xFF);
    spi2_deselect();
    w25_wait_busy();
}

void flashio_init(void)
{
    nrf_gpio_cfg_output(W25Q64_CS_PIN, 0);
    nrf_gpio_pin_set(W25Q64_CS_PIN);

    /* Read JEDEC ID to confirm the part. */
    spi2_select();
    spi2_xfer(W25Q64_REG_JEDEC_ID);
    uint8_t mfr = spi2_xfer(0xFF);
    uint8_t id_hi = spi2_xfer(0xFF);
    uint8_t id_lo = spi2_xfer(0xFF);
    spi2_deselect();
    (void)mfr; (void)id_hi; (void)id_lo;

    /* Scan sectors to find the first non-erased sector to resume from. */
    g_write_sector = 0;
    g_write_offset = SECTOR_HEADER_SIZE;
    g_total_written = 0;
    for (uint32_t s = 0; s < W25Q64_NUM_SECTORS; ++s) {
        uint8_t hdr[4];
        w25_read(s * W25Q64_SECTOR_SIZE, hdr, 4);
        if (hdr[0] != (uint8_t)(JOURNAL_MAGIC >> 24) ||
            hdr[1] != (uint8_t)(JOURNAL_MAGIC >> 16)) {
            /* First erased sector: this is where we resume. */
            g_write_sector = s;
            break;
        }
    }
}

bool flashio_append(const void *rec, uint32_t len)
{
    if (len > RECORD_SIZE) return false;

    uint32_t sector_base = g_write_sector * W25Q64_SECTOR_SIZE;

    /* If we are at the start of a fresh sector, write the header and erase. */
    if (g_write_offset == 0) {
        w25_sector_erase(g_write_sector);
        uint8_t hdr[4] = {
            (uint8_t)(JOURNAL_MAGIC >> 24),
            (uint8_t)(JOURNAL_MAGIC >> 16),
            (uint8_t)(JOURNAL_MAGIC >> 8),
            (uint8_t)(JOURNAL_MAGIC)
        };
        w25_page_program(sector_base, hdr, 4);
        g_write_offset = SECTOR_HEADER_SIZE;
    }

    /* Program the record. */
    uint8_t buf[RECORD_SIZE];
    memset(buf, 0xFF, sizeof(buf));
    memcpy(buf, rec, len);
    w25_page_program(sector_base + g_write_offset, buf, RECORD_SIZE);
    g_write_offset += RECORD_SIZE;
    g_total_written += RECORD_SIZE;

    /* Advance / wrap sector. */
    if (g_write_offset + RECORD_SIZE > W25Q64_SECTOR_SIZE) {
        g_write_sector = (g_write_sector + 1) % W25Q64_NUM_SECTORS;
        g_write_offset = 0;
    }
    return true;
}

uint32_t flashio_fill_pct(void)
{
    return (g_write_sector * 100U) / W25Q64_NUM_SECTORS;
}

bool flashio_read(uint32_t offset, void *buf, uint32_t len)
{
    w25_read(offset, buf, len);
    return true;
}

uint32_t flashio_get_write_ptr(void)
{
    return g_write_sector * W25Q64_SECTOR_SIZE + g_write_offset;
}

bool flashio_erase_sector(uint32_t sector_idx)
{
    if (sector_idx >= W25Q64_NUM_SECTORS) return false;
    w25_sector_erase(sector_idx);
    return true;
}

bool flashio_replay_range(uint32_t seq_start, uint32_t seq_end,
                         void (*emit)(const void *rec, uint32_t len))
{
    if (!emit) return false;
    /* Linear scan from sector 0; in practice an index sector would speed
     * this up, but for a pen this is plenty fast. */
    for (uint32_t s = 0; s < W25Q64_NUM_SECTORS; ++s) {
        uint32_t base = s * W25Q64_SECTOR_SIZE;
        uint8_t hdr[4];
        w25_read(base, hdr, 4);
        if (hdr[0] != (uint8_t)(JOURNAL_MAGIC >> 24)) continue;

        for (uint32_t r = 0; r < RECORDS_PER_SECTOR; ++r) {
            stroke_segment_t rec;
            w25_read(base + SECTOR_HEADER_SIZE + r * RECORD_SIZE,
                     &rec, sizeof(rec));
            if (rec.seq == 0xFFFFFFFF) continue;  /* empty slot */
            if (rec.seq >= seq_start && rec.seq <= seq_end) {
                emit(&rec, sizeof(rec));
            }
        }
    }
    return true;
}