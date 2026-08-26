/*
 * PipeWhisper board definitions
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#ifndef PIPEWHISPER_BOARD_H
#define PIPEWHISPER_BOARD_H

#include <stddef.h>
#include <stdint.h>

#define PIPEWHISPER_AUTHOR "jayis1"
#define PIPEWHISPER_DEVICE_NAME "PipeWhisper"
#define PIPEWHISPER_FIRMWARE_VERSION "1.0.0-sim"
#define PIPEWHISPER_LOOP_COUNT 30u
#define PIPEWHISPER_EVENT_CAPACITY 64u
#define PIPEWHISPER_HISTORY_CAPACITY 64u

typedef enum {
    PIPE_PROFILE_KITCHEN_COLD = 0,
    PIPE_PROFILE_LAUNDRY_HOT = 1,
    PIPE_PROFILE_UTILITY_MIXED = 2
} pipe_profile_t;

typedef enum {
    ALERT_NONE = 0,
    ALERT_INFO = 1,
    ALERT_CAUTION = 2,
    ALERT_WARNING = 3,
    ALERT_CRITICAL = 4
} alert_level_t;

typedef struct {
    float acoustic_rms;
    float dominant_hz;
    float impulsiveness;
    float texture;
    float chatter_index;
    float drip_period_s;
} acoustic_frame_t;

typedef struct {
    float draw_estimate_lpm;
    float drip_confidence;
    float steady_flow_confidence;
    float fixture_similarity_sink;
    float fixture_similarity_washer;
    float fixture_similarity_icemaker;
    float signature_drift;
} flow_frame_t;

typedef struct {
    float hammer_score;
    float impulse_count;
    float ring_decay_ms;
    float burst_risk;
    float strain_peak;
} pressure_frame_t;

typedef struct {
    float surface_temp_c;
    float ambient_temp_c;
    float humidity_rh;
    float dew_risk;
    float freeze_slope_cph;
    float condensation_risk;
} environment_frame_t;

typedef struct {
    float battery_percent;
    float battery_mv;
    uint8_t charging;
    uint8_t low_power_hint;
} power_frame_t;

typedef struct {
    float leak_confidence;
    float freeze_risk;
    float install_quality;
    float maintenance_priority;
    float appliance_drift;
    float health_index;
    alert_level_t alert;
} inference_frame_t;

typedef struct {
    uint32_t minute_index;
    acoustic_frame_t acoustic;
    flow_frame_t flow;
    pressure_frame_t pressure;
    environment_frame_t environment;
    power_frame_t power;
    inference_frame_t inference;
} pipe_snapshot_t;

typedef struct {
    uint32_t minute_index;
    alert_level_t level;
    char code[20];
    char detail[120];
} pipe_event_t;

typedef struct {
    pipe_event_t events[PIPEWHISPER_EVENT_CAPACITY];
    size_t count;
} event_log_t;

typedef struct {
    pipe_snapshot_t history[PIPEWHISPER_HISTORY_CAPACITY];
    size_t count;
} snapshot_log_t;

#endif
