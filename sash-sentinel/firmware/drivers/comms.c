/*
 * sash-sentinel/firmware/drivers/comms.c
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */

#include "comms.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

static device_config_t g_boot_config;

static int starts_with(const char *text, const char *prefix) {
    return strncmp(text, prefix, strlen(prefix)) == 0;
}

void comms_init(const device_config_t *config) {
    g_boot_config = *config;
}

void comms_encode_telemetry(const device_sample_t *sample, const risk_report_t *risk, char *out, size_t out_size) {
    snprintf(
        out,
        out_size,
        "{\"author\":\"jayis1\",\"product\":\"%s\",\"version\":\"%s\",\"epoch\":%u,"
        "\"env\":{\"indoor_temp_c\":%.2f,\"cavity_humidity_pct\":%.2f,\"dew_point_c\":%.2f,\"sill_moisture_pct\":%.2f},"
        "\"airflow\":{\"leak_velocity_mps\":%.2f,\"acoustic_leak_score\":%.1f},"
        "\"latch\":{\"force_n\":%.2f,\"offset_mm\":%.2f,\"closed\":%s},"
        "\"risk\":{\"condensation\":%.1f,\"infiltration\":%.1f,\"mold\":%.1f,\"latch_fault\":%.1f,\"level\":%d,\"summary\":\"%s\"}}",
        FW_PRODUCT_NAME,
        FW_VERSION,
        sample->epoch_s,
        sample->env.indoor_temp_c,
        sample->env.cavity_humidity_pct,
        sample->env.dew_point_c,
        sample->env.sill_moisture_pct,
        sample->airflow.leak_velocity_mps,
        sample->airflow.acoustic_leak_score,
        sample->latch.latch_force_n,
        sample->latch.sash_offset_mm,
        sample->latch.latch_closed ? "true" : "false",
        risk->condensation_risk,
        risk->infiltration_risk,
        risk->mold_risk,
        risk->latch_fault_risk,
        risk->level,
        risk->summary
    );
}

void comms_build_dashboard(const sample_history_t *history, const risk_report_t *risk, char *out, size_t out_size) {
    const device_sample_t *latest = board_history_latest(history);
    if (latest == NULL) {
        snprintf(out, out_size, "author=jayis1\nstatus=empty\n");
        return;
    }

    snprintf(
        out,
        out_size,
        "author=jayis1\nproduct=%s\nlocation=%s\ncondensation=%.1f\ninfiltration=%.1f\nmold=%.1f\n"
        "latest_temp=%.2f\nlatest_humidity=%.2f\nlatest_offset=%.2f\naction=%s\n",
        FW_PRODUCT_NAME,
        g_boot_config.install_location,
        risk->condensation_risk,
        risk->infiltration_risk,
        risk->mold_risk,
        latest->env.indoor_temp_c,
        latest->env.cavity_humidity_pct,
        latest->latch.sash_offset_mm,
        risk->action
    );
}

bool comms_apply_command(const char *command, device_config_t *config, char *response, size_t response_size) {
    if (starts_with(command, "set:ssid=")) {
        snprintf(config->ssid, sizeof(config->ssid), "%s", command + 9);
        snprintf(response, response_size, "ok:ssid:%s", config->ssid);
        return true;
    }
    if (starts_with(command, "set:location=")) {
        snprintf(config->install_location, sizeof(config->install_location), "%s", command + 13);
        snprintf(response, response_size, "ok:location:%s", config->install_location);
        return true;
    }
    if (starts_with(command, "set:buzzer=")) {
        config->enable_buzzer = strstr(command + 11, "on") != NULL;
        snprintf(response, response_size, "ok:buzzer:%s", config->enable_buzzer ? "on" : "off");
        return true;
    }
    if (strcmp(command, "get:profile") == 0) {
        snprintf(
            response,
            response_size,
            "author=jayis1;ssid=%s;location=%s;metric=%d;buzzer=%d",
            config->ssid,
            config->install_location,
            config->metric_units ? 1 : 0,
            config->enable_buzzer ? 1 : 0
        );
        return true;
    }

    char lower[64];
    size_t n = strlen(command);
    if (n >= sizeof(lower)) {
        snprintf(response, response_size, "error:command-too-long");
        return false;
    }
    for (size_t i = 0; i < n; ++i) {
        lower[i] = (char)tolower((unsigned char)command[i]);
    }
    lower[n] = '\0';

    if (strcmp(lower, "ping") == 0) {
        snprintf(response, response_size, "pong:jayis1:%s", FW_VERSION);
        return true;
    }

    snprintf(response, response_size, "error:unsupported-command");
    return false;
}
