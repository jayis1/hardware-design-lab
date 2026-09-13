/*
 * HingeScribe motion analysis and event classification
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */
#include "hingescribe.h"
#include <math.h>
#include <string.h>

static float absolute(float value) { return value < 0.0f ? -value : value; }
static float maximum(float a, float b) { return a > b ? a : b; }
static float clamp(float value, float low, float high)
{
    if (value < low) return low;
    if (value > high) return high;
    return value;
}

static door_state_t classify_state(const hs_sample_t *sample, door_state_t previous)
{
    if (!sample->magnet_valid) return DOOR_UNKNOWN;
    if (sample->angle_deg <= CLOSED_THRESHOLD_DEG && absolute(sample->angular_velocity_dps) < 2.0f) return DOOR_CLOSED;
    if (sample->angular_velocity_dps > 3.0f) return DOOR_OPENING;
    if (sample->angular_velocity_dps < -3.0f) return DOOR_CLOSING;
    if (sample->angle_deg > OPEN_THRESHOLD_DEG) return DOOR_OPEN;
    if (previous == DOOR_CLOSING && sample->force_n > STALL_FORCE_N) return DOOR_OBSTRUCTED;
    return previous;
}

static bool begins_event(door_state_t old_state, door_state_t new_state)
{
    return old_state == DOOR_CLOSED && (new_state == DOOR_OPENING || new_state == DOOR_OPEN);
}

static bool ends_event(const hs_analyzer_t *analyzer, const hs_sample_t *sample, door_state_t next)
{
    if (!analyzer->event_active) return false;
    if (next != DOOR_CLOSED) return false;
    /* A closed classification already requires low angular velocity. The
     * timestamp guard prevents a zero-duration bounce without delaying the
     * completed event until another sample. */
    return sample->timestamp_ms - analyzer->active.started_ms >= EVENT_DEBOUNCE_MS;
}

static void start_event(hs_analyzer_t *analyzer, const hs_sample_t *sample)
{
    memset(&analyzer->active, 0, sizeof(analyzer->active));
    analyzer->active.sequence++;
    analyzer->active.started_ms = sample->timestamp_ms;
    analyzer->active.peak_open_force_n = sample->force_n;
    analyzer->active.peak_close_force_n = 0.0f;
    analyzer->active.max_velocity_dps = absolute(sample->angular_velocity_dps);
    analyzer->active.health_score = 100u;
    analyzer->event_active = true;
}

static void accumulate_event(hs_analyzer_t *analyzer, const hs_sample_t *sample, door_state_t state)
{
    hs_event_t *event = &analyzer->active;
    float velocity = absolute(sample->angular_velocity_dps);
    event->max_velocity_dps = maximum(event->max_velocity_dps, velocity);
    event->harvested_mj = sample->harvested_mj;
    if (state == DOOR_OPENING) event->peak_open_force_n = maximum(event->peak_open_force_n, absolute(sample->force_n));
    if (state == DOOR_CLOSING || state == DOOR_OBSTRUCTED) event->peak_close_force_n = maximum(event->peak_close_force_n, absolute(sample->force_n));
    if (!sample->magnet_valid || !sample->load_valid) event->flags |= HS_FLAG_SENSOR_FAULT;
    if (sample->battery_mv < BATTERY_MIN_MV) event->flags |= HS_FLAG_LOW_BATTERY;
    if (state == DOOR_OBSTRUCTED) event->flags |= HS_FLAG_OBSTRUCTED;
    if (state == DOOR_CLOSING && velocity > 220.0f) event->flags |= HS_FLAG_SLAM;
}

static void finish_event(hs_analyzer_t *analyzer, const hs_sample_t *sample)
{
    hs_event_t *event = &analyzer->active;
    event->duration_ms = sample->timestamp_ms - event->started_ms;
    event->closing_time_s = (float)event->duration_ms / 1000.0f;
    event->final_angle_deg = sample->angle_deg;
    event->estimated_sag_mm = analyzer_estimate_sag(analyzer, sample->angle_deg);
    if (event->peak_open_force_n > STALL_FORCE_N) event->flags |= HS_FLAG_STICKING;
    if (event->estimated_sag_mm > SAG_WARN_MM) event->flags |= HS_FLAG_SAG;
    if (event->closing_time_s > CLOSER_WARN_SECONDS) event->flags |= HS_FLAG_CLOSER_SLOW;
    event->health_score = analyzer_health_score(analyzer, event);
    analyzer->event_active = false;
}

void analyzer_init(hs_analyzer_t *analyzer, const hs_calibration_t *calibration)
{
    if (analyzer == NULL) return;
    memset(analyzer, 0, sizeof(*analyzer));
    analyzer->state = DOOR_UNKNOWN;
    analyzer->force_variance = 1.0f;
    analyzer->angle_closed_ema = 0.0f;
    analyzer->sag_baseline_deg = 0.0f;
    analyzer->calibrated = calibration != NULL && calibration->load_counts_per_n > 1.0f;
}

bool analyzer_update(hs_analyzer_t *analyzer, const hs_sample_t *sample, hs_event_t *completed)
{
    if (analyzer == NULL || sample == NULL) return false;
    door_state_t next = classify_state(sample, analyzer->state);
    if (next != analyzer->state) analyzer->last_transition_ms = sample->timestamp_ms;
    if (begins_event(analyzer->state, next)) start_event(analyzer, sample);
    if (analyzer->event_active) accumulate_event(analyzer, sample, next);
    if (next == DOOR_CLOSED && sample->load_valid) {
        float force_error = sample->force_n - analyzer->force_baseline;
        analyzer->force_baseline += 0.015f * force_error;
        analyzer->force_variance += 0.015f * (force_error * force_error - analyzer->force_variance);
    }
    if (next == DOOR_CLOSED && sample->magnet_valid) analyzer->angle_closed_ema += 0.005f * (sample->angle_deg - analyzer->angle_closed_ema);
    bool finished = ends_event(analyzer, sample, next);
    if (finished) {
        finish_event(analyzer, sample);
        if (completed != NULL) *completed = analyzer->active;
    }
    analyzer->previous = *sample;
    analyzer->state = next;
    analyzer->last_sample_ms = sample->timestamp_ms;
    return finished;
}

float analyzer_estimate_sag(const hs_analyzer_t *analyzer, float closed_angle)
{
    if (analyzer == NULL) return 0.0f;
    float drift = absolute(closed_angle - analyzer->sag_baseline_deg);
    return clamp(drift * 0.72f, 0.0f, 20.0f);
}

uint16_t analyzer_health_score(const hs_analyzer_t *analyzer, const hs_event_t *event)
{
    if (analyzer == NULL || event == NULL) return 0u;
    float score = 100.0f;
    float sigma = sqrtf(maximum(analyzer->force_variance, 0.1f));
    float excess = event->peak_open_force_n - (analyzer->force_baseline + 4.0f * sigma);
    if (excess > 0.0f) score -= clamp(excess * 0.7f, 0.0f, 30.0f);
    score -= clamp(event->estimated_sag_mm * 7.0f, 0.0f, 25.0f);
    if (event->flags & HS_FLAG_CLOSER_SLOW) score -= 12.0f;
    if (event->flags & HS_FLAG_SLAM) score -= 10.0f;
    if (event->flags & HS_FLAG_OBSTRUCTED) score -= 10.0f;
    if (event->flags & HS_FLAG_SENSOR_FAULT) score -= 20.0f;
    return (uint16_t)clamp(score, 0.0f, 100.0f);
}

void history_init(hs_history_t *history)
{
    if (history != NULL) memset(history, 0, sizeof(*history));
}

void history_append(hs_history_t *history, const hs_event_t *event)
{
    if (history == NULL || event == NULL) return;
    history->records[history->head] = *event;
    history->head = (uint16_t)((history->head + 1u) % HS_HISTORY_RECORDS);
    if (history->count < HS_HISTORY_RECORDS) ++history->count;
}

bool history_get(const hs_history_t *history, unsigned newest_index, hs_event_t *event)
{
    if (history == NULL || event == NULL || newest_index >= history->count) return false;
    unsigned position = (history->head + HS_HISTORY_RECORDS - 1u - newest_index) % HS_HISTORY_RECORDS;
    *event = history->records[position];
    return true;
}
