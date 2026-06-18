/*
 * apdetection.h — Plant action potential event detection
 * FloraPulse — Plant Electrophysiology & Sap-Flow Stress Monitor
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef FLORAPULSE_APDETECTION_H
#define FLORAPULSE_APDETECTION_H

#include <stdint.h>

/* Action potential event structure */
typedef struct {
    uint32_t timestamp_ms;   /* Time of event (system tick) */
    int16_t peak_uv;         /* Peak amplitude (µV) */
    uint16_t duration_ms;    /* Duration (ms) */
    uint8_t  channel;        /* 1 or 2 */
    uint8_t  classified;     /* 0=unknown, 1=drought, 2=wounding, 3=light, 4=herbivore */
} ap_event_t;

/* Initialize the AP detection engine */
void ap_detect_init(void);

/* Feed a new sample into the detection engine.
 * Call at sample rate (250 SPS).
 * ch: 1 or 2, value_uv: signal in microvolts.
 * Returns 1 if an event was detected, 0 otherwise.
 * If event detected, fills *evt with event details.
 */
uint8_t ap_detect_feed(uint8_t ch, float value_uv, ap_event_t *evt);

/* Set detection threshold (µV) — adaptive baseline + N*sigma */
void ap_detect_set_threshold(float threshold_uv);

/* Get current threshold */
float ap_detect_get_threshold(void);

/* Enable adaptive threshold mode (baseline wander tracking) */
void ap_detect_enable_adaptive(uint8_t enable);

/* Get event count in last N seconds */
uint16_t ap_detect_event_count(uint16_t seconds);

/* Get firing rate (events per minute) */
float ap_detect_firing_rate(void);

/* Classify an event based on waveform morphology.
 * Uses amplitude, duration, and shape (rise time, decay time).
 * Returns classification code (see ap_event_t.classified).
 */
uint8_t ap_detect_classify(const ap_event_t *evt);

/* Get pointer to event buffer for export */
const ap_event_t *ap_detect_get_events(uint16_t *count);

#endif /* FLORAPULSE_APDETECTION_H */