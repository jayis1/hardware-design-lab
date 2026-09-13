/*
 * HingeScribe firmware APIs
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */
#ifndef HINGESCRIBE_H
#define HINGESCRIBE_H
#include "board.h"

typedef struct {
    door_state_t state;
    hs_sample_t previous;
    hs_event_t active;
    float force_baseline;
    float force_variance;
    float angle_closed_ema;
    float sag_baseline_deg;
    uint32_t last_transition_ms;
    uint32_t last_sample_ms;
    bool event_active;
    bool calibrated;
} hs_analyzer_t;

typedef struct {
    hs_event_t records[HS_HISTORY_RECORDS];
    uint16_t head;
    uint16_t count;
} hs_history_t;

void sensor_init(void);
hs_result_t sensor_read(hs_sample_t *sample, const hs_calibration_t *calibration);
hs_result_t sensor_tare(hs_calibration_t *calibration, unsigned samples);
void analyzer_init(hs_analyzer_t *analyzer, const hs_calibration_t *calibration);
bool analyzer_update(hs_analyzer_t *analyzer, const hs_sample_t *sample, hs_event_t *completed);
uint16_t analyzer_health_score(const hs_analyzer_t *analyzer, const hs_event_t *event);
float analyzer_estimate_sag(const hs_analyzer_t *analyzer, float closed_angle);
void history_init(hs_history_t *history);
void history_append(hs_history_t *history, const hs_event_t *event);
bool history_get(const hs_history_t *history, unsigned newest_index, hs_event_t *event);
uint32_t hs_crc32(const uint8_t *data, size_t length);
uint16_t hs_crc16(const uint8_t *data, size_t length);
size_t protocol_encode_status(const hs_sample_t *sample, const hs_event_t *event, uint8_t *out, size_t capacity);
size_t protocol_encode_history(const hs_history_t *history, unsigned index, uint8_t *out, size_t capacity);
hs_result_t protocol_handle_command(const uint8_t *packet, size_t length, hs_calibration_t *calibration, hs_history_t *history);
bool calibration_load(hs_calibration_t *calibration);
bool calibration_save(hs_calibration_t *calibration);

#endif
