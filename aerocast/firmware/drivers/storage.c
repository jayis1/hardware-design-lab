/*
 * storage.c — AeroCast eMMC / microSD event logging & calibration storage
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 *
 * Writes append-only CSV event logs and the calibration overlay file
 * to a FAT32 volume on the SDMMC1 peripheral (eMMC primary, microSD
 * fallback). This is a deliberately simple, blocking, sector-oriented
 * driver: we keep a single open append context and flush every minute.
 *
 * For the review build we model the volume as a flat in-RAM buffer
 * plus a tiny FAT-like directory so the firmware logic is testable
 * without a real block device; the real build links against a TinyUSB
 * + FatFs SDIO backend. The public API is identical either way.
 */
#ifndef AEROCAST_STORAGE_C
#define AEROCAST_STORAGE_C
#endif

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "board.h"
#include "registers.h"
#include "storage.h"
#include "classify.h"   /* for ref_row_t via storage_load_calib signature */

/* In-RAM model of the log volume (16 KB ≈ a few hundred CSV rows). */
#define VOL_BYTES 16384
static uint8_t  g_vol[VOL_BYTES];
static uint32_t g_vol_head = 0;
static uint8_t  g_initialized = 0;

/* ---- Internal: append a string to the volume ---- */
static void vol_append(const char *s)
{
    size_t n = strlen(s);
    if (g_vol_head + n + 1u > VOL_BYTES) {
        /* Wrap: simple ring — overwrite from start */
        g_vol_head = 0;
    }
    memcpy(g_vol + g_vol_head, s, n);
    g_vol_head += n;
    g_vol[g_vol_head++] = '\n';
}

/* ---- Public API ---- */
void storage_init(void)
{
    /* In a real build: sdmmc_init(SDMMC1, 4-bit, 50 MHz); f_mount(&fs,"0:",1);
     * For review: zero the RAM volume and write a header. */
    memset(g_vol, 0, sizeof(g_vol));
    g_vol_head = 0;
    g_initialized = 1;
    vol_append("# AeroCast event log — author jayis1");
}

void storage_append_log(const char *line)
{
    if (!g_initialized) return;
    vol_append(line);
}

void storage_write_bin_csv(const minute_bin_t *b)
{
    if (!g_initialized) return;
    char row[256];
    int n = snprintf(row, sizeof(row),
        "%lu,%.2f,%.1f,%.1f,%.1f,%.0f,%.1f,%.1f,%.1f,%.1f,%.1f",
        (unsigned long)b->epoch_min, b->flow_lpm, b->temperature, b->humidity,
        b->pressure, b->wind_dir, b->wind_speed, b->fsc_p50, b->ssc_p50,
        b->flb_p50, b->fla_p50);
    /* Append per-class counts as CSV tail */
    for (int i = 0; i < N_CLASSES; ++i) {
        n += snprintf(row + n, sizeof(row) - n, ",%lu",
                      (unsigned long)b->counts[i]);
        if (n >= (int)sizeof(row) - 16) break;
    }
    vol_append(row);
}

/* ---- Calibration overlay persistence ---- */
/* calib.bin layout: u16 count, then count × 5 bytes (f[4]+cls). */
int storage_save_calib(const uint8_t *buf, uint16_t len)
{
    if (!g_initialized) return -1;
    /* In real build: f_open("0:/aerocast/calib.bin", FA_WRITE|FA_CREATE_ALWAYS) */
    /* For review: stash at end of volume with a marker. */
    const char marker[] = "###CALIB###";
    vol_append(marker);
    if (len > 1280) len = 1280;   /* 256 rows × 5 */
    if (g_vol_head + len + 2u <= VOL_BYTES) {
        g_vol[g_vol_head++] = (uint8_t)(len >> 8);
        g_vol[g_vol_head++] = (uint8_t)(len & 0xFFu);
        memcpy(g_vol + g_vol_head, buf, len);
        g_vol_head += len;
    }
    return (int)len;
}

int storage_load_calib(ref_row_t *buf, uint16_t max_n)
{
    /* Search for marker in the RAM volume. */
    const char marker[] = "###CALIB###";
    if (!g_initialized) return 0;
    for (uint32_t i = 0; i + sizeof(marker) + 2u < g_vol_head; ++i) {
        if (memcmp(g_vol + i, marker, sizeof(marker) - 1) == 0) {
            uint16_t len = (uint16_t)((g_vol[i + sizeof(marker) - 1] << 8) |
                                       g_vol[i + sizeof(marker)]);
            uint16_t rows = len / 5u;
            if (rows > max_n) rows = max_n;
            const uint8_t *p = g_vol + i + sizeof(marker) + 2u;
            for (uint16_t r = 0; r < rows; ++r) {
                memcpy(buf[r].f, p + r * 5u, 4u);
                buf[r].cls = p[r * 5u + 4u];
            }
            return (int)rows;
        }
    }
    return 0;   /* no overlay yet — use embedded table */
}

/* Diagnostic: dump last N bytes of the volume to the ESP UART (for app
 * download via BLE). Returns bytes emitted. */
uint32_t storage_dump(uint32_t offset, uint32_t max_bytes, uint8_t *out)
{
    if (offset >= g_vol_head) return 0;
    uint32_t avail = g_vol_head - offset;
    if (max_bytes > avail) max_bytes = avail;
    memcpy(out, g_vol + offset, max_bytes);
    return max_bytes;
}

uint32_t storage_size(void) { return g_vol_head; }