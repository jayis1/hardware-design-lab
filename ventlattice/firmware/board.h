/*
 * VentLattice board definitions
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#ifndef VENTLATTICE_BOARD_H
#define VENTLATTICE_BOARD_H

#include <stddef.h>
#include <stdint.h>

#define VENTLATTICE_DEVICE_NAME "VentLattice"
#define VENTLATTICE_AUTHOR "jayis1"
#define VENTLATTICE_FIRMWARE_VERSION "1.0.0"
#define VENTLATTICE_LOOP_COUNT 24u
#define VENTLATTICE_EVENT_CAPACITY 32u
#define VENTLATTICE_SNAPSHOT_CAPACITY 48u

typedef enum {
    ALERT_NONE = 0,
    ALERT_INFO = 1,
    ALERT_CAUTION = 2,
    ALERT_WARNING = 3,
    ALERT_CRITICAL = 4
} alert_level_t;

typedef enum {
    ROOM_PROFILE_HOME_OFFICE = 0,
    ROOM_PROFILE_NURSERY = 1,
    ROOM_PROFILE_CLASSROOM = 2
} room_profile_t;

typedef struct {
    float airflow_cfm;
    float velocity_mps;
    float blockage_index;
    float delivery_stability;
    float vent_open_percent;
    float nozzle_delta_pa;
    uint8_t hvac_call_active;
} airflow_state_t;

typedef struct {
    float ripple_pa;
    float blower_signature;
    float filter_load_index;
    float branch_restriction;
    float turbulence_index;
} pressure_state_t;

typedef struct {
    float supply_temp_c;
    float room_temp_c;
    float humidity_rh;
    float voc_index;
    float dew_margin_c;
    float light_lux;
    float thermal_need;
} environment_state_t;

typedef struct {
    float presence_confidence;
    float dwell_hours;
    float occupied_alignment;
    uint8_t occupied_now;
} occupancy_state_t;

typedef struct {
    float battery_mv;
    float battery_percent;
    float current_ma;
    float runtime_hours_est;
    uint8_t charging;
} power_state_t;

typedef struct {
    float service_score;
    float comfort_waste;
    float stale_air_risk;
    float maintenance_priority;
    float condensation_risk;
    float install_quality;
    alert_level_t alert;
    char reason[64];
} inference_state_t;

typedef struct {
    uint32_t hour_index;
    airflow_state_t airflow;
    pressure_state_t pressure;
    environment_state_t environment;
    occupancy_state_t occupancy;
    power_state_t power;
    inference_state_t inference;
} vent_snapshot_t;

typedef struct {
    uint32_t hour_index;
    alert_level_t level;
    char code[20];
    char detail[128];
} vent_event_t;

typedef struct {
    vent_event_t items[VENTLATTICE_EVENT_CAPACITY];
    size_t count;
} event_log_t;

typedef struct {
    vent_snapshot_t items[VENTLATTICE_SNAPSHOT_CAPACITY];
    size_t count;
} snapshot_log_t;

#endif
