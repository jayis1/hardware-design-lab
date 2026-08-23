/*
 * board.h — StudGuard board definitions
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#ifndef STUDGUARD_BOARD_H
#define STUDGUARD_BOARD_H

#include <stdint.h>

#define STUDGUARD_FIRMWARE_VERSION   "1.0.0"
#define STUDGUARD_BUILD_AUTHOR       "jayis1"

#define BOARD_NAME                   "StudGuard Tile RevA"
#define MCU_NAME                     "nRF5340"
#define UWB_NAME                     "DW3110"
#define HUMIDITY_SENSOR_NAME         "SHT41"
#define ACCEL_NAME                   "LIS2DW12"

#define AUDIO_SAMPLE_RATE_HZ         16000u
#define AUDIO_FRAME_SAMPLES          512u
#define CAP_SEGMENT_COUNT            4u
#define MAX_MESH_NODES               8u
#define LOG_CAPACITY                 128u
#define NOMINAL_INTERVAL_MS          900000u
#define DIAGNOSTIC_INTERVAL_MS       15000u
#define CRITICAL_INTERVAL_MS         5000u

#define LEAK_ACTIVITY_ALERT          0.68f
#define WETNESS_SPREAD_ALERT         0.55f
#define CONFIDENCE_ALERT             0.60f

typedef enum {
    MODE_BASELINE = 0,
    MODE_MONITORING,
    MODE_DIAGNOSTIC,
    MODE_REPAIR_VERIFICATION,
    MODE_SLEEPY
} sg_mode_t;

typedef enum {
    EVENT_NONE = 0,
    EVENT_CONDENSATION,
    EVENT_INTERMITTENT_LEAK,
    EVENT_ACTIVE_PRESSURE_LEAK,
    EVENT_POST_REPAIR_DRYING,
    EVENT_SENSOR_FAULT
} sg_event_t;

typedef struct {
    float humidity_rh;
    float temperature_c;
    float dew_point_c;
    float wall_temperature_c;
    float cap_segments[CAP_SEGMENT_COUNT];
    float cap_vector_x;
    float cap_vector_y;
    float cap_mean;
    float cap_delta;
    float acoustic_energy;
    float acoustic_decay_ms;
    float spectral_centroid_hz;
    float phase_stability;
    float damping_ratio;
    float peer_attenuation;
    float leak_activity;
    float wetness_spread;
    float confidence;
    float origin_band;
    sg_event_t event;
    uint8_t peer_count;
} sg_measurement_t;

typedef struct {
    uint8_t id;
    float x;
    float y;
    float z;
    float leak_activity;
    float wetness_spread;
    float confidence;
    float attenuation;
    float cap_mean;
} sg_peer_snapshot_t;

typedef struct {
    uint8_t node_id;
    sg_mode_t mode;
    float battery_percent;
    uint32_t interval_ms;
    uint32_t uptime_s;
    uint8_t mounted;
    uint8_t mesh_enabled;
} sg_device_status_t;

#endif
