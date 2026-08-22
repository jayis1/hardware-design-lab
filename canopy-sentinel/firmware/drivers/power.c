/*
 * Canopy Sentinel power driver
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */
#include "power.h"

void power_init(cs_power_state_t *state) {
    state->battery_percent = 92.0f;
    state->charging = false;
    state->bus_voltage_v = 3.98f;
    state->current_ma = 48.0f;
}

void power_tick(cs_power_state_t *state, bool scanning, bool wifi_active) {
    float drain = 0.02f;
    if (scanning) {
        drain += 0.11f;
    }
    if (wifi_active) {
        drain += 0.04f;
    }
    if (state->charging) {
        state->battery_percent += 0.25f;
    } else {
        state->battery_percent -= drain;
    }
    state->battery_percent = cs_clampf(state->battery_percent, 0.0f, 100.0f);
    state->bus_voltage_v = 3.2f + (state->battery_percent / 100.0f) * 1.0f;
    state->current_ma = scanning ? 168.0f : 62.0f;
    if (wifi_active) {
        state->current_ma += 24.0f;
    }
}

float power_estimated_minutes_remaining(const cs_power_state_t *state) {
    float wh = 3.2f * (state->battery_percent / 100.0f);
    float average_w = 0.85f;
    return (wh / average_w) * 60.0f;
}
