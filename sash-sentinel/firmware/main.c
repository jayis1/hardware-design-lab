/*
 * sash-sentinel/firmware/main.c
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */

#include "board.h"
#include "registers.h"
#include "drivers/airflow.h"
#include "drivers/comms.h"
#include "drivers/env.h"
#include "drivers/inference.h"
#include "drivers/latch.h"
#include "drivers/logger.h"
#include "drivers/power.h"
#include "drivers/thermal.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

uint32_t g_register_bank[256];

void board_init_config(device_config_t *config) {
    snprintf(config->ssid, sizeof(config->ssid), "sash-lab");
    snprintf(config->install_location, sizeof(config->install_location), "north-bedroom-window");
    config->expected_pressure_bias_pa = -1.5f;
    config->acceptable_condensation_score = 24.0f;
    config->enable_buzzer = true;
    config->metric_units = true;
}

void board_history_init(sample_history_t *history) {
    memset(history, 0, sizeof(*history));
}

void board_history_push(sample_history_t *history, const device_sample_t *sample) {
    history->samples[history->head] = *sample;
    history->head = (history->head + 1u) % FW_SAMPLE_HISTORY;
    if (history->count < FW_SAMPLE_HISTORY) {
        ++history->count;
    }
}

const device_sample_t *board_history_latest(const sample_history_t *history) {
    if (history->count == 0u) {
        return NULL;
    }
    size_t index = (history->head + FW_SAMPLE_HISTORY - 1u) % FW_SAMPLE_HISTORY;
    return &history->samples[index];
}

float board_clampf(float value, float min_value, float max_value) {
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}

float board_lerpf(float a, float b, float t) {
    return a + (b - a) * t;
}

float board_average(const float *values, size_t count) {
    if (count == 0u) {
        return 0.0f;
    }
    float total = 0.0f;
    for (size_t i = 0; i < count; ++i) {
        total += values[i];
    }
    return total / (float)count;
}

float board_max(const float *values, size_t count) {
    if (count == 0u) {
        return 0.0f;
    }
    float max_value = values[0];
    for (size_t i = 1; i < count; ++i) {
        if (values[i] > max_value) {
            max_value = values[i];
        }
    }
    return max_value;
}

float board_min(const float *values, size_t count) {
    if (count == 0u) {
        return 0.0f;
    }
    float min_value = values[0];
    for (size_t i = 1; i < count; ++i) {
        if (values[i] < min_value) {
            min_value = values[i];
        }
    }
    return min_value;
}

static void update_registers(const device_sample_t *sample, const risk_report_t *risk) {
    reg_write_u32(REG_SYS_STATUS, SYS_STATUS_BOOTED | SYS_STATUS_CONFIGURED | SYS_STATUS_WIFI_READY | SYS_STATUS_BLE_READY | SYS_STATUS_STORAGE_OK);
    reg_write_u32(REG_ENV_STATUS, (uint32_t)(sample->env.cavity_humidity_pct * 10.0f));
    reg_write_u32(REG_THERMAL_STATUS, (uint32_t)(sample->thermal.edge_cold_spot_c * 100.0f));
    reg_write_u32(REG_LATCH_STATUS, (uint32_t)(sample->latch.sash_offset_mm * 100.0f));
    reg_write_u32(REG_AIRFLOW_STATUS, (uint32_t)(sample->airflow.leak_velocity_mps * 100.0f));
    reg_write_u32(REG_POWER_STATUS, (uint32_t)(sample->power.battery_percent * 10.0f));

    uint32_t alert_bits = 0u;
    if (risk->level == ALERT_INFO) {
        alert_bits |= ALERT_STATUS_INFO;
    } else if (risk->level == ALERT_WARNING) {
        alert_bits |= ALERT_STATUS_WARNING;
    } else if (risk->level == ALERT_CRITICAL) {
        alert_bits |= ALERT_STATUS_CRITICAL;
    }
    reg_write_u32(REG_ALERT_STATUS, alert_bits);
    reg_write_u32(REG_EVENT_COUNTER, (uint32_t)logger_count());
}

static void emit_log_if_needed(const device_sample_t *sample, const risk_report_t *risk) {
    if (risk->level >= ALERT_WARNING) {
        logger_log(risk->level, "risk", risk->summary);
    }
    if (sample->thermal.frost_signature) {
        logger_log(ALERT_WARNING, "thermal", "Cold-edge frost signature observed at lower seal.");
    }
    if (!sample->latch.latch_closed) {
        logger_log(ALERT_INFO, "latch", "Latch compression below target; sash alignment check recommended.");
    }
    if (sample->airflow.rain_pattern) {
        logger_log(ALERT_WARNING, "airflow", "Pressure and humidity pattern resembles wind-driven rain ingress.");
    }
}

static void print_console_report(const device_sample_t *sample, const risk_report_t *risk) {
    char telemetry[FW_TELEMETRY_TEXT_CAPACITY];
    comms_encode_telemetry(sample, risk, telemetry, sizeof(telemetry));
    printf("Telemetry: %s\n", telemetry);
    printf("Summary: %s\n", risk->summary);
    printf("Action : %s\n", risk->action);
    printf("Battery: %.1f%% at %.2f V\n", sample->power.battery_percent, sample->power.battery_voltage_v);
    printf("Registers env=%u thermal=%u airflow=%u alerts=%u events=%u\n\n",
           reg_read_u32(REG_ENV_STATUS),
           reg_read_u32(REG_THERMAL_STATUS),
           reg_read_u32(REG_AIRFLOW_STATUS),
           reg_read_u32(REG_ALERT_STATUS),
           reg_read_u32(REG_EVENT_COUNTER));
}

static float compute_ventilation_bias(uint32_t tick) {
    float base = sinf((float)tick * 0.28f) * 0.8f;
    float occupancy = cosf((float)tick * 0.12f) * 0.35f;
    return base + occupancy;
}

static void run_self_test(device_config_t *config) {
    char response[128];
    const char *commands[] = {"ping", "get:profile", "set:location=office-east-double-hung", "set:buzzer=off"};
    const size_t command_count = sizeof(commands) / sizeof(commands[0]);

    for (size_t i = 0; i < command_count; ++i) {
        bool ok = comms_apply_command(commands[i], config, response, sizeof(response));
        printf("Command %-36s -> %s (%s)\n", commands[i], response, ok ? "ok" : "err");
    }
    puts("");
}

int main(void) {
    device_config_t config;
    sample_history_t history;

    board_init_config(&config);
    board_history_init(&history);
    env_init();
    thermal_init();
    latch_init();
    airflow_init();
    power_init();
    logger_init();
    comms_init(&config);
    inference_init();

    run_self_test(&config);

    for (uint32_t tick = 1u; tick <= 18u; ++tick) {
        device_sample_t sample;
        memset(&sample, 0, sizeof(sample));
        sample.epoch_s = (uint32_t)time(NULL) + tick * 60u;

        float ventilation_bias = compute_ventilation_bias(tick);
        sample.env = env_sample(tick, &config, ventilation_bias);
        sample.thermal = thermal_sample(tick, &sample.env);
        sample.latch = latch_sample(tick, &sample.thermal);
        sample.airflow = airflow_sample(tick, &sample.env, &sample.latch);
        sample.power = power_sample(tick, (tick % 3u) == 0u);

        board_history_push(&history, &sample);
        risk_report_t risk = inference_evaluate(&history, &config);
        update_registers(&sample, &risk);
        emit_log_if_needed(&sample, &risk);
        print_console_report(&sample, &risk);
    }

    char dashboard[FW_TELEMETRY_TEXT_CAPACITY];
    risk_report_t final_risk = inference_evaluate(&history, &config);
    comms_build_dashboard(&history, &final_risk, dashboard, sizeof(dashboard));
    printf("Dashboard Snapshot\n%s\n", dashboard);

    log_entry_t entries[FW_LOG_CAPACITY];
    size_t log_count = logger_copy(entries, FW_LOG_CAPACITY);
    printf("Event Log (%zu entries)\n", log_count);
    for (size_t i = 0; i < log_count; ++i) {
        printf("[%u] level=%d category=%s message=%s\n",
               entries[i].timestamp_s,
               entries[i].level,
               entries[i].category,
               entries[i].message);
    }

    return 0;
}
