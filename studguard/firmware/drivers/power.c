/*
 * power.c — StudGuard power and interval policy
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#include "power.h"
#include "../registers.h"

static float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

void power_init(power_state_t *state) {
    state->battery_percent = 100.0f;
    state->voltage_mv = 4180.0f;
    state->current_ma = 0.9f;
    state->interval_ms = NOMINAL_INTERVAL_MS;
    state->charging = 0u;
    SG_PWR.CTRL = 1u;
    SG_PWR.SOC_MILLIPCT = 100000u;
    SG_PWR.VBAT_MV = 4180u;
    SG_PWR.CURRENT_MA = 1u;
}

uint32_t power_choose_interval(const power_state_t *state, sg_mode_t mode, float leak_activity, float confidence) {
    if (mode == MODE_BASELINE) {
        return 60000u;
    }
    if (mode == MODE_DIAGNOSTIC) {
        return DIAGNOSTIC_INTERVAL_MS;
    }
    if (mode == MODE_REPAIR_VERIFICATION) {
        return 120000u;
    }
    if (leak_activity > 0.82f && confidence > 0.70f) {
        return CRITICAL_INTERVAL_MS;
    }
    if (leak_activity > 0.60f || confidence < 0.45f) {
        return DIAGNOSTIC_INTERVAL_MS;
    }
    if (state->battery_percent < 25.0f) {
        return 1800000u;
    }
    return NOMINAL_INTERVAL_MS;
}

void power_tick(power_state_t *state, sg_mode_t mode, float leak_activity, float confidence, uint32_t elapsed_ms) {
    float drain = 0.0025f * ((float)elapsed_ms / 1000.0f);
    if (mode == MODE_DIAGNOSTIC) {
        drain *= 3.2f;
    } else if (mode == MODE_BASELINE) {
        drain *= 1.7f;
    }
    if (leak_activity > 0.7f) {
        drain *= 1.2f;
    }
    state->battery_percent = clampf(state->battery_percent - drain, 2.0f, 100.0f);
    state->voltage_mv = 3300.0f + (state->battery_percent / 100.0f) * 900.0f;
    state->current_ma = 0.8f + (float)(mode + 1) * 0.6f + leak_activity * 1.5f;
    state->interval_ms = power_choose_interval(state, mode, leak_activity, confidence);

    SG_PWR.SOC_MILLIPCT = (uint32_t)(state->battery_percent * 1000.0f);
    SG_PWR.VBAT_MV = (uint32_t)state->voltage_mv;
    SG_PWR.CURRENT_MA = (uint32_t)(state->current_ma * 100.0f);
    SG_PWR.STATUS = state->battery_percent < 10.0f ? 2u : 1u;
}
