/*
 * event_detect.c — Sag/swell/interruption/NILM event detection
 * WattLens — 3-Phase Power Quality Analyzer & NILM
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 *
 * IEC 61000-4-30 Class S event detection:
 *   Sag:            V_rms < 90% of nominal for > 10 ms (half-cycle)
 *   Swell:          V_rms > 110% of nominal for > 10 ms
 *   Interruption:   V_rms < 10% of nominal for > 10 ms
 *
 * NILM events: appliance on/off transitions with debounce.
 */

#include "event_detect.h"
#include "flicker.h"
#include <string.h>

#define NOMINAL_VOLTAGE_230   230.0f
#define NOMINAL_VOLTAGE_120   120.0f
#define SAG_THRESHOLD         0.90f    /* 90% of nominal */
#define SWELL_THRESHOLD       1.10f    /* 110% of nominal */
#define INT_THRESHOLD         0.10f    /* 10% of nominal */
#define FLICKER_PST_LIMIT     1.0f     /* Pst limit per IEC 61000-4-15 */
#define HARMONIC_THD_LIMIT    8.0f     /* THD limit per IEC 61000-4-7 (advisory) */

#define EVENT_QUEUE_SIZE      32

static pq_event_t event_queue[EVENT_QUEUE_SIZE];
static volatile int eq_head = 0, eq_tail = 0;

/* NILM debounce state */
static uint8_t nilm_prev_state[NILM_MAX_CLASSES];
static uint32_t nilm_debounce_ms[NILM_MAX_CLASSES];

void event_detect_init(void) {
    memset(event_queue, 0, sizeof(event_queue));
    memset(nilm_prev_state, 0, sizeof(nilm_prev_state));
    memset(nilm_debounce_ms, 0, sizeof(nilm_debounce_ms));
    eq_head = eq_tail = 0;
}

/* ========================================================================
 * Push event to queue
 * ======================================================================== */
static void push_event(const pq_event_t *evt) {
    int next = (eq_head + 1) % EVENT_QUEUE_SIZE;
    if (next == eq_tail) {
        /* Queue full — drop oldest */
        eq_tail = (eq_tail + 1) % EVENT_QUEUE_SIZE;
    }
    event_queue[eq_head] = *evt;
    eq_head = next;
}

/* ========================================================================
 * Check for voltage events (sag/swell/interruption)
 * ======================================================================== */
void event_detect_check(const power_metrics_t *m, device_ctx_t *ctx) {
    float nominal = (ctx->cal.grid_freq == 60) ? NOMINAL_VOLTAGE_120 : NOMINAL_VOLTAGE_230;
    uint32_t now = ctx->uptime_s;

    for (int phase = 0; phase < 3; phase++) {
        float v = m->v_rms[phase];
        float ratio = v / nominal;

        pq_event_t evt = {0};
        evt.timestamp = now;
        evt.phase = (uint8_t)phase;

        if (ratio < INT_THRESHOLD) {
            evt.type = PQ_EVENT_INTERRUPTION;
            evt.severity = v;
            evt.duration_ms = 1000;  /* 1-second window */
            push_event(&evt);
        } else if (ratio < SAG_THRESHOLD) {
            evt.type = PQ_EVENT_SAG;
            evt.severity = (nominal - v) / nominal * 100.0f;  /* % dip */
            evt.duration_ms = 1000;
            push_event(&evt);
        } else if (ratio > SWELL_THRESHOLD) {
            evt.type = PQ_EVENT_SWELL;
            evt.severity = (v - nominal) / nominal * 100.0f;  /* % rise */
            evt.duration_ms = 1000;
            push_event(&evt);
        }

        /* Harmonic exceedance */
        if (m->thd_v[phase] > HARMONIC_THD_LIMIT) {
            evt.type = PQ_EVENT_HARMONIC_EXCEED;
            evt.severity = m->thd_v[phase];
            evt.duration_ms = 1000;
            push_event(&evt);
        }
    }

    /* Flicker check */
    for (int phase = 0; phase < 3; phase++) {
        float pst = flicker_get_pst(phase);
        if (pst > FLICKER_PST_LIMIT) {
            pq_event_t evt = {0};
            evt.timestamp = now;
            evt.type = PQ_EVENT_FLICKER;
            evt.phase = (uint8_t)phase;
            evt.severity = pst;
            evt.duration_ms = 1000;
            push_event(&evt);
        }
    }
}

/* ========================================================================
 * NILM appliance on/off event detection with debounce
 * ======================================================================== */
void event_detect_nilm(const nilm_result_t *nilm, device_ctx_t *ctx) {
    uint32_t now = ctx->uptime_s;

    for (int c = 0; c < NILM_MAX_CLASSES; c++) {
        uint8_t current = nilm->nilm_state[c];
        uint8_t prev = nilm_prev_state[c];

        if (current != prev) {
            /* Check debounce: must be stable for EVENT_DEBOUNCE_S seconds */
            if (now - nilm_debounce_ms[c] >= EVENT_DEBOUNCE_S) {
                pq_event_t evt = {0};
                evt.timestamp = now;
                evt.type = current ? PQ_EVENT_NILM_ON : PQ_EVENT_NILM_OFF;
                evt.phase = 3;  /* all phases (aggregate) */
                evt.severity = nilm->nilm_power[c];
                evt.duration_ms = 0;
                push_event(&evt);

                nilm_prev_state[c] = current;
                nilm_debounce_ms[c] = now;
            }
        } else {
            nilm_debounce_ms[c] = now;  /* reset debounce timer if state is stable */
        }
    }
}

/* ========================================================================
 * Pop event from queue (returns 1 if event available, 0 if empty)
 * ======================================================================== */
int event_detect_pop(device_ctx_t *ctx, pq_event_t *evt) {
    (void)ctx;
    if (eq_head == eq_tail) return 0;  /* empty */
    *evt = event_queue[eq_tail];
    eq_tail = (eq_tail + 1) % EVENT_QUEUE_SIZE;
    return 1;
}