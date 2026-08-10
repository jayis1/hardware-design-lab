/**
 * @file    storage.h
 * @brief   TideBand — NAND flash storage driver for dive profile logging.
 * @author  jayis1
 * @copyright © 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 */

#ifndef TIDEBAND_STORAGE_H
#define TIDEBAND_STORAGE_H

#include <stdint.h>

/* ---- Profile record (48 bytes) ---- */
typedef struct __attribute__((packed)) {
    uint32_t timestamp;    /* Seconds since epoch (from RTC) */
    float    depth_m;      /* Depth in meters */
    float    temp_c;       /* Water temperature °C */
    float    vn_ms;        /* North velocity (m/s) */
    float    ve_ms;        /* East velocity (m/s) */
    float    vu_ms;        /* Up velocity (m/s) */
    float    speed_ms;     /* Current speed magnitude (m/s) */
    float    heading_deg;  /* Current heading (deg from north) */
    int16_t  roll_deg;     /* Attitude roll (deg ×1) */
    int16_t  pitch_deg;    /* Attitude pitch (deg ×1) */
    uint8_t  quality;      /* 0-3 quality score */
    uint8_t  reserved;     /* Padding to 48 bytes */
    uint16_t crc;          /* CRC16 of record */
} profile_record_t;

_Static_assert(sizeof(profile_record_t) == 48, "Record must be 48 bytes");

/* ---- Dive session metadata (stored at start of each dive block) ---- */
typedef struct __attribute__((packed)) {
    uint32_t magic;          /* 0xD1VE544D = "DIVE" */
    uint32_t dive_id;        /* Unique dive ID (incrementing) */
    uint32_t start_time;     /* Start timestamp */
    uint32_t end_time;       /* End timestamp (0 if ongoing) */
    float    max_depth_m;    /* Maximum depth reached */
    float    avg_current_ms; /* Average current speed during dive */
    uint16_t record_count;   /* Number of profile records in this dive */
    uint16_t sample_rate_hz; /* Sample rate used for this dive */
    uint8_t  reserved[24];   /* Future expansion */
    uint16_t crc;            /* CRC16 of header */
} dive_header_t;

#define DIVE_MAGIC 0x44495645u  /* "DIVE" */

/* ---- Public API ---- */

/** Initialize NAND flash and wear-leveling layer. */
int storage_init(void);

/** Start a new dive session. Returns dive ID (>0) or -1 on error. */
int storage_start_dive(uint32_t timestamp, uint16_t sample_rate_hz);

/** Append a profile record to the current dive. Returns 0 on success. */
int storage_write_record(const profile_record_t *rec);

/** End the current dive session (writes final header). */
int storage_end_dive(uint32_t timestamp);

/** Read a dive header by index (0-based). Returns 0 on success. */
int storage_read_dive_header(uint16_t index, dive_header_t *hdr);

/** Read a profile record by absolute record index. */
int storage_read_record(uint32_t index, profile_record_t *rec);

/** Erase all stored dives (factory reset of log). */
int storage_erase_all(void);

/** Get total number of dives stored. */
uint16_t storage_get_dive_count(void);

/** Get free space in bytes. */
uint32_t storage_get_free_bytes(void);

#endif /* TIDEBAND_STORAGE_H */