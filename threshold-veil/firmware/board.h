/*
 * Threshold Veil board definitions
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */

#ifndef THRESHOLD_VEIL_BOARD_H
#define THRESHOLD_VEIL_BOARD_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define TV_FW_VERSION_MAJOR 1
#define TV_FW_VERSION_MINOR 0
#define TV_FW_VERSION_PATCH 0

#define TV_SAMPLE_HISTORY 32
#define TV_EVENT_LOG_SIZE 64
#define TV_NAME_MAX 32

typedef enum {
    TV_MODE_AUTO = 0,
    TV_MODE_QUIET,
    TV_MODE_SHELTER,
    TV_MODE_OPEN_FLOW
} tv_mode_t;

typedef enum {
    TV_LOUVER_SAMPLE = 0,
    TV_LOUVER_SEAL,
    TV_LOUVER_EQUALIZE
} tv_louver_t;

typedef enum {
    TV_STATE_CALM = 0,
    TV_STATE_ODOR_PUSH,
    TV_STATE_SMOKE_PUSH,
    TV_STATE_QUIET_HOURS,
    TV_STATE_PRESSURE_SURGE,
    TV_STATE_SHELTER,
    TV_STATE_DOOR_OPEN,
    TV_STATE_SERVICE
} tv_state_t;

typedef struct {
    float indoor_temp_c;
    float indoor_humidity_pct;
    float indoor_voc_index;
    float corridor_temp_c;
    float corridor_humidity_pct;
    float corridor_voc_index;
    float corridor_pm25_ugm3;
    float corridor_pm10_ugm3;
    float pressure_pa;
    float threshold_temp_c;
    bool door_closed;
    bool latch_aligned;
    bool quiet_hours;
    uint32_t tick;
} tv_env_frame_t;

typedef struct {
    float low_band_db;
    float mid_band_db;
    float high_band_db;
    float transient_score;
    bool impact_knock;
    bool rolling_noise;
} tv_acoustic_frame_t;

typedef struct {
    tv_louver_t louver;
    float blower_pwm;
    float gasket_pressure_kpa;
    float target_pressure_kpa;
    float seal_health_pct;
    bool relief_valve_open;
} tv_seal_frame_t;

typedef struct {
    float battery_voltage;
    float battery_pct;
    float current_ma;
    float est_hours_remaining;
    bool charging;
    bool thermal_derate;
} tv_power_frame_t;

typedef struct {
    tv_state_t state;
    float ingress_score;
    float smoke_score;
    float odor_score;
    float draft_score;
    float acoustic_score;
    float confidence;
    bool recommend_push_alert;
    char recommendation[96];
} tv_inference_t;

typedef struct {
    uint32_t tick;
    tv_state_t state;
    float ingress_score;
    float battery_pct;
    float corridor_pm25_ugm3;
    float pressure_pa;
    char note[96];
} tv_log_entry_t;

#endif
