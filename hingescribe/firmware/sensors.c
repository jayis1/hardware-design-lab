/*
 * HingeScribe sensor acquisition and calibration
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */
#include "hingescribe.h"
#include <math.h>
#include <string.h>

static float last_angle;
static uint32_t last_time;
static float low_pass_force;
static bool initialized;

static uint16_t be16(const uint8_t bytes[2])
{
    return (uint16_t)(((uint16_t)bytes[0] << 8) | bytes[1]);
}

static float normalize_angle(float degrees)
{
    while (degrees < -180.0f) degrees += 360.0f;
    while (degrees >= 180.0f) degrees -= 360.0f;
    return degrees;
}

static float median5(float *values)
{
    for (unsigned i = 1; i < 5; ++i) {
        float candidate = values[i];
        unsigned j = i;
        while (j > 0 && values[j - 1] > candidate) {
            values[j] = values[j - 1];
            --j;
        }
        values[j] = candidate;
    }
    return values[2];
}

static hs_result_t read_angle(float *degrees, bool *valid)
{
    uint8_t status = 0;
    uint8_t raw[2] = {0};
    hs_result_t result = platform_i2c_read(AS5600_ADDRESS, AS5600_STATUS, &status, 1);
    if (result != HS_OK) return result;
    *valid = (status & 0x20u) != 0u && (status & 0x18u) == 0u;
    result = platform_i2c_read(AS5600_ADDRESS, AS5600_RAW_ANGLE, raw, 2);
    if (result != HS_OK) return result;
    uint16_t counts = (uint16_t)(be16(raw) & 0x0FFFu);
    *degrees = (float)counts * (360.0f / 4096.0f);
    return HS_OK;
}

static hs_result_t read_temperature(float *temperature)
{
    uint8_t raw[2] = {0};
    hs_result_t result = platform_i2c_read(TMP117_ADDRESS, TMP117_TEMP, raw, 2);
    if (result != HS_OK) return result;
    int16_t signed_raw = (int16_t)be16(raw);
    *temperature = (float)signed_raw * 0.0078125f;
    if (*temperature < -40.0f || *temperature > 125.0f) return HS_ERR_RANGE;
    return HS_OK;
}

static hs_result_t read_force(float *force, bool *valid, const hs_calibration_t *cal)
{
    float window[5];
    *valid = false;
    if (fabsf(cal->load_counts_per_n) < 1.0f) return HS_ERR_STATE;
    for (unsigned i = 0; i < 5; ++i) {
        int32_t counts = 0;
        hs_result_t result = platform_loadcell_read(&counts, 25u);
        if (result != HS_OK) return result;
        window[i] = ((float)counts - cal->load_zero_counts) / cal->load_counts_per_n;
    }
    float filtered = median5(window);
    low_pass_force += 0.35f * (filtered - low_pass_force);
    *force = low_pass_force;
    *valid = isfinite(*force) && fabsf(*force) < 250.0f;
    return *valid ? HS_OK : HS_ERR_RANGE;
}

void sensor_init(void)
{
    uint8_t config[2] = {0x02u, 0x20u};
    (void)platform_i2c_write(TMP117_ADDRESS, TMP117_CONFIG, config, 2);
    last_angle = 0.0f;
    last_time = platform_millis();
    low_pass_force = 0.0f;
    initialized = true;
}

hs_result_t sensor_read(hs_sample_t *sample, const hs_calibration_t *calibration)
{
    if (sample == NULL || calibration == NULL) return HS_ERR_ARGUMENT;
    if (!initialized) sensor_init();
    memset(sample, 0, sizeof(*sample));
    sample->timestamp_ms = platform_millis();
    float absolute_angle = 0.0f;
    hs_result_t angle_result = read_angle(&absolute_angle, &sample->magnet_valid);
    if (angle_result != HS_OK) sample->magnet_valid = false;
    sample->angle_deg = normalize_angle(absolute_angle - calibration->closed_zero_deg);
    uint32_t elapsed = sample->timestamp_ms - last_time;
    if (elapsed > 0u && elapsed < 1000u) {
        float delta = normalize_angle(sample->angle_deg - last_angle);
        sample->angular_velocity_dps = delta * (1000.0f / (float)elapsed);
    }
    hs_result_t load_result = read_force(&sample->force_n, &sample->load_valid, calibration);
    if (load_result != HS_OK) sample->load_valid = false;
    if (read_temperature(&sample->temperature_c) != HS_OK) sample->temperature_c = NAN;
    sample->battery_mv = platform_adc_mv(PIN_BATTERY_SENSE);
    sample->harvested_mj = platform_harvest_mj();
    last_angle = sample->angle_deg;
    last_time = sample->timestamp_ms;
    if (!sample->magnet_valid && !sample->load_valid) return HS_ERR_IO;
    return HS_OK;
}

hs_result_t sensor_tare(hs_calibration_t *calibration, unsigned samples)
{
    if (calibration == NULL || samples < 8u || samples > 128u) return HS_ERR_ARGUMENT;
    double angle_sum = 0.0;
    double load_sum = 0.0;
    unsigned good_angles = 0;
    unsigned good_loads = 0;
    for (unsigned i = 0; i < samples; ++i) {
        float angle = 0.0f;
        bool valid = false;
        if (read_angle(&angle, &valid) == HS_OK && valid) {
            angle_sum += angle;
            ++good_angles;
        }
        int32_t counts = 0;
        if (platform_loadcell_read(&counts, 50u) == HS_OK) {
            load_sum += counts;
            ++good_loads;
        }
        platform_delay_ms(10u);
    }
    if (good_angles < samples * 3u / 4u || good_loads < samples * 3u / 4u) return HS_ERR_IO;
    calibration->closed_zero_deg = (float)(angle_sum / good_angles);
    calibration->load_zero_counts = (float)(load_sum / good_loads);
    if (fabsf(calibration->load_counts_per_n) < 1.0f) calibration->load_counts_per_n = 8420.0f;
    if (calibration->sag_mm_per_deg <= 0.0f) calibration->sag_mm_per_deg = 0.72f;
    return calibration_save(calibration) ? HS_OK : HS_ERR_STORAGE;
}
