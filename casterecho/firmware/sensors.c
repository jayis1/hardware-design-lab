/*
 * CasterEcho inertial, acoustic, Hall, and thermal acquisition.
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */
#include "casterecho.h"
#include <math.h>
#include <string.h>

static ce_sample_t previous_sample;
static bool have_previous;
static float wheel_circumference_mm = CE_WHEEL_DIAMETER_MM * 3.14159265359f;

static int16_t unpack_i16(const uint8_t *bytes) {
    return (int16_t)((uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8));
}

static float clampf(float value, float low, float high) {
    if (value < low) return low;
    if (value > high) return high;
    return value;
}

ce_result_t sensors_init(void) {
    uint8_t identity = 0u;
    uint8_t ctrl1 = 0x77u;
    uint8_t ctrl4 = 0x88u;
    if (platform_i2c_read(LIS3DH_ADDRESS, LIS3DH_WHO_AM_I, &identity, 1u) != CE_OK) return CE_ERR_IO;
    if (identity != 0x33u) return CE_ERR_STATE;
    if (platform_i2c_write(LIS3DH_ADDRESS, LIS3DH_CTRL1, &ctrl1, 1u) != CE_OK) return CE_ERR_IO;
    if (platform_i2c_write(LIS3DH_ADDRESS, LIS3DH_CTRL4, &ctrl4, 1u) != CE_OK) return CE_ERR_IO;
    sensors_reset();
    return CE_OK;
}

void sensors_reset(void) {
    memset(&previous_sample, 0, sizeof(previous_sample));
    have_previous = false;
}

float sensors_speed_from_period(uint32_t period_us, float circumference_mm) {
    if (period_us == 0u || circumference_mm <= 0.0f) return 0.0f;
    return (circumference_mm / 1000.0f) / ((float)period_us / 1000000.0f);
}

ce_motion_t sensors_classify_motion(const ce_sample_t *sample, const ce_sample_t *previous) {
    if (sample == NULL || !sample->imu_valid) return MOTION_UNKNOWN;
    const float magnitude = sqrtf(sample->accel_x_g*sample->accel_x_g + sample->accel_y_g*sample->accel_y_g + sample->accel_z_g*sample->accel_z_g);
    if (magnitude > 2.8f || sample->piezo_g > 2.5f) return MOTION_SHOCK;
    if (!sample->hall_valid || sample->speed_mps < CE_MIN_SPEED_MPS) return MOTION_STOPPED;
    if (previous == NULL || previous->speed_mps < CE_MIN_SPEED_MPS) return MOTION_STARTING;
    if (sample->speed_mps < previous->speed_mps * 0.92f) return MOTION_COASTING;
    return MOTION_ROLLING;
}

ce_result_t sensors_read(ce_sample_t *sample) {
    uint8_t raw[6];
    uint32_t period_us = 0u;
    int8_t direction = 0;
    if (sample == NULL) return CE_ERR_ARGUMENT;
    memset(sample, 0, sizeof(*sample));
    sample->timestamp_ms = platform_millis();
    if (platform_i2c_read(LIS3DH_ADDRESS, (uint8_t)(LIS3DH_OUT_X_L | 0x80u), raw, sizeof(raw)) == CE_OK) {
        sample->accel_x_g = (float)unpack_i16(&raw[0]) / 16384.0f;
        sample->accel_y_g = (float)unpack_i16(&raw[2]) / 16384.0f;
        sample->accel_z_g = (float)unpack_i16(&raw[4]) / 16384.0f;
        sample->imu_valid = isfinite(sample->accel_x_g) && isfinite(sample->accel_y_g) && isfinite(sample->accel_z_g);
    }
    const uint16_t piezo_mv = platform_adc_mv(PIN_PIEZO_ADC);
    sample->piezo_g = fabsf(((float)piezo_mv - 1650.0f) / 330.0f);
    const uint16_t temp_mv = platform_adc_mv(PIN_TEMP_ADC);
    sample->bearing_temp_c = ((float)temp_mv - 500.0f) / 10.0f;
    sample->generator_mv = (float)platform_adc_mv(PIN_GENERATOR_ADC) * 2.0f;
    if (platform_hall_edges(&period_us, &direction)) {
        sample->speed_mps = clampf(sensors_speed_from_period(period_us, wheel_circumference_mm), 0.0f, CE_MAX_SPEED_MPS);
        sample->wheel_rpm = sample->speed_mps * 60000.0f / wheel_circumference_mm;
        sample->hall_direction = direction;
        sample->hall_valid = true;
    }
    if (!sample->imu_valid && !sample->hall_valid) return CE_ERR_IO;
    previous_sample = *sample;
    have_previous = true;
    (void)previous_sample;
    (void)have_previous;
    return CE_OK;
}
