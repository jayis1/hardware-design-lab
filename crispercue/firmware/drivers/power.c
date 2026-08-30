/*
 * CrisperCue power management driver
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#include "power.h"

void power_init(power_state_t *state)
{
    state->battery_mv = 4170.0f;
    state->battery_percent = 100.0f;
    state->current_ma = 18.0f;
    state->solar_lux_recovery = 120.0f;
    state->charging = 0u;
}

void power_update(power_state_t *state, const power_state_t *previous, const crisper_snapshot_t *snapshot)
{
    const float sensing_load = 3.5f + snapshot->thermal.drawer_open_minutes * 0.7f + snapshot->gas.voc_index * 0.05f;
    const float radio_load = 1.2f + snapshot->inference.ventilation_demand * 4.0f;
    const float recharge = (snapshot->thermal.drawer_open_minutes > 5.5f) ? 1.8f : 0.6f;

    state->current_ma = sensing_load + radio_load;
    state->battery_percent = previous->battery_percent - 0.55f + recharge * 0.08f;
    if (state->battery_percent < 18.0f) {
        state->charging = 1u;
        state->battery_percent += 1.6f;
        state->solar_lux_recovery = 260.0f;
    } else {
        state->charging = 0u;
        state->solar_lux_recovery = 110.0f + snapshot->thermal.drawer_open_minutes * 9.0f;
    }
    if (state->battery_percent > 100.0f) {
        state->battery_percent = 100.0f;
    }
    state->battery_mv = 3600.0f + state->battery_percent * 5.7f;
}

unsigned power_status_register(const power_state_t *state)
{
    unsigned status = 0u;
    if (state->charging) {
        status |= 0x01u;
    }
    if (state->battery_percent < 25.0f) {
        status |= 0x02u;
    }
    if (state->current_ma > 28.0f) {
        status |= 0x04u;
    }
    return status;
}
