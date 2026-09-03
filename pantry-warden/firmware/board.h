/*
 * Pantry Warden board definitions
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */

#ifndef PANTRY_WARDEN_BOARD_H
#define PANTRY_WARDEN_BOARD_H

#include <stddef.h>

#define PW_SAMPLE_COUNT            48U
#define PW_LOG_DEPTH               24U
#define PW_AUDIO_WINDOW_MS         120U
#define PW_GAS_WINDOW_MS           400U
#define PW_RESTOCK_MASS_DELTA_KG   0.45f
#define PW_SPOILAGE_VOC_TRIGGER    26.0f
#define PW_PEST_WINGBEAT_TRIGGER   48.0f
#define PW_CONDENSE_TRIGGER_PCT    74.0f
#define PW_LOW_BATTERY_PCT         18.0f

typedef enum {
    PW_MODE_AUTO = 0,
    PW_MODE_QUIET = 1,
    PW_MODE_NIGHT_SWEEP = 2,
    PW_MODE_CLEANOUT = 3
} pw_mode_t;

typedef enum {
    PW_STATE_STABLE = 0,
    PW_STATE_RESTOCKED = 1,
    PW_STATE_CONDENSATION_WATCH = 2,
    PW_STATE_SPOILAGE_SUSPECT = 3,
    PW_STATE_PEST_WATCH = 4,
    PW_STATE_CRITICAL_INTERVENE = 5
} pw_state_t;

typedef struct {
    unsigned tick;
    float sample_minutes;
    float shelf_depth_mm;
    unsigned quiet_hours_start;
    unsigned quiet_hours_end;
    float target_humidity_pct;
} pw_profile_t;

typedef struct {
    float temp_c;
    float humidity_pct;
    float co2_ppm;
    float voc_index;
    float ethanol_ppm;
    float stale_air_index;
    float fan_duty_pct;
    float dew_margin_c;
} pw_gas_frame_t;

typedef struct {
    float total_mass_kg;
    float left_mass_kg;
    float right_mass_kg;
    float front_gap_mm;
    float optical_freshness_pct;
    float moisture_strip_pct;
    float package_tilt_deg;
    float disturbance_score;
} pw_shelf_frame_t;

typedef struct {
    float wingbeat_score;
    float chew_score;
    float structure_energy;
    float airborne_energy;
    unsigned transient_count;
} pw_acoustic_frame_t;

typedef struct {
    float battery_pct;
    float bus_voltage_v;
    float current_ma;
    float estimated_hours_left;
    int charging;
} pw_power_frame_t;

typedef struct {
    pw_state_t state;
    float shelf_health_score;
    float spoilage_confidence;
    float pest_confidence;
    float condensation_risk;
    float restock_confidence;
    const char *action;
} pw_inference_t;

#endif
