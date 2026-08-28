/*
 * VentLattice inference engine
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#include <stdio.h>
#include <string.h>
#include "inference.h"

static float clampf(float value, float low, float high)
{
    if (value < low) return low;
    if (value > high) return high;
    return value;
}

void inference_init(inference_state_t *state)
{
    memset(state, 0, sizeof(*state));
    state->service_score = 82.0f;
    state->comfort_waste = 0.12f;
    state->stale_air_risk = 0.08f;
    state->maintenance_priority = 0.10f;
    state->condensation_risk = 0.04f;
    state->install_quality = 0.93f;
    state->alert = ALERT_NONE;
    snprintf(state->reason, sizeof(state->reason), "baseline healthy airflow");
}

void inference_update(inference_state_t *state, const vent_snapshot_t *current, const vent_snapshot_t *previous)
{
    float under_service;
    float stale;
    float waste;
    float maint;
    float cond;

    under_service = current->environment.thermal_need * 0.55f
                  + current->pressure.branch_restriction * 0.30f
                  + (1.0f - current->airflow.delivery_stability) * 0.15f;

    stale = current->occupancy.presence_confidence * 0.42f
          + (current->environment.voc_index / 260.0f) * 0.40f
          + (1.0f - (current->airflow.airflow_cfm / 110.0f)) * 0.18f;

    waste = (1.0f - current->occupancy.occupied_alignment) * 0.45f
          + (current->airflow.airflow_cfm / 120.0f) * 0.25f
          + (current->airflow.hvac_call_active ? 0.08f : 0.0f)
          - current->environment.thermal_need * 0.20f;

    maint = current->pressure.filter_load_index * 0.32f
          + current->pressure.branch_restriction * 0.36f
          + current->airflow.blockage_index * 0.18f
          + (previous->airflow.airflow_cfm > current->airflow.airflow_cfm ? 0.08f : 0.0f);

    cond = (current->environment.dew_margin_c < 1.5f ? 0.45f : 0.08f)
         + current->environment.humidity_rh / 100.0f * 0.28f
         + (current->environment.supply_temp_c < 18.0f ? 0.14f : 0.02f);

    state->comfort_waste = clampf(waste, 0.0f, 1.0f);
    state->stale_air_risk = clampf(stale, 0.0f, 1.0f);
    state->maintenance_priority = clampf(maint, 0.0f, 1.0f);
    state->condensation_risk = clampf(cond, 0.0f, 1.0f);
    state->install_quality = clampf(0.96f - current->airflow.blockage_index * 0.18f - current->pressure.turbulence_index * 0.10f, 0.50f, 0.98f);
    state->service_score = clampf(100.0f - under_service * 45.0f - state->stale_air_risk * 18.0f - state->maintenance_priority * 16.0f, 0.0f, 100.0f);

    if (current->airflow.blockage_index > 0.58f) {
        snprintf(state->reason, sizeof(state->reason), "partial register blockage likely");
    } else if (state->stale_air_risk > 0.56f && current->occupancy.occupied_now) {
        snprintf(state->reason, sizeof(state->reason), "occupied room under-ventilated");
    } else if (state->comfort_waste > 0.50f && !current->occupancy.occupied_now) {
        snprintf(state->reason, sizeof(state->reason), "conditioning empty room inefficiently");
    } else if (state->maintenance_priority > 0.52f) {
        snprintf(state->reason, sizeof(state->reason), "upstream filter or branch maintenance trend");
    } else {
        snprintf(state->reason, sizeof(state->reason), "delivery within expected room envelope");
    }

    if (state->service_score < 38.0f || state->maintenance_priority > 0.76f) {
        state->alert = ALERT_CRITICAL;
    } else if (state->service_score < 54.0f || state->stale_air_risk > 0.68f || current->airflow.blockage_index > 0.60f) {
        state->alert = ALERT_WARNING;
    } else if (state->service_score < 68.0f || state->comfort_waste > 0.48f || state->condensation_risk > 0.54f) {
        state->alert = ALERT_CAUTION;
    } else if (state->maintenance_priority > 0.36f || state->stale_air_risk > 0.34f) {
        state->alert = ALERT_INFO;
    } else {
        state->alert = ALERT_NONE;
    }
}

const char *inference_primary_reason(const vent_snapshot_t *snapshot)
{
    return snapshot->inference.reason;
}
