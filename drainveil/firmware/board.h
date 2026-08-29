/*
 * DrainVeil board definitions
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#ifndef DRAINVEIL_BOARD_H
#define DRAINVEIL_BOARD_H

#include <stddef.h>
#include <stdint.h>

#define DRAINVEIL_DEVICE_NAME "DrainVeil"
#define DRAINVEIL_AUTHOR "jayis1"
#define DRAINVEIL_FIRMWARE_VERSION "1.0.0"
#define DRAINVEIL_MAX_PACKET 1024u
#define DRAINVEIL_LOOP_COUNT 24u
#define DRAINVEIL_EVENT_CAPACITY 48u
#define DRAINVEIL_SNAPSHOT_CAPACITY 32u

typedef enum {
    INSTALL_PROFILE_KITCHEN_SINK = 0,
    INSTALL_PROFILE_FLOOR_DRAIN = 1,
    INSTALL_PROFILE_GREASE_INTERCEPTOR = 2
} install_profile_t;

typedef enum {
    ALERT_NONE = 0,
    ALERT_INFO = 1,
    ALERT_CAUTION = 2,
    ALERT_WARNING = 3,
    ALERT_CRITICAL = 4
} alert_level_t;

typedef struct {
    float ultrasonic_velocity;
    float reflection_strength;
    float turbulence_index;
    float fill_height_percent;
    float slug_probability;
    float flow_lpm;
    float drain_time_s;
    float bubble_factor;
} flow_state_t;

typedef struct {
    float line_pressure_kpa;
    float pulse_variance;
    float water_hammer_score;
    float trap_oscillation;
    float vibration_rms;
    float blockage_gradient;
    float branch_asymmetry;
} pressure_state_t;

typedef struct {
    float humidity_percent;
    float condensate_risk;
    float h2s_ppm;
    float voc_index;
    float biofilm_proxy;
    float grease_proxy;
    float corrosion_index;
} chemistry_state_t;

typedef struct {
    float pipe_temp_c;
    float ambient_temp_c;
    float freeze_margin_c;
    float thermal_recovery_s;
    float heat_leak_score;
    float cold_slug_index;
} thermal_state_t;

typedef struct {
    float battery_mv;
    float battery_percent;
    float solar_reclaim_mah;
    float estimated_days_left;
    float rail_noise_mv;
    uint8_t charger_online;
} power_state_t;

typedef struct {
    float clog_risk;
    float odor_risk;
    float freeze_risk;
    float maintenance_priority;
    float service_score;
    float confidence;
    float efficiency_penalty;
    alert_level_t alert;
    char reason[48];
} inference_state_t;

typedef struct {
    uint32_t minute_index;
    flow_state_t flow;
    pressure_state_t pressure;
    chemistry_state_t chemistry;
    thermal_state_t thermal;
    power_state_t power;
    inference_state_t inference;
} drain_snapshot_t;

typedef struct {
    uint32_t minute_index;
    alert_level_t level;
    char code[16];
    char detail[128];
} drain_event_t;

typedef struct {
    drain_event_t items[DRAINVEIL_EVENT_CAPACITY];
    size_t count;
} event_log_t;

typedef struct {
    drain_snapshot_t items[DRAINVEIL_SNAPSHOT_CAPACITY];
    size_t count;
} snapshot_log_t;

float dv_clampf(float value, float lo, float hi);
float dv_lerp(float a, float b, float t);
float dv_profile_bias(install_profile_t profile, float sink, float floor, float interceptor);
const char *dv_profile_name(install_profile_t profile);
const char *dv_alert_name(alert_level_t level);

#endif
