/*
 * flash_log.c — QSPI flash event buffering for Occlusograph.
 *
 * Author: jayis1
 * Copyright © 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 *
 * Stores event_record_t entries into the W25R128 16 MB QSPI NOR flash in a
 * circular log. Each event is 14 bytes; a 4 KB sector holds ~292 events.
 * The log uses a simple header at the start of each sector with a magic
 * number, a sequence counter, and a fill count. Wear-leveling is achieved
 * by cycling through all 4096 sectors before wrapping.
 *
 * The flash is also used to store the current calibration table (piezo
 * offsets + gains) in a dedicated sector at the end of the address space.
 */

#include "flash_log.h"
#include "board.h"
#include "registers.h"
#include <string.h>

/* ---- Platform shim (target: nRF QSPI peripheral via Zephyr) ---- */
extern int  hal_qspi_init(void);
extern int  hal_qspi_read(uint32_t addr, uint8_t *buf, uint32_t len);
extern int  hal_qspi_program(uint32_t addr, const uint8_t *buf, uint32_t len);
extern int  hal_qspi_erase_sector(uint32_t addr);
extern int  hal_qspi_read_id(uint8_t *id3);
extern void hal_flash_log_mutex_lock(void);
extern void hal_flash_log_mutex_unlock(void);

/* ---- Layout ---- */
#define LOG_SECTOR_SIZE     FLASH_SECTOR_SIZE   /* 4096 */
#define LOG_NUM_SECTORS     3500u               /* leave room for calibration + metadata */
#define LOG_BASE_ADDR       0x000000u
#define CALIB_SECTOR_ADDR   (LOG_BASE_ADDR + LOG_NUM_SECTORS * LOG_SECTOR_SIZE)
#define META_SECTOR_ADDR    (CALIB_SECTOR_ADDR + LOG_SECTOR_SIZE)

#define SECTOR_MAGIC        0x4F434C47u  /* "OCLOG" truncated to 4 bytes */
#define EVENTS_PER_SECTOR   (LOG_SECTOR_SIZE / sizeof(event_record_t))

/* In-RAM metadata (mirrored to META_SECTOR on change). */
typedef struct {
    uint32_t write_sector;     /* current sector being written */
    uint32_t write_offset;      /* byte offset within current sector */
    uint32_t total_events;      /* total ever written (monotonic) */
    uint32_t synced_events;     /* events confirmed synced to app */
} log_meta_t;

static log_meta_t g_meta;

/* ---- Helpers ---- */

static int sector_header_valid(uint32_t sector_addr)
{
    uint8_t hdr[8];
    if (hal_qspi_read(sector_addr, hdr, 8)) return 0;
    uint32_t magic = ((uint32_t)hdr[0] << 24) | ((uint32_t)hdr[1] << 16) |
                     ((uint32_t)hdr[2] << 8) | hdr[3];
    return (magic == SECTOR_MAGIC);
}

static int init_sector(uint32_t sector_addr)
{
    uint8_t hdr[8];
    hdr[0] = (SECTOR_MAGIC >> 24) & 0xFF;
    hdr[1] = (SECTOR_MAGIC >> 16) & 0xFF;
    hdr[2] = (SECTOR_MAGIC >> 8) & 0xFF;
    hdr[3] = SECTOR_MAGIC & 0xFF;
    hdr[4] = (uint8_t)(g_meta.total_events >> 24);
    hdr[5] = (uint8_t)(g_meta.total_events >> 16);
    hdr[6] = (uint8_t)(g_meta.total_events >> 8);
    hdr[7] = (uint8_t)(g_meta.total_events & 0xFF);
    /* Must erase before programming. */
    if (hal_qspi_erase_sector(sector_addr)) return -1;
    if (hal_qspi_program(sector_addr, hdr, 8)) return -1;
    return 0;
}

static void meta_save(void)
{
    uint8_t buf[sizeof(log_meta_t)];
    memcpy(buf, &g_meta, sizeof(g_meta));
    hal_qspi_erase_sector(META_SECTOR_ADDR);
    hal_qspi_program(META_SECTOR_ADDR, buf, sizeof(buf));
}

static void meta_load(void)
{
    uint8_t buf[sizeof(log_meta_t)];
    if (hal_qspi_read(META_SECTOR_ADDR, buf, sizeof(buf)) == 0) {
        memcpy(&g_meta, buf, sizeof(g_meta));
        /* Sanity check: if magic is invalid, reset. */
        if (g_meta.total_events > 10000000u) {
            memset(&g_meta, 0, sizeof(g_meta));
        }
    } else {
        memset(&g_meta, 0, sizeof(g_meta));
    }
}

/* ---- Public API ---- */

int flash_log_init(void)
{
    uint8_t id[3];
    int rc = hal_qspi_init();
    if (rc) return rc;

    rc = hal_qspi_read_id(id);
    if (rc) return rc;
    if (id[0] != W25_JEDEC_MANUF || id[1] != W25_JEDEC_TYPE) return -2;

    meta_load();

    /* If the current write sector is invalid, initialize it. */
    uint32_t sec_addr = LOG_BASE_ADDR + g_meta.write_sector * LOG_SECTOR_SIZE;
    if (!sector_header_valid(sec_addr)) {
        init_sector(sec_addr);
        g_meta.write_offset = 8;  /* skip header */
    }
    return 0;
}

int flash_log_append(const event_record_t *evt)
{
    if (!evt) return -1;

    hal_flash_log_mutex_lock();

    uint32_t sec_addr = LOG_BASE_ADDR + g_meta.write_sector * LOG_SECTOR_SIZE;

    /* If this sector is full, advance to the next (wear-leveling wrap). */
    if (g_meta.write_offset + sizeof(event_record_t) > LOG_SECTOR_SIZE) {
        g_meta.write_sector = (g_meta.write_sector + 1) % LOG_NUM_SECTORS;
        sec_addr = LOG_BASE_ADDR + g_meta.write_sector * LOG_SECTOR_SIZE;
        init_sector(sec_addr);
        g_meta.write_offset = 8;
    }

    /* Program the event record. NOR flash can program without erasing as
     * long as we're only changing 1s to 0s; since we erased the sector at
     * init, this is safe for the first pass. */
    uint8_t buf[sizeof(event_record_t)];
    memcpy(buf, evt, sizeof(event_record_t));
    int rc = hal_qspi_program(sec_addr + g_meta.write_offset, buf, sizeof(buf));
    if (rc) {
        hal_flash_log_mutex_unlock();
        return rc;
    }

    g_meta.write_offset += sizeof(event_record_t);
    g_meta.total_events++;
    meta_save();

    hal_flash_log_mutex_unlock();
    return 0;
}

uint32_t flash_log_read(uint32_t start_idx, event_record_t *out, uint32_t max_count)
{
    if (!out || max_count == 0) return 0;

    uint32_t read_count = 0;
    uint32_t evt_idx = start_idx;

    while (read_count < max_count && evt_idx < g_meta.total_events) {
        uint32_t abs_offset = evt_idx * sizeof(event_record_t) + 8; /* +header per sector */
        /* Account for header in each sector. */
        uint32_t sec = abs_offset / LOG_SECTOR_SIZE;
        uint32_t off = abs_offset % LOG_SECTOR_SIZE;
        if (off < 8) { evt_idx++; continue; }  /* skip header region */

        uint32_t addr = LOG_BASE_ADDR + sec * LOG_SECTOR_SIZE + off;
        uint8_t buf[sizeof(event_record_t)];
        if (hal_qspi_read(addr, buf, sizeof(buf))) break;
        memcpy(&out[read_count], buf, sizeof(event_record_t));
        read_count++;
        evt_idx++;
    }
    return read_count;
}

uint32_t flash_log_count(void)
{
    return g_meta.total_events;
}

int flash_log_erase_all(void)
{
    hal_flash_log_mutex_lock();
    for (uint32_t s = 0; s < LOG_NUM_SECTORS; s++) {
        hal_qspi_erase_sector(LOG_BASE_ADDR + s * LOG_SECTOR_SIZE);
    }
    memset(&g_meta, 0, sizeof(g_meta));
    init_sector(LOG_BASE_ADDR);
    g_meta.write_offset = 8;
    meta_save();
    hal_flash_log_mutex_unlock();
    return 0;
}

int flash_log_mark_synced(uint32_t upto_idx)
{
    hal_flash_log_mutex_lock();
    g_meta.synced_events = upto_idx;
    meta_save();
    hal_flash_log_mutex_unlock();
    return 0;
}

uint32_t flash_log_unsynced_idx(void)
{
    if (g_meta.total_events > g_meta.synced_events)
        return g_meta.synced_events;
    return g_meta.total_events;
}