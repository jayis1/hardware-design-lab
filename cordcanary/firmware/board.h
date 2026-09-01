/*
 * CordCanary board definitions
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */

#ifndef CORDCANARY_BOARD_H
#define CORDCANARY_BOARD_H

#include <stdbool.h>
#include <stddef.h>

#define CC_DEVICE_NAME "CordCanary"
#define CC_AUTHOR "jayis1"
#define CC_HISTORY_DEPTH 24U
#define CC_EVENT_TEXT_MAX 96U
#define CC_UI_TEXT_MAX 96U
#define CC_FRAME_TEXT_MAX 256U
#define CC_STRIDE_SAMPLES 32U

typedef enum {
    CC_MODE_HOME = 0,
    CC_MODE_GARAGE = 1,
    CC_MODE_RV = 2,
    CC_MODE_WORKSHOP = 3
} cc_mode_t;

typedef enum {
    CC_STATE_NOMINAL = 0,
    CC_STATE_LOAD_WATCH = 1,
    CC_STATE_OUTLET_WEAR = 2,
    CC_STATE_CORD_FATIGUE = 3,
    CC_STATE_DAMP_LEAKAGE = 4,
    CC_STATE_ARC_SUSPECT = 5
} cc_state_t;

typedef struct {
    float ambient_c;
    float humidity_pct;
    float dew_margin_c;
    float shell_temp_c;
    float plug_face_temp_c;
    float cord_neck_temp_c;
    float hotspot_delta_c;
    float rise_rate_cpm;
} cc_thermal_frame_t;

typedef struct {
    float rms_current_a;
    float crest_factor;
    float hf_noise_score;
    float leakage_ma;
    float estimated_power_w;
    float transient_density;
    bool load_present;
} cc_current_frame_t;

typedef struct {
    float bend_radius_mm;
    float pull_force_n;
    float torsion_deg;
    float fatigue_index;
    bool clipped_securely;
} cc_strain_frame_t;

typedef struct {
    float vibration_rms_g;
    float orientation_deg;
    float wobble_score;
    unsigned drop_events;
    bool recently_moved;
} cc_motion_frame_t;

typedef struct {
    float battery_pct;
    float battery_v;
    float rail_v;
    float estimated_runtime_h;
    bool usb_present;
    bool charging;
} cc_power_frame_t;

typedef struct {
    cc_state_t state;
    float risk_score;
    float outlet_health_score;
    float confidence;
    bool urgent_unplug;
    char advisory[CC_UI_TEXT_MAX];
    char root_cause[CC_UI_TEXT_MAX];
} cc_inference_t;

typedef struct {
    unsigned tick;
    cc_state_t state;
    float risk_score;
    char note[CC_EVENT_TEXT_MAX];
} cc_log_entry_t;

typedef struct {
    cc_log_entry_t entries[CC_HISTORY_DEPTH];
    size_t count;
} cc_logger_t;

const char *cc_mode_name(cc_mode_t mode);
const char *cc_state_name(cc_state_t state);

#endif
