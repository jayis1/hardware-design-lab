/*
 * sdlog.c — MicroSD Card SEG-Y Logging
 * StrataScan — Open-Source Stepped-Frequency Ground Penetrating Radar
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Implements SEG-Y (rev 1.0) format logging of GPR B-scan traces to a
 * MicroSD card via the STM32H743 SDMMC1 interface (4-bit mode).  Each survey
 * produces a .sgy file containing the trace data with headers encoding the
 * band preset, permittivity, GNSS position, and trace number.
 *
 * SEG-Y format (simplified for GPR):
 *  - 3200-byte EBCDIC text header (we use ASCII)
 *  - 400-byte binary header (sample interval, samples per trace, etc.)
 *  - Per-trace: 240-byte trace header + trace data (float32 samples)
 *
 * The SD card is accessed via a minimal FAT32 filesystem layer.  This driver
 * uses the SDMMC1 peripheral in 4-bit mode with DMA for high throughput.
 * For simplicity, the FAT32 layer is stubbed — file operations use raw
 * sector writes at pre-allocated offsets (a real implementation would link
 * a FAT filesystem library like FatFS).
 */

#include "sdlog.h"
#include "../board.h"
#include "../registers.h"
#include <string.h>

/* ===================================================================== */
/*  SEG-Y constants                                                       */
/* ===================================================================== */

#define SEGY_TEXT_HEADER_SIZE   3200
#define SEGY_BIN_HEADER_SIZE     400
#define SEGY_TRACE_HEADER_SIZE   240
#define SEGY_TEXT_HEADER_SECTORS  7   /* 3200 / 512 ≈ 7 */
#define SEGY_BIN_HEADER_SECTORS   1   /* 400 / 512 ≈ 1 */

/* ===================================================================== */
/*  SDMMC1 minimal driver (4-bit mode)                                    */
/* ===================================================================== */

static uint8_t sd_initialized = 0;
static uint32_t sd_sector_offset = 0;  /* current write sector */

static int sdmmc_wait_flag(uint32_t mask, uint32_t timeout)
{
    /* Simplified: poll SDMMC1 status register */
    uint32_t t = 0;
    volatile uint32_t *sta = (volatile uint32_t *)(SDMMC1_BASE + 0x34);
    while (!(*sta & mask) && t < timeout) { t++; }
    return (t < timeout) ? 0 : -1;
}

static int sd_write_sector(uint32_t sector, const uint8_t *data)
{
    /* Minimal SDMMC1 block write — a real implementation configures the
     * SDMMC1 peripheral with clock, sends CMD24 (WRITE_SINGLE_BLOCK),
     * then DMA-transfers 512 bytes.  This is a stub that simulates success.
     */
    (void)sector;
    (void)data;
    /* In a complete implementation:
     *  1. SDMMC1->CLKCR = clock_div | 4-bit mode enable
     *  2. Send CMD0 (reset), CMD8 (SEND_IF_COND), ACMD41 (init), CMD2, CMD3
     *  3. Send CMD24 with sector address
     *  4. DMA2 Stream3 → SDMMC1 FIFO (512 bytes)
     *  5. Wait for CRC status + programming complete
     */
    return 0;
}

/* ===================================================================== */
/*  Simple buffered sector writer                                        */
/* ===================================================================== */

static uint8_t  sector_buf[512];
static uint16_t sector_pos = 0;

static int flush_sector(void)
{
    if (sector_pos == 0) return 0;
    /* Zero-fill the rest of the sector */
    while (sector_pos < 512) {
        sector_buf[sector_pos++] = 0;
    }
    int rc = sd_write_sector(sd_sector_offset, sector_buf);
    sd_sector_offset++;
    sector_pos = 0;
    return rc;
}

static int write_bytes(const uint8_t *data, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++) {
        sector_buf[sector_pos++] = data[i];
        if (sector_pos >= 512) {
            if (flush_sector()) return -1;
        }
    }
    return 0;
}

static int write_u16_be(uint16_t val)
{
    uint8_t buf[2] = { (uint8_t)(val >> 8), (uint8_t)(val & 0xFF) };
    return write_bytes(buf, 2);
}

static int write_u32_be(uint32_t val)
{
    uint8_t buf[4] = { (uint8_t)(val >> 24), (uint8_t)(val >> 16),
                       (uint8_t)(val >> 8), (uint8_t)(val & 0xFF) };
    return write_bytes(buf, 4);
}

static int write_float_be(float val)
{
    uint8_t *p = (uint8_t *)&val;
    /* IEEE 754 big-endian */
    uint8_t buf[4] = { p[3], p[2], p[1], p[0] };
    return write_bytes(buf, 4);
}

static int write_i32_be(int32_t val)
{
    uint8_t buf[4] = { (uint8_t)(val >> 24), (uint8_t)(val >> 16),
                       (uint8_t)(val >> 8), (uint8_t)(val & 0xFF) };
    return write_bytes(buf, 4);
}

/* ===================================================================== */
/*  SEG-Y header construction                                             */
/* ===================================================================== */

static int write_text_header(void)
{
    /* 3200-byte ASCII text header (40 lines × 80 chars) */
    char line[81];
    /* Line 1: survey description */
    memset(line, ' ', 80);
    line[80] = '\0';
    const char *desc = "StrataScan GPR Survey by jayis1 — SFCW Ground Penetrating Radar";
    uint8_t dl = (uint8_t)strlen(desc);
    if (dl > 80) dl = 80;
    memcpy(line, desc, dl);
    write_bytes((uint8_t *)line, 80);

    /* Fill remaining 39 lines with spaces */
    memset(line, ' ', 80);
    for (int i = 0; i < 39; i++) {
        write_bytes((uint8_t *)line, 80);
    }
    return 0;
}

static int write_binary_header(uint16_t samples_per_trace,
                                uint16_t sample_interval_us)
{
    /* 400-byte binary header (big-endian) */
    /* Key fields (SEG-Y rev 1.0): */
    write_u16_be(3000);   /* job ID */
    write_u16_be(0);      /* line number */
    write_u16_be(0);      /* reel number */
    write_u16_be(samples_per_trace);  /* traces per ensemble */
    write_u16_be(1);      /* aux traces per ensemble */
    write_u16_be(sample_interval_us); /* sample interval (μs) */
    write_u16_be(0);      /* sample interval (original, μs) */
    write_u16_be(samples_per_trace); /* samples per trace */
    write_u16_be(samples_per_trace); /* samples per trace (original) */
    write_u16_be(5);      /* data sample format code: 5 = IEEE float32 */
    write_u16_be(1);      /* measurement system: 1 = meters */

    /* Zero-fill remaining 400 - 44 = 356 bytes */
    uint8_t zeros[356];
    memset(zeros, 0, sizeof(zeros));
    write_bytes(zeros, 356);
    return 0;
}

static int write_trace_header(uint32_t trace_num, float lat, float lon)
{
    /* 240-byte trace header (big-endian) */
    write_u32_be(trace_num);     /* trace sequence number */
    write_u32_be(trace_num);     /* trace sequence number (line) */
    write_u32_be(0);              /* field record number */
    write_u32_be(0);              /* trace number within field record */
    write_u32_be(1);              /* energy source point */
    write_u32_be(0);              /* CDP ensemble number */
    /* Source/receiver coordinates (lat/lon → scaled integers) */
    write_i32_be((int32_t)(lat * 1e7));  /* source X */
    write_i32_be((int32_t)(lon * 1e7));  /* source Y */
    write_i32_be((int32_t)(lat * 1e7));  /* receiver X */
    write_i32_be((int32_t)(lon * 1e7));  /* receiver Y */

    /* Zero-fill remaining 240 - 56 = 184 bytes */
    uint8_t zeros[184];
    memset(zeros, 0, sizeof(zeros));
    write_bytes(zeros, 184);
    return 0;
}

/* ===================================================================== */
/*  Public API                                                            */
/* ===================================================================== */

int sdlog_init(void)
{
    /* Configure SDMMC1 for 4-bit mode at 24 MHz (max for standard SD) */
    /* A full implementation configures:
     *  - SDMMC1 clock divider
     *  - CMD line pull-up
     *  - SD card initialization sequence (CMD0, CMD8, ACMD41, CMD2, CMD3)
     *  - Block size = 512
     */
    sd_initialized = 1;
    sd_sector_offset = 0;
    sector_pos = 0;
    return 0;
}

int sdlog_start_survey(uint8_t band_idx, float eps_r)
{
    if (!sd_initialized) return -1;

    /* Start writing at sector 0 (raw mode — no FAT) */
    sd_sector_offset = 0;
    sector_pos = 0;

    /* Write EBCDIC/ASCII text header (3200 bytes) */
    write_text_header();

    /* Write binary header (400 bytes) */
    const band_preset_t *bp = &band_presets[band_idx];
    uint16_t samples = bp->num_steps;
    uint16_t interval = bp->dwell_us;
    write_binary_header(samples, interval);

    /* Flush partial sector (headers may not align to 512 bytes) */
    flush_sector();

    return 0;
}

int sdlog_write_trace(const float *trace, uint16_t n,
                       uint32_t trace_num, float lat, float lon)
{
    if (!sd_initialized) return -1;

    /* Write 240-byte trace header */
    write_trace_header(trace_num, lat, lon);

    /* Write n × 4-byte float samples (IEEE 754 big-endian) */
    for (uint16_t i = 0; i < n; i++) {
        write_float_be(trace[i]);
    }

    /* Flush if we're near a sector boundary */
    if (sector_pos > 400) {
        flush_sector();
    }

    return 0;
}

int sdlog_end_survey(void)
{
    if (!sd_initialized) return -1;
    /* Flush any remaining data */
    flush_sector();
    return 0;
}

int sdlog_sync(void)
{
    /* Force flush current sector to card */
    return flush_sector();
}

/* End of sdlog.c */