/* SPDX-License-Identifier: MIT
 * QuadraAlign sensor and estimator API
 * Author: jayis1
 */
#ifndef QUADRALIGN_SENSORS_H
#define QUADRALIGN_SENSORS_H
#include <stdbool.h>
#include <stdint.h>
#include "board.h"

typedef struct { float offset_counts; float counts_per_kg; float temperature_coefficient; } load_cal_t;
typedef struct { int32_t raw[QA_NUM_FEET]; float kg[QA_NUM_FEET]; bool valid; } load_frame_t;
typedef struct { float ax_g, ay_g, az_g; float pitch_deg, roll_deg; float vibration_rms_g; bool valid; } imu_frame_t;
typedef struct {
    float foot_kg[QA_NUM_FEET]; float total_kg; float imbalance_percent;
    float center_x_mm; float center_y_mm; float pitch_deg; float roll_deg;
    float vibration_rms_g; uint8_t recommended_foot; float recommended_turns;
    uint32_t sequence; bool stable; bool overloaded;
} qa_measurement_t;

typedef struct {
    load_cal_t cal[QA_NUM_FEET]; float footprint_width_mm; float footprint_depth_mm;
    float screw_pitch_mm; float warn_imbalance_percent; float warn_tilt_deg; uint32_t crc32;
} qa_config_t;

bool sensors_init(void);
bool sensors_read_load(load_frame_t *frame, const qa_config_t *config);
bool sensors_read_imu(imu_frame_t *frame);
void sensors_tare(qa_config_t *config, uint16_t samples);
void sensors_calibrate_channel(qa_config_t *config, uint8_t channel, int32_t zero, int32_t loaded, float reference_kg);
void estimator_init(void);
bool estimator_update(const load_frame_t *load, const imu_frame_t *imu, const qa_config_t *config, qa_measurement_t *out);
uint32_t qa_crc32(const void *data, uint32_t length);
bool config_load(qa_config_t *config);
bool config_save(qa_config_t *config);
void config_defaults(qa_config_t *config);
#endif
