/*
 * sash-sentinel/firmware/board.h
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */

#ifndef SASH_SENTINEL_BOARD_H
#define SASH_SENTINEL_BOARD_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define FW_AUTHOR "jayis1"
#define FW_PRODUCT_NAME "Sash Sentinel"
#define FW_VERSION "1.0.0"
#define FW_LOG_CAPACITY 64
#define FW_TELEMETRY_TEXT_CAPACITY 512
#define FW_WINDOW_ZONES 4
#define FW_SAMPLE_HISTORY 24

typedef enum {
    POWER_MODE_BOOT = 0,
    POWER_MODE_ACTIVE,
    POWER_MODE_LOW_POWER,
    POWER_MODE_CHARGING,
    POWER_MODE_SHIPPING
} power_mode_t;

typedef enum {
    ALERT_NONE = 0,
    ALERT_INFO,
    ALERT_WARNING,
    ALERT_CRITICAL
} alert_level_t;

typedef struct {
    float indoor_temp_c;
    float outdoor_est_temp_c;
    float indoor_humidity_pct;
    float cavity_humidity_pct;
    float cavity_pressure_pa;
    float dew_point_c;
    float sill_moisture_pct;
    float voc_index;
} env_snapshot_t;

typedef struct {
    float frame_temp_c[FW_WINDOW_ZONES];
    float glass_temp_c[FW_WINDOW_ZONES];
    float seal_temp_c[FW_WINDOW_ZONES];
    float thermal_gradient_c;
    float edge_cold_spot_c;
    bool frost_signature;
} thermal_snapshot_t;

typedef struct {
    float latch_force_n;
    float sash_offset_mm;
    float vibration_rms;
    float travel_cycles;
    bool latch_closed;
    bool tamper_event;
} latch_snapshot_t;

typedef struct {
    float leak_velocity_mps;
    float acoustic_leak_score;
    float pressure_pulse_pa;
    bool gust_event;
    bool rain_pattern;
} airflow_snapshot_t;

typedef struct {
    float battery_voltage_v;
    float battery_percent;
    float current_ma;
    bool usb_present;
    power_mode_t mode;
} power_snapshot_t;

typedef struct {
    uint32_t epoch_s;
    env_snapshot_t env;
    thermal_snapshot_t thermal;
    latch_snapshot_t latch;
    airflow_snapshot_t airflow;
    power_snapshot_t power;
} device_sample_t;

typedef struct {
    float condensation_risk;
    float infiltration_risk;
    float mold_risk;
    float latch_fault_risk;
    float rot_risk;
    float comfort_loss_risk;
    alert_level_t level;
    char summary[160];
    char action[192];
} risk_report_t;

typedef struct {
    uint32_t timestamp_s;
    alert_level_t level;
    char category[24];
    char message[160];
} log_entry_t;

typedef struct {
    device_sample_t samples[FW_SAMPLE_HISTORY];
    size_t count;
    size_t head;
} sample_history_t;

typedef struct {
    char ssid[32];
    char install_location[48];
    float expected_pressure_bias_pa;
    float acceptable_condensation_score;
    bool enable_buzzer;
    bool metric_units;
} device_config_t;

void board_init_config(device_config_t *config);
void board_history_init(sample_history_t *history);
void board_history_push(sample_history_t *history, const device_sample_t *sample);
const device_sample_t *board_history_latest(const sample_history_t *history);
float board_clampf(float value, float min_value, float max_value);
float board_lerpf(float a, float b, float t);
float board_average(const float *values, size_t count);
float board_max(const float *values, size_t count);
float board_min(const float *values, size_t count);

#endif
