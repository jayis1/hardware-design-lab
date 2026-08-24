/*
 * SplintSense board configuration
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#ifndef SPLINTSENSE_BOARD_H
#define SPLINTSENSE_BOARD_H

#include <stdint.h>
#include <stdbool.h>

#define SPLINTSENSE_AUTHOR "jayis1"
#define SPLINTSENSE_FIRMWARE_VERSION "1.0.0-sim"
#define SPLINTSENSE_DEVICE_NAME "SplintSense"

#define SPLINTSENSE_PRESSURE_ZONES 8
#define SPLINTSENSE_MOISTURE_ZONES 8
#define SPLINTSENSE_ALERT_LOG_CAPACITY 64
#define SPLINTSENSE_HISTORY_CAPACITY 96
#define SPLINTSENSE_EXPORT_TEXT_CAPACITY 4096

#define SPLINTSENSE_BATTERY_FULL_MV 4200.0f
#define SPLINTSENSE_BATTERY_EMPTY_MV 3320.0f
#define SPLINTSENSE_BATTERY_NOMINAL_MV 3840.0f
#define SPLINTSENSE_LOW_BATTERY_PERCENT 18.0f

#define SPLINTSENSE_SIMULATION_MINUTE_SECONDS 240.0f
#define SPLINTSENSE_LOOP_COUNT 36

typedef enum {
    SPLINT_PROFILE_WRIST = 0,
    SPLINT_PROFILE_ANKLE = 1
} splint_profile_t;

typedef enum {
    ALERT_NONE = 0,
    ALERT_INFO = 1,
    ALERT_CAUTION = 2,
    ALERT_WARNING = 3,
    ALERT_CRITICAL = 4
} alert_level_t;

typedef struct {
    float temperature_c;
    float humidity_rh;
    float voc_index;
    float acceleration_g;
    float orientation_drift;
    float impact_g;
    uint32_t step_count;
} env_frame_t;

typedef struct {
    float zones[SPLINTSENSE_PRESSURE_ZONES];
    float max_zone;
    float average_zone;
    float asymmetry;
    float dwell_risk;
} pressure_frame_t;

typedef struct {
    float zones[SPLINTSENSE_MOISTURE_ZONES];
    float average;
    float peak;
    float persistence_minutes;
    float drying_rate;
} moisture_frame_t;

typedef struct {
    float battery_percent;
    float battery_mv;
    bool charging;
    float estimated_hours_remaining;
} power_state_t;

typedef struct {
    env_frame_t env;
    pressure_frame_t pressure;
    moisture_frame_t moisture;
    power_state_t power;
    float recovery_stability_index;
    float fit_score;
    float odor_risk;
    float comfort_score;
    float compliance_score;
    alert_level_t alert;
    uint32_t minute_index;
} recovery_snapshot_t;

typedef struct {
    uint32_t minute_index;
    alert_level_t level;
    char code[24];
    char detail[96];
} alert_event_t;

#endif
