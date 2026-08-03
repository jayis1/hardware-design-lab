/*
 * stroke.c — Stroke segmentation for Inkwell
 *
 * Consumes per-sample (dx, dy, pressure) tuples from the dead-reckoner and
 * the pen-lift FSM, accumulates them, and every 20 ms (or at pen-lift)
 * produces a stroke_segment_t record. The flags field marks stroke-start
 * (first segment after a pen-down transition), stroke-end (the segment
 * containing a pen-up transition), pen-down (active), and optical-flow
 * validity (mirrored from the dead-reckoner state). Each record is
 * timestamped and sequence-numbered so the app can reorder and verify
 * completeness.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#include "stroke.h"
#include "../board.h"
#include "../registers.h"
#include <string.h>

#define STROKE_QUEUE_LEN 32

static stroke_segment_t g_queue[STROKE_QUEUE_LEN];
static volatile uint32_t g_head, g_tail;

static uint32_t g_seq = 0;
static uint32_t g_last_flush_ms = 0;
static uint32_t g_session_start_ms = 0;

/* Accumulators for the current 20 ms segment. */
static int32_t  g_acc_dx_um = 0;
static int32_t  g_acc_dy_um = 0;
static uint32_t g_acc_p_sum = 0;
static uint32_t g_acc_p_count = 0;
static uint8_t  g_seg_flags = 0;
static bool     g_in_stroke = false;
static bool     g_first_segment_of_stroke = false;

/* Pen-lift edge tracking (independent of pressure.c FSM so we can mark
 * stroke-start / stroke-end correctly). */
static bool     g_prev_pen_down = false;

/* CRC-8 (poly 0x07, init 0x00) — used for record integrity. */
static uint8_t crc8(const uint8_t *data, uint32_t len)
{
    uint8_t crc = 0x00;
    for (uint32_t i = 0; i < len; ++i) {
        crc ^= data[i];
        for (uint32_t b = 0; b < 8; ++b) {
            if (crc & 0x80) crc = (uint8_t)((crc << 1) ^ 0x07);
            else            crc = (uint8_t)(crc << 1);
        }
    }
    return crc;
}

void stroke_init(uint16_t pen_down_mN, uint16_t pen_up_mN, uint8_t debounce)
{
    (void)pen_down_mN; (void)pen_up_mN; (void)debounce;
    g_head = g_tail = 0;
    g_seq = 0;
    g_last_flush_ms = 0;
    g_session_start_ms = 0;
    g_acc_dx_um = g_acc_dy_um = 0;
    g_acc_p_sum = 0; g_acc_p_count = 0;
    g_seg_flags = 0;
    g_in_stroke = false;
    g_first_segment_of_stroke = false;
    g_prev_pen_down = false;
}

static void enqueue_segment(uint32_t now_ms, bool force_end)
{
    if (g_acc_p_count == 0 && !force_end) return;  /* nothing to send */

    stroke_segment_t *s = &g_queue[g_head];
    s->seq   = g_seq++;
    s->ts_ms = now_ms - g_session_start_ms;
    s->dx_um = g_acc_dx_um;
    s->dy_um = g_acc_dy_um;
    s->p_mN  = (g_acc_p_count > 0)
             ? (uint16_t)(g_acc_p_sum / g_acc_p_count)
             : 0;

    uint8_t flags = 0;
    if (g_in_stroke)                 flags |= 0x01;   /* pen-down */
    if (g_first_segment_of_stroke)   flags |= 0x02;   /* stroke-start */
    if (force_end)                   flags |= 0x04;   /* stroke-end */
    /* bit3 (optical-flow-valid) would be set by dead_reckon state mirror. */
    s->flags = flags;

    /* Compute CRC over the first 19 bytes (everything but the crc field). */
    s->crc8 = 0;
    s->crc8 = crc8((const uint8_t *)s, sizeof(*s) - 1);

    g_head = (g_head + 1) % STROKE_QUEUE_LEN;

    /* Reset accumulators. */
    g_acc_dx_um = 0;
    g_acc_dy_um = 0;
    g_acc_p_sum = 0;
    g_acc_p_count = 0;
    g_first_segment_of_stroke = false;
}

void stroke_feed(uint32_t ts_ms, int32_t dx_um, int32_t dy_um,
                 uint16_t p_mN, bool pen_down)
{
    if (g_session_start_ms == 0) g_session_start_ms = ts_ms;

    /* Detect pen-down edge → stroke-start. */
    if (pen_down && !g_prev_pen_down) {
        g_in_stroke = true;
        g_first_segment_of_stroke = true;
    }
    /* Detect pen-up edge → flush a final segment marked stroke-end. */
    if (!pen_down && g_prev_pen_down) {
        g_in_stroke = false;
        enqueue_segment(ts_ms, true);  /* force-end */
        g_prev_pen_down = pen_down;
        g_last_flush_ms = ts_ms;
        return;
    }
    g_prev_pen_down = pen_down;

    /* Only accumulate while pen is down (pen-up samples contribute nothing). */
    if (pen_down) {
        g_acc_dx_um += dx_um;
        g_acc_dy_um += dy_um;
        g_acc_p_sum += p_mN;
        g_acc_p_count++;
    }

    /* Time-based flush every 20 ms. */
    if ((ts_ms - g_last_flush_ms) >= BLE_SEGMENT_PERIOD_MS) {
        enqueue_segment(ts_ms, false);
        g_last_flush_ms = ts_ms;
    }
}

bool stroke_pop_segment(stroke_segment_t *out)
{
    if (g_tail == g_head) return false;
    *out = g_queue[g_tail];
    g_tail = (g_tail + 1) % STROKE_QUEUE_LEN;
    return true;
}