/*
 * DrainVeil inference engine
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#include <string.h>
#include "inference.h"

static void set_reason(inference_state_t *state, const char *reason)
{
    size_t i = 0u;
    for (; reason[i] != '\0' && i + 1u < sizeof(state->reason); ++i) {
        state->reason[i] = reason[i];
    }
    state->reason[i] = '\0';
}

void inference_init(inference_state_t *state)
{
    memset(state, 0, sizeof(*state));
    set_reason(state, "baseline");
}

void inference_update(inference_state_t *state, const drain_snapshot_t *current, const drain_snapshot_t *previous, install_profile_t profile)
{
    float clog_from_fill = current->flow.fill_height_percent / 100.0f;
    float clog_from_pressure = current->pressure.blockage_gradient;
    float clog_from_time = current->flow.drain_time_s / 58.0f;
    float odor_from_gas = current->chemistry.h2s_ppm / 8.0f;
    float odor_from_voc = current->chemistry.voc_index / 300.0f;
    float freeze_from_margin = 1.0f - dv_clampf(current->thermal.freeze_margin_c / 8.0f, 0.0f, 1.0f);
    float freeze_from_slug = current->thermal.cold_slug_index;
    float profile_penalty = dv_profile_bias(profile, 0.00f, 0.08f, 0.06f);

    state->clog_risk = dv_clampf(0.34f * clog_from_fill + 0.34f * clog_from_pressure + 0.22f * clog_from_time + 0.10f * current->flow.slug_probability + profile_penalty, 0.0f, 1.0f);
    state->odor_risk = dv_clampf(0.36f * odor_from_gas + 0.28f * odor_from_voc + 0.20f * current->chemistry.biofilm_proxy + 0.16f * current->chemistry.grease_proxy, 0.0f, 1.0f);
    state->freeze_risk = dv_clampf(0.56f * freeze_from_margin + 0.20f * freeze_from_slug + 0.14f * current->thermal.heat_leak_score + 0.10f * current->chemistry.condensate_risk, 0.0f, 1.0f);
    state->efficiency_penalty = dv_clampf(0.45f * state->clog_risk + 0.25f * current->pressure.vibration_rms + 0.15f * current->thermal.heat_leak_score + 0.15f * current->flow.turbulence_index, 0.0f, 1.0f);
    state->service_score = dv_clampf(0.40f * state->clog_risk + 0.28f * state->odor_risk + 0.18f * state->freeze_risk + 0.14f * current->chemistry.corrosion_index, 0.0f, 1.0f);
    state->confidence = dv_clampf(0.62f + 0.12f * current->flow.reflection_strength + 0.10f * current->pressure.pulse_variance + 0.10f * current->chemistry.condensate_risk - 0.08f * current->power.rail_noise_mv / 30.0f, 0.0f, 1.0f);
    state->maintenance_priority = dv_clampf(0.44f * state->service_score + 0.24f * state->efficiency_penalty + 0.18f * previous->inference.maintenance_priority + 0.14f * (current->minute_index / 24.0f), 0.0f, 1.0f);

    if (state->freeze_risk > 0.72f) {
        set_reason(state, "freeze-risk");
        state->alert = ALERT_CRITICAL;
    } else if (state->clog_risk > 0.74f) {
        set_reason(state, "clog-growth");
        state->alert = ALERT_WARNING;
    } else if (state->odor_risk > 0.64f) {
        set_reason(state, "odor-plume");
        state->alert = ALERT_WARNING;
    } else if (state->service_score > 0.52f) {
        set_reason(state, "maintenance-window");
        state->alert = ALERT_CAUTION;
    } else if (current->power.battery_percent < 20.0f) {
        set_reason(state, "low-battery");
        state->alert = ALERT_INFO;
    } else {
        set_reason(state, "healthy-run");
        state->alert = ALERT_NONE;
    }
}

const char *inference_primary_reason(const drain_snapshot_t *snapshot)
{
    return snapshot->inference.reason;
}
