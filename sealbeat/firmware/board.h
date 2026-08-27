/*
 * SealBeat board definitions
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#ifndef SEALBEAT_BOARD_H
#define SEALBEAT_BOARD_H

#include <stddef.h>
#include <stdint.h>

#define SEALBEAT_DEVICE_NAME "SealBeat"
#define SEALBEAT_AUTHOR "jayis1"
#define SEALBEAT_FIRMWARE_VERSION "0.1.0"
#define SEALBEAT_LOOP_COUNT 48u
#define SEALBEAT_MAX_EVENTS 32u
#define SEALBEAT_MAX_SNAPSHOTS 64u
#define SEALBEAT_MAX_PACKET 256u

typedef enum {
    APPLIANCE_PROFILE_RESIDENTIAL_FRIDGE = 0,
    APPLIANCE_PROFILE_UPRIGHT_FREEZER,
    APPLIANCE_PROFILE_PHARMACY_COOLER
} appliance_profile_t;

typedef enum {
    ALERT_NONE = 0,
    ALERT_INFO,
    ALERT_CAUTION,
    ALERT_WARNING,
    ALERT_CRITICAL
} alert_level_t;

typedef struct {
    float latch_sharpness;
    float compressor_harmonic;
    float frame_vibration;
    float slam_energy;
    float noise_floor;
    float compressor_burden;
    unsigned shock_events;
} acoustic_state_t;

typedef struct {
    float door_angle_deg;
    float dwell_open_seconds;
    float bounce_count;
    float close_velocity;
    float hinge_skew;
    float tilt_drift_deg;
    unsigned cycle_count;
    unsigned night_cycles;
} door_state_t;

typedef struct {
    float top_edge_score;
    float latch_edge_score;
    float bottom_edge_score;
    float hinge_edge_score;
    float compression_uniformity;
    float magnetic_pull;
    float final_gap_mm;
    float leak_vector;
    float closure_confidence;
} seal_state_t;

typedef struct {
    float ambient_temp_c;
    float humidity_rh;
    float edge_temp_c;
    float compartment_temp_c;
    float warm_rebound_c;
    float recovery_tau_s;
    float frost_risk;
    float safety_margin;
} thermal_state_t;

typedef struct {
    float battery_mv;
    float battery_percent;
    float charge_current_ma;
    float average_current_ma;
    float estimated_days_left;
    unsigned charging;
    unsigned low_power_mode;
} power_state_t;

typedef struct {
    float seal_integrity;
    float safety_confidence;
    float hinge_wear;
    float maintenance_priority;
    float energy_penalty;
    float closure_quality;
    float service_score;
    alert_level_t alert;
} inference_state_t;

typedef struct {
    uint32_t minute_index;
    acoustic_state_t acoustic;
    door_state_t door;
    seal_state_t seal;
    thermal_state_t thermal;
    power_state_t power;
    inference_state_t inference;
} appliance_snapshot_t;

typedef struct {
    uint32_t minute_index;
    alert_level_t level;
    char category[24];
    char detail[128];
} event_record_t;

typedef struct {
    event_record_t items[SEALBEAT_MAX_EVENTS];
    size_t count;
} event_log_t;

typedef struct {
    appliance_snapshot_t items[SEALBEAT_MAX_SNAPSHOTS];
    size_t count;
} snapshot_log_t;

float sb_clampf(float value, float lo, float hi);
float sb_lerp(float a, float b, float t);
float sb_profile_bias(appliance_profile_t profile, float residential, float freezer, float pharmacy);
const char *sb_profile_name(appliance_profile_t profile);
const char *sb_alert_name(alert_level_t level);

#endif
