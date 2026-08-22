/*
 * Canopy Sentinel firmware board definitions
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */
#ifndef CANOPY_SENTINEL_BOARD_H
#define CANOPY_SENTINEL_BOARD_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CS_AUTHOR "jayis1"
#define CS_PRODUCT_NAME "Canopy Sentinel"
#define CS_FW_VERSION "1.0.0"
#define CS_BUILD_TARGET "host-sim"

#define CS_THERMAL_WIDTH 32
#define CS_THERMAL_HEIGHT 24
#define CS_THERMAL_PIXELS (CS_THERMAL_WIDTH * CS_THERMAL_HEIGHT)
#define CS_HISTORY_LENGTH 16
#define CS_LOG_CAPACITY 64
#define CS_PACKET_MAX 256
#define CS_ROW_NAME_MAX 24
#define CS_NOTES_MAX 96
#define CS_DEVICE_NAME_MAX 24
#define CS_BLE_MTU 244

typedef enum {
    CS_RISK_LOW = 0,
    CS_RISK_MODERATE = 1,
    CS_RISK_ELEVATED = 2,
    CS_RISK_CRITICAL = 3
} cs_risk_level_t;

typedef enum {
    CS_CROP_GRAPE = 0,
    CS_CROP_TOMATO = 1,
    CS_CROP_STRAWBERRY = 2,
    CS_CROP_CUCUMBER = 3,
    CS_CROP_HOPS = 4,
    CS_CROP_CUSTOM = 5
} cs_crop_profile_t;

typedef struct {
    float air_c;
    float rh_percent;
    float co2_ppm;
    float dew_point_c;
    float vpd_kpa;
} cs_climate_sample_t;

typedef struct {
    float pixels[CS_THERMAL_PIXELS];
    float min_c;
    float max_c;
    float mean_c;
    float variance;
    float leaf_c;
} cs_thermal_frame_t;

typedef struct {
    float raw_conductive;
    float raw_capacitive;
    float normalized_wetness;
    float persistence_score;
    bool clip_attached;
} cs_leaf_sample_t;

typedef struct {
    uint32_t pulse_count;
    float event_rate_hz;
    float fluorescence_index;
    float baseline;
    float peak;
} cs_spore_sample_t;

typedef struct {
    float airflow_score;
    float stagnation_score;
    float differential_pa;
} cs_airflow_sample_t;

typedef struct {
    uint32_t timestamp_s;
    uint32_t session_id;
    char row_id[CS_ROW_NAME_MAX];
    char notes[CS_NOTES_MAX];
    cs_crop_profile_t crop;
    cs_climate_sample_t climate;
    cs_thermal_frame_t thermal;
    cs_leaf_sample_t leaf;
    cs_spore_sample_t spore;
    cs_airflow_sample_t airflow;
    float dew_margin_c;
    float thermal_delta_c;
    float risk_score;
    cs_risk_level_t risk_level;
} cs_scan_result_t;

typedef struct {
    float dew_margin_weight;
    float wetness_weight;
    float spore_weight;
    float airflow_weight;
    float thermal_weight;
    float co2_weight;
    float critical_threshold;
    float elevated_threshold;
    float moderate_threshold;
} cs_risk_model_t;

typedef struct {
    char name[CS_DEVICE_NAME_MAX];
    char serial[16];
    uint32_t boot_count;
    uint32_t total_scans;
    cs_crop_profile_t active_crop;
    float wetness_calibration_gain;
    float spore_threshold;
    float airflow_calibration_gain;
} cs_device_state_t;

typedef struct {
    cs_scan_result_t entries[CS_LOG_CAPACITY];
    size_t count;
    size_t head;
} cs_log_t;

typedef struct {
    float battery_percent;
    bool charging;
    float bus_voltage_v;
    float current_ma;
} cs_power_state_t;

const char *cs_crop_name(cs_crop_profile_t crop);
const char *cs_risk_name(cs_risk_level_t risk);
cs_risk_model_t cs_default_model(cs_crop_profile_t crop);
float cs_clampf(float value, float min_value, float max_value);
float cs_lerp(float a, float b, float t);
float cs_rand_unit(void);
uint32_t cs_timestamp_now(void);

#endif
