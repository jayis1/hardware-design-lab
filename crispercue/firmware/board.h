/*
 * CrisperCue board definitions
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#ifndef CRISPERCUE_BOARD_H
#define CRISPERCUE_BOARD_H

#include <stddef.h>
#include <stdint.h>

#define CRISPERCUE_DEVICE_NAME "CrisperCue"
#define CRISPERCUE_AUTHOR "jayis1"
#define CRISPERCUE_FIRMWARE_VERSION "1.0.0"
#define CRISPERCUE_LOOP_COUNT 36u
#define CRISPERCUE_EVENT_CAPACITY 48u
#define CRISPERCUE_SNAPSHOT_CAPACITY 64u

typedef enum {
    BIN_PROFILE_LEAFY_GREENS = 0,
    BIN_PROFILE_BERRIES = 1,
    BIN_PROFILE_CLIMACTERIC_FRUIT = 2
} bin_profile_t;

typedef enum {
    ALERT_NONE = 0,
    ALERT_INFO = 1,
    ALERT_CAUTION = 2,
    ALERT_WARNING = 3,
    ALERT_CRITICAL = 4
} alert_level_t;

typedef struct {
    float co2_ppm;
    float ethylene_ppm;
    float voc_index;
    float oxygen_percent;
    float humidity_rh;
    float purge_efficiency;
} gas_state_t;

typedef struct {
    float tray_mass_g;
    float daily_loss_g;
    float moisture_loss_percent;
    float usage_velocity;
    uint8_t refill_detected;
} mass_state_t;

typedef struct {
    float color_index;
    float chlorophyll_index;
    float bruise_probability;
    float mold_signature;
    float surface_gloss;
} optical_state_t;

typedef struct {
    float air_temp_c;
    float produce_temp_c;
    float dew_margin_c;
    float compressor_cycles;
    float drawer_open_minutes;
} thermal_state_t;

typedef struct {
    float battery_mv;
    float battery_percent;
    float current_ma;
    float solar_lux_recovery;
    uint8_t charging;
} power_state_t;

typedef struct {
    float freshness_score;
    float spoilage_risk;
    float recipe_urgency;
    float ventilation_demand;
    float shopper_value_left_usd;
    alert_level_t alert;
    char stage[32];
    char reason[96];
} inference_state_t;

typedef struct {
    uint32_t cycle_index;
    gas_state_t gas;
    mass_state_t mass;
    optical_state_t optical;
    thermal_state_t thermal;
    power_state_t power;
    inference_state_t inference;
} crisper_snapshot_t;

typedef struct {
    uint32_t cycle_index;
    alert_level_t level;
    char code[20];
    char detail[128];
} crisper_event_t;

typedef struct {
    crisper_event_t items[CRISPERCUE_EVENT_CAPACITY];
    size_t count;
} event_log_t;

typedef struct {
    crisper_snapshot_t items[CRISPERCUE_SNAPSHOT_CAPACITY];
    size_t count;
} snapshot_log_t;

#endif
