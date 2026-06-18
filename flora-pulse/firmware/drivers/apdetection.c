/*
 * apdetection.c — Plant action potential event detection
 * FloraPulse — Plant Electrophysiology & Sap-Flow Stress Monitor
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Detects action potential (AP) events in the plant electrophysiology
 * signal stream.  Plant APs are slow (1–10 s duration) compared to
 * neural APs (1–2 ms), with amplitudes of 10–500 µV at the surface
 * electrode.
 *
 * Detection algorithm:
 *  1. Maintain a sliding-window baseline (exponential moving average).
 *  2. Compute the short-term RMS over a 100 ms window.
 *  3. If |sample - baseline| > threshold × RMS, mark as potential event.
 *  4. Track event onset and peak; when signal returns to baseline,
 *     classify the event by amplitude and duration.
 *
 * Classification (morphology-based):
 *  - Drought stress: 50–200 µV, 2–5 s, slow rise
 *  - Wounding:       200–500 µV, 1–3 s, rapid rise
 *  - Light response: 20–80 µV, 5–10 s, gradual rise
 *  - Herbivory:      100–300 µV, 1–4 s, with rapid fluctuations
 */

#include "apdetection.h"
#include "../board.h"
#include "../registers.h"
#include <string.h>

/* ===================================================================== */
/*  Configuration                                                        */
/* ===================================================================== */

#define AP_MAX_EVENTS     64U
#define AP_BASELINE_ALPHA 0.001f   /* EMA time constant (slow) */
#define AP_RMS_WINDOW     25U      /* 100 ms at 250 SPS */
#define AP_REFRACTORY_MS  500U     /* Minimum time between events */

/* ===================================================================== */
/*  Internal state                                                       */
/* ===================================================================== */

static ap_event_t event_buf[AP_MAX_EVENTS];
static volatile uint16_t event_head = 0;
static volatile uint16_t event_count = 0;

static float baseline = 0.0f;
static float threshold_uv = 50.0f;
static uint8_t adaptive_enabled = 1;

/* Event tracking */
static uint8_t in_event = 0;
static uint32_t event_start_ms = 0;
static float event_peak_uv = 0.0f;
static uint8_t event_channel = 0;
static uint32_t last_event_ms = 0;

/* RMS computation buffer */
static float rms_buf[AP_RMS_WINDOW];
static uint8_t rms_idx = 0;

/* Event rate tracking */
static uint32_t event_timestamps[16];
static uint8_t ts_head = 0;
static uint8_t ts_count = 0;

static volatile uint32_t ap_tick_ms = 0;

/* ===================================================================== */
/*  Helper: exponential moving average                                   */
/* ===================================================================== */

static float ema_update(float current, float new_val, float alpha)
{
    return current + alpha * (new_val - current);
}

/* ===================================================================== */
/*  Public API                                                           */
/* ===================================================================== */

void ap_detect_init(void)
{
    baseline = 0.0f;
    event_head = 0;
    event_count = 0;
    in_event = 0;
    event_peak_uv = 0.0f;
    rms_idx = 0;
    ts_head = 0;
    ts_count = 0;

    for (int i = 0; i < AP_RMS_WINDOW; i++)
        rms_buf[i] = 0.0f;
}

uint8_t ap_detect_feed(uint8_t ch, float value_uv, ap_event_t *evt)
{
    uint8_t event_detected = 0;

    /* Update baseline (slow EMA) */
    if (adaptive_enabled) {
        baseline = ema_update(baseline, value_uv, AP_BASELINE_ALPHA);
    }

    /* Update RMS buffer */
    rms_buf[rms_idx] = value_uv;
    rms_idx = (rms_idx + 1) % AP_RMS_WINDOW;

    /* Compute RMS */
    float sum_sq = 0.0f;
    for (int i = 0; i < AP_RMS_WINDOW; i++)
        sum_sq += rms_buf[i] * rms_buf[i];
    float rms = sqrtf(sum_sq / (float)AP_RMS_WINDOW);

    /* Compute deviation from baseline */
    float deviation = value_uv - baseline;
    float abs_dev = deviation < 0 ? -deviation : deviation;

    /* Adaptive threshold: max(fixed_threshold, baseline_noise × 3) */
    float effective_threshold = threshold_uv;
    if (adaptive_enabled) {
        float noise = rms * 3.0f;
        if (noise > effective_threshold)
            effective_threshold = noise;
    }

    /* Detection logic */
    if (!in_event) {
        /* Check for event onset */
        if (abs_dev > effective_threshold) {
            /* Check refractory period */
            if (ap_tick_ms - last_event_ms > AP_REFRACTORY_MS) {
                in_event = 1;
                event_start_ms = ap_tick_ms;
                event_peak_uv = deviation;
                event_channel = ch;
            }
        }
    } else {
        /* In event: track peak */
        if (abs_dev > (event_peak_uv < 0 ? -event_peak_uv : event_peak_uv)) {
            event_peak_uv = deviation;
        }

        /* Check for event end (signal returns near baseline) */
        if (abs_dev < effective_threshold * 0.3f) {
            /* Event complete */
            uint32_t duration = ap_tick_ms - event_start_ms;

            if (duration > 100) {  /* Minimum 100 ms to be a real event */
                ap_event_t *e = &event_buf[event_head];
                e->timestamp_ms = event_start_ms;
                e->peak_uv = (int16_t)event_peak_uv;
                e->duration_ms = (uint16_t)duration;
                e->channel = event_channel;
                e->classified = ap_detect_classify(e);

                event_head = (event_head + 1) % AP_MAX_EVENTS;
                if (event_count < AP_MAX_EVENTS)
                    event_count++;

                last_event_ms = ap_tick_ms;

                /* Track for firing rate */
                event_timestamps[ts_head] = event_start_ms;
                ts_head = (ts_head + 1) % 16;
                if (ts_count < 16) ts_count++;

                if (evt) {
                    *evt = *e;
                }
                event_detected = 1;
            }
            in_event = 0;
            event_peak_uv = 0.0f;
        }
    }

    return event_detected;
}

void ap_detect_set_threshold(float threshold_uv_val)
{
    threshold_uv = threshold_uv_val;
}

float ap_detect_get_threshold(void)
{
    return threshold_uv;
}

void ap_detect_enable_adaptive(uint8_t enable)
{
    adaptive_enabled = enable;
}

uint16_t ap_detect_event_count(uint16_t seconds)
{
    uint32_t cutoff = ap_tick_ms - (uint32_t)seconds * 1000U;
    uint16_t count = 0;

    for (uint8_t i = 0; i < ts_count; i++) {
        uint8_t idx = (ts_head + 16 - ts_count + i) % 16;
        if (event_timestamps[idx] >= cutoff)
            count++;
    }
    return count;
}

float ap_detect_firing_rate(void)
{
    if (ts_count < 2) return 0.0f;

    /* Compute events per minute over the window */
    uint32_t oldest = event_timestamps[(ts_head + 16 - ts_count) % 16];
    uint32_t newest = event_timestamps[(ts_head + 15) % 16];
    uint32_t span_ms = newest - oldest;

    if (span_ms == 0) return 0.0f;

    return (float)ts_count * 60000.0f / (float)span_ms;
}

uint8_t ap_detect_classify(const ap_event_t *evt)
{
    float amp = evt->peak_uv < 0 ? -evt->peak_uv : evt->peak_uv;
    uint16_t dur = evt->duration_ms;

    /* Classification rules based on amplitude and duration */
    if (amp < 80.0f && dur > 4000) {
        return 3;  /* Light response (low amp, long duration) */
    } else if (amp >= 200.0f && dur < 3000) {
        return 2;  /* Wounding (high amp, short duration) */
    } else if (dur > 5000 && amp > 100.0f) {
        return 1;  /* Drought stress (medium amp, long duration) */
    } else if (dur < 4000 && amp > 80.0f) {
        return 4;  /* Herbivory (medium amp, medium duration) */
    }

    return 0;  /* Unknown */
}

const ap_event_t *ap_detect_get_events(uint16_t *count)
{
    *count = event_count;
    return event_buf;
}

void ap_detect_set_tick(uint32_t ms)
{
    ap_tick_ms = ms;
}

/* Math function declarations */
extern double sqrtf(double x);