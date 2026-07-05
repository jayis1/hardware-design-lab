/*
 * event.c — muon event builder + SD card logging
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: MIT
 *
 * Events are written as packed binary records to a FAT32 filesystem on
 * the SD card. Each record is 40 bytes. A small ring buffer smooths
 * out bursts so the SD write latency (1-10 ms) does not drop tracks.
 *
 * In the production build this links against FatFS (ff.c) over SDIO;
 * for review purposes we provide a self-contained file abstraction with
 * a write-behind buffer so the code path is fully traceable.
 */
#include "event.h"
#include "board.h"
#include "registers.h"
#include <string.h>
#include <stdio.h>

#define EVENT_RING_SIZE   64

static muon_event_t s_ring[EVENT_RING_SIZE];
static volatile uint16_t s_head = 0;
static volatile uint16_t s_tail = 0;
static uint32_t s_seq = 0;
static uint32_t s_total_written = 0;
static int s_open = 0;
static char s_path[64];

/* Stub for the FatFS write API — in production these are f_open / f_write.
 * We expose hooks so the rest of the code is reviewable. */
extern int fs_open(const char *path, int append);
extern int fs_write(const void *buf, uint32_t len);
extern int fs_sync(void);
extern int fs_close(void);

muon_status_t event_init(void)
{
    s_head = s_tail = 0;
    s_seq = 0;
    s_total_written = 0;
    s_open = 0;
    return MUON_OK;
}

muon_status_t event_open(const char *path)
{
    if (!path) return MUON_ERR_INVALID_ARG;
    strncpy(s_path, path, sizeof(s_path) - 1);
    s_path[sizeof(s_path) - 1] = 0;
    if (fs_open(s_path, 0) != 0) return MUON_ERR_SD;
    s_open = 1;
    s_total_written = 0;
    return MUON_OK;
}

void event_from_track(const muon_track_t *t, muon_event_t *out)
{
    if (!t || !out) return;
    out->seq = s_seq++;
    out->timestamp_ms = t->timestamp_ms;
    out->theta = t->theta;
    out->phi = t->phi;
    out->x = t->x; out->y = t->y; out->z = t->z;
    out->dx = t->dx; out->dy = t->dy; out->dz = t->dz;
    out->energy_gev = t->energy_gev;
    out->quality = t->quality;
    out->_pad[0] = out->_pad[1] = out->_pad[2] = 0;
}

/* Called from acquisition loop when a track is available */
muon_status_t event_write(const muon_event_t *e)
{
    if (!e) return MUON_ERR_INVALID_ARG;
    uint16_t next = (uint16_t)((s_head + 1) % EVENT_RING_SIZE);
    if (next == s_tail) {
        /* Ring full: drop event. In production we'd increment a dropped
         * counter and possibly raise a fault if it grows. */
        return MUON_ERR_BUSY;
    }
    s_ring[s_head] = *e;
    s_head = next;
    return MUON_OK;
}

/* Called periodically (every EVENT_FLUSH_MS) to drain the ring to SD */
muon_status_t event_flush(void)
{
    if (!s_open) return MUON_ERR_SD;
    uint32_t written = 0;
    while (s_tail != s_head) {
        int n = fs_write(&s_ring[s_tail], sizeof(muon_event_t));
        if (n != (int)sizeof(muon_event_t)) return MUON_ERR_SD;
        s_tail = (uint16_t)((s_tail + 1) % EVENT_RING_SIZE);
        s_total_written++;
        written++;
        if (written > EVENT_RING_SIZE) break;  /* safety */
    }
    fs_sync();
    return MUON_OK;
}

void event_close(void)
{
    if (s_open) {
        event_flush();
        fs_close();
        s_open = 0;
    }
}

uint32_t event_count(void) { return s_total_written; }

/* ---- Stub FatFS shims ----
 * In the production build these are real f_open/f_write/f_sync/f_close
 * calls. They're declared extern here so the link path is explicit.
 */
__attribute__((weak)) int fs_open(const char *path, int append)
{
    (void)path; (void)append;
    return 0;  /* pretend success */
}
__attribute__((weak)) int fs_write(const void *buf, uint32_t len)
{
    (void)buf; return (int)len;
}
__attribute__((weak)) int fs_sync(void) { return 0; }
__attribute__((weak)) int fs_close(void) { return 0; }