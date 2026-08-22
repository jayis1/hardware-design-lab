/*
 * Canopy Sentinel firmware simulation
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */
#include "board.h"
#include "registers.h"
#include "drivers/ble.h"
#include "drivers/climate.h"
#include "drivers/display.h"
#include "drivers/leaf.h"
#include "drivers/power.h"
#include "drivers/spore.h"
#include "drivers/storage.h"
#include "drivers/thermal.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static uint32_t g_rng = 0xC0FFEE11u;

float cs_clampf(float value, float min_value, float max_value) {
    if (value < min_value) return min_value;
    if (value > max_value) return max_value;
    return value;
}

float cs_lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

float cs_rand_unit(void) {
    g_rng = 1664525u * g_rng + 1013904223u;
    return ((float)((g_rng >> 8) & 0xFFFFu) / 65535.0f) - 0.5f;
}

uint32_t cs_timestamp_now(void) {
    return (uint32_t)time(NULL);
}

const char *cs_crop_name(cs_crop_profile_t crop) {
    switch (crop) {
        case CS_CROP_GRAPE: return "grape";
        case CS_CROP_TOMATO: return "tomato";
        case CS_CROP_STRAWBERRY: return "strawberry";
        case CS_CROP_CUCUMBER: return "cucumber";
        case CS_CROP_HOPS: return "hops";
        case CS_CROP_CUSTOM: return "custom";
        default: return "unknown";
    }
}

const char *cs_risk_name(cs_risk_level_t risk) {
    switch (risk) {
        case CS_RISK_LOW: return "low";
        case CS_RISK_MODERATE: return "moderate";
        case CS_RISK_ELEVATED: return "elevated";
        case CS_RISK_CRITICAL: return "critical";
        default: return "invalid";
    }
}

cs_risk_model_t cs_default_model(cs_crop_profile_t crop) {
    cs_risk_model_t model = {
        .dew_margin_weight = 0.30f,
        .wetness_weight = 0.22f,
        .spore_weight = 0.18f,
        .airflow_weight = 0.14f,
        .thermal_weight = 0.10f,
        .co2_weight = 0.06f,
        .critical_threshold = 78.0f,
        .elevated_threshold = 56.0f,
        .moderate_threshold = 34.0f,
    };

    switch (crop) {
        case CS_CROP_GRAPE:
            model.wetness_weight = 0.25f;
            model.spore_weight = 0.20f;
            break;
        case CS_CROP_TOMATO:
            model.airflow_weight = 0.18f;
            model.co2_weight = 0.08f;
            break;
        case CS_CROP_STRAWBERRY:
            model.thermal_weight = 0.14f;
            model.critical_threshold = 74.0f;
            break;
        case CS_CROP_CUCUMBER:
            model.dew_margin_weight = 0.34f;
            break;
        case CS_CROP_HOPS:
            model.spore_weight = 0.22f;
            model.moderate_threshold = 30.0f;
            break;
        case CS_CROP_CUSTOM:
        default:
            break;
    }
    return model;
}

static cs_risk_level_t classify_risk(float score, const cs_risk_model_t *model) {
    if (score >= model->critical_threshold) return CS_RISK_CRITICAL;
    if (score >= model->elevated_threshold) return CS_RISK_ELEVATED;
    if (score >= model->moderate_threshold) return CS_RISK_MODERATE;
    return CS_RISK_LOW;
}

static cs_airflow_sample_t derive_airflow(uint32_t tick, cs_crop_profile_t crop) {
    cs_airflow_sample_t sample;
    float crop_bias = (crop == CS_CROP_TOMATO || crop == CS_CROP_CUCUMBER) ? -0.08f : 0.0f;
    sample.differential_pa = 5.2f + 2.0f * sinf(tick * 0.29f) + crop_bias * 5.0f + cs_rand_unit() * 0.5f;
    sample.airflow_score = cs_clampf(sample.differential_pa / 10.0f, 0.0f, 1.0f);
    sample.stagnation_score = cs_clampf((1.0f - sample.airflow_score) * 100.0f, 0.0f, 100.0f);
    return sample;
}

static float compute_dew_margin(const cs_climate_sample_t *climate, const cs_thermal_frame_t *thermal) {
    return thermal->leaf_c - climate->dew_point_c;
}

static float compute_thermal_delta(const cs_climate_sample_t *climate, const cs_thermal_frame_t *thermal) {
    return climate->air_c - thermal->leaf_c;
}

static float score_dew_margin(float dew_margin_c) {
    if (dew_margin_c >= 3.0f) return 0.0f;
    if (dew_margin_c >= 0.0f) return (3.0f - dew_margin_c) / 3.0f * 55.0f;
    return cs_clampf(55.0f + (-dew_margin_c * 18.0f), 0.0f, 100.0f);
}

static float score_co2(float co2_ppm) {
    if (co2_ppm < 500.0f) return 10.0f;
    if (co2_ppm > 900.0f) return 75.0f;
    return 10.0f + ((co2_ppm - 500.0f) / 400.0f) * 65.0f;
}

static float compute_risk_score(const cs_scan_result_t *result, const cs_risk_model_t *model) {
    float dew_score = score_dew_margin(result->dew_margin_c);
    float wet_score = result->leaf.persistence_score;
    float spore_score = result->spore.fluorescence_index;
    float airflow_score = result->airflow.stagnation_score;
    float thermal_score = cs_clampf(result->thermal.variance * 120.0f, 0.0f, 100.0f);
    float co2_score = score_co2(result->climate.co2_ppm);

    return dew_score * model->dew_margin_weight +
           wet_score * model->wetness_weight +
           spore_score * model->spore_weight +
           airflow_score * model->airflow_weight +
           thermal_score * model->thermal_weight +
           co2_score * model->co2_weight;
}

static void init_device(cs_device_state_t *device) {
    memset(device, 0, sizeof(*device));
    snprintf(device->name, sizeof(device->name), "%s", "CanopySentinel");
    snprintf(device->serial, sizeof(device->serial), "%s", "CS-260822-A1");
    device->boot_count = 1u;
    device->total_scans = 0u;
    device->active_crop = CS_CROP_GRAPE;
    device->wetness_calibration_gain = 1.02f;
    device->spore_threshold = 0.32f;
    device->airflow_calibration_gain = 1.0f;
}

static void annotate_row(char *row_id, size_t size, cs_crop_profile_t crop, uint32_t index) {
    const char *prefix = "ROW";
    switch (crop) {
        case CS_CROP_GRAPE: prefix = "GV"; break;
        case CS_CROP_TOMATO: prefix = "TM"; break;
        case CS_CROP_STRAWBERRY: prefix = "SB"; break;
        case CS_CROP_CUCUMBER: prefix = "CU"; break;
        case CS_CROP_HOPS: prefix = "HP"; break;
        case CS_CROP_CUSTOM: prefix = "CX"; break;
        default: break;
    }
    snprintf(row_id, size, "%s-%02u", prefix, index + 1u);
}

static cs_scan_result_t run_scan(cs_device_state_t *device, uint32_t tick, uint32_t session_id) {
    cs_scan_result_t result;
    memset(&result, 0, sizeof(result));

    result.timestamp_s = cs_timestamp_now();
    result.session_id = session_id;
    result.crop = device->active_crop;
    annotate_row(result.row_id, sizeof(result.row_id), result.crop, tick);
    snprintf(result.notes, sizeof(result.notes), "%s scan by %s", cs_crop_name(result.crop), CS_AUTHOR);

    result.climate = climate_sample(tick, result.crop);
    result.thermal = thermal_capture(tick, result.crop, result.climate.air_c);
    result.dew_margin_c = compute_dew_margin(&result.climate, &result.thermal);
    result.thermal_delta_c = compute_thermal_delta(&result.climate, &result.thermal);
    result.leaf = leaf_sample(tick, result.crop, result.climate.rh_percent, result.dew_margin_c, (tick % 2u) == 0u);
    result.airflow = derive_airflow(tick, result.crop);
    result.spore = spore_sample(tick, result.crop, result.climate.rh_percent, result.airflow.airflow_score);

    cs_risk_model_t model = cs_default_model(result.crop);
    result.risk_score = compute_risk_score(&result, &model);
    result.risk_level = classify_risk(result.risk_score, &model);

    device->total_scans++;
    return result;
}

static void print_component_breakdown(const cs_scan_result_t *result) {
    printf("[scan] climate air=%.2fC rh=%.1f%% co2=%.0fppm dew=%.2fC vpd=%.2fkPa\n",
           result->climate.air_c,
           result->climate.rh_percent,
           result->climate.co2_ppm,
           result->climate.dew_point_c,
           result->climate.vpd_kpa);
    printf("[scan] thermal leaf=%.2fC min=%.2f max=%.2f variance=%.3f delta=%.2f\n",
           result->thermal.leaf_c,
           result->thermal.min_c,
           result->thermal.max_c,
           result->thermal.variance,
           result->thermal_delta_c);
    printf("[scan] wetness=%.1f persistence=%.1f clip=%s\n",
           result->leaf.normalized_wetness,
           result->leaf.persistence_score,
           result->leaf.clip_attached ? "yes" : "no");
    printf("[scan] spore rate=%.1fHz fluor=%.1f pulses=%u\n",
           result->spore.event_rate_hz,
           result->spore.fluorescence_index,
           result->spore.pulse_count);
    printf("[scan] airflow_score=%.2f stagnation=%.1f dp=%.2fPa\n",
           result->airflow.airflow_score,
           result->airflow.stagnation_score,
           result->airflow.differential_pa);
}

int main(void) {
    cs_device_state_t device;
    cs_power_state_t power;
    cs_log_t log;
    char export_buffer[4096];

    init_device(&device);
    power_init(&power);
    storage_init(&log);
    climate_init();
    thermal_init();
    leaf_init(device.wetness_calibration_gain);
    spore_init(device.spore_threshold);
    display_init();
    display_show_boot(&device);
    ble_init(&device);

    cs_packet_t status = ble_build_status_packet(&device, &power);
    ble_print_packet(&status, "status");

    for (uint32_t i = 0; i < 6u; ++i) {
        power_tick(&power, true, (i % 3u) == 0u);
        cs_scan_result_t result = run_scan(&device, i, 1000u + i);
        print_component_breakdown(&result);
        display_show_result(&result, &power);
        cs_packet_t packet = ble_build_scan_packet(&result);
        ble_print_packet(&packet, "scan");
        storage_append(&log, &result);
    }

    display_show_storage(&log);
    storage_export_csv(&log, export_buffer, sizeof(export_buffer));
    printf("[export]\n%s", export_buffer);
    printf("[power] est_minutes_remaining=%.1f\n", power_estimated_minutes_remaining(&power));
    return 0;
}
