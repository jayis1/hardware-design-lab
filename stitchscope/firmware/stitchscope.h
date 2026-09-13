/* StitchScope shared firmware API
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#ifndef STITCHSCOPE_H
#define STITCHSCOPE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "board.h"

typedef enum {
    SENSOR_OK = 0,
    SENSOR_NOT_READY,
    SENSOR_BAD_ID,
    SENSOR_BUS_ERROR,
    SENSOR_RANGE_ERROR
} sensor_status_t;

typedef enum {
    CLASS_OK = 0,
    CLASS_SKIP_SUSPECT,
    CLASS_TOP_THREAD_BREAK,
    CLASS_BOBBIN_DEPLETION,
    CLASS_FEED_STALL,
    CLASS_HARD_STRIKE,
    CLASS_BIRDNEST_GROWTH,
    CLASS_UNTRAINED
} stitch_class_t;

typedef struct {
    uint32_t timestamp_us;
    int32_t force_un;
    int16_t accel_mg[3];
    int16_t mag_ut[3];
    uint16_t tof_mm[9];
    uint8_t tof_confidence[9];
    int16_t temperature_centi_c;
    uint16_t humidity_centi_pct;
    bool hall;
} sensor_frame_t;

typedef struct {
    uint32_t cycle_index;
    uint32_t start_us;
    uint32_t duration_us;
    int32_t force_peak_un;
    int32_t force_integral_un_bins;
    int32_t force_slope_un_per_bin;
    uint32_t force_hysteresis;
    uint32_t impulse_low;
    uint32_t impulse_mid;
    uint32_t impulse_high;
    uint16_t impulse_phase_q15;
    int32_t feed_um;
    uint16_t feed_confidence_q15;
    uint16_t phase_jitter_q15;
    uint16_t anomaly_q12;
    uint16_t quality_q12;
    stitch_class_t classification;
    uint16_t capability_mask;
} stitch_features_t;

typedef struct {
    int32_t mean[10];
    uint32_t deviation[10];
    uint16_t expected_feed_um;
    uint16_t sensitivity_q12;
    uint32_t sample_count;
    bool trained;
} recipe_t;

typedef struct {
    int32_t force_bins[PHASE_BIN_COUNT];
    int32_t accel_bins[PHASE_BIN_COUNT];
    uint16_t bin_counts[PHASE_BIN_COUNT];
    int32_t first_tof_centroid;
    int32_t last_tof_centroid;
    uint32_t tof_confidence_sum;
    uint16_t tof_frames;
    uint32_t start_us;
    uint32_t previous_cycle_us;
    uint32_t cycle_index;
    uint16_t max_phase_error_q15;
    bool active;
} cycle_accumulator_t;

typedef struct {
    uint8_t data[BLE_MAX_PAYLOAD];
    uint16_t length;
    uint16_t expected;
    uint8_t state;
} protocol_parser_t;

void sensors_init(void);
sensor_status_t sensors_sample(sensor_frame_t *frame, uint32_t now_us);
uint16_t sensors_capability_mask(void);
uint16_t sensors_battery_mv(void);

void signal_init(cycle_accumulator_t *acc);
bool signal_ingest(cycle_accumulator_t *acc, const sensor_frame_t *frame,
                   bool cycle_boundary, stitch_features_t *completed);
void signal_finalize(cycle_accumulator_t *acc, stitch_features_t *out, uint32_t end_us);

void classifier_init(void);
void classifier_set_recipe(const recipe_t *recipe);
stitch_class_t classifier_evaluate(stitch_features_t *features);
void classifier_learn(recipe_t *recipe, const stitch_features_t *features);
const char *classifier_name(stitch_class_t value);

void protocol_init(protocol_parser_t *parser);
bool protocol_push(protocol_parser_t *parser, uint8_t byte, uint8_t *type,
                   uint16_t *sequence, const uint8_t **payload, uint16_t *length);
size_t protocol_encode(uint8_t type, uint16_t sequence, const uint8_t *payload,
                       uint16_t length, uint8_t *output, size_t capacity);
uint16_t protocol_crc16(const uint8_t *data, size_t length);

#endif
