/*
 * CrisperCue inference engine
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "inference.h"

static float clampf(float value, float low, float high)
{
    if (value < low) {
        return low;
    }
    if (value > high) {
        return high;
    }
    return value;
}

static float profile_value_per_kg(bin_profile_t profile)
{
    switch (profile) {
    case BIN_PROFILE_LEAFY_GREENS: return 8.0f;
    case BIN_PROFILE_BERRIES: return 18.0f;
    case BIN_PROFILE_CLIMACTERIC_FRUIT: return 6.0f;
    default: return 7.0f;
    }
}

void inference_init(inference_state_t *state)
{
    state->freshness_score = 92.0f;
    state->spoilage_risk = 0.10f;
    state->recipe_urgency = 0.18f;
    state->ventilation_demand = 0.12f;
    state->shopper_value_left_usd = 6.80f;
    state->alert = ALERT_NONE;
    strncpy(state->stage, "fresh", sizeof(state->stage));
    strncpy(state->reason, "bin initialized", sizeof(state->reason));
}

void inference_update(inference_state_t *state, bin_profile_t profile, const crisper_snapshot_t *current, const crisper_snapshot_t *previous)
{
    float freshness_penalty = 0.0f;
    freshness_penalty += current->gas.ethylene_ppm * 18.0f;
    freshness_penalty += fmaxf(0.0f, current->gas.co2_ppm - 1000.0f) / 38.0f;
    freshness_penalty += current->mass.moisture_loss_percent * 1.15f;
    freshness_penalty += current->optical.bruise_probability * 22.0f;
    freshness_penalty += current->optical.mold_signature * 34.0f;
    freshness_penalty += fmaxf(0.0f, 1.2f - current->thermal.dew_margin_c) * 8.0f;

    state->freshness_score = clampf(100.0f - freshness_penalty, 0.0f, 100.0f);
    state->spoilage_risk = clampf((100.0f - state->freshness_score) / 100.0f + current->optical.mold_signature * 0.2f, 0.0f, 1.0f);
    state->recipe_urgency = clampf(current->mass.usage_velocity * 0.35f + state->spoilage_risk * 0.7f + (current->mass.tray_mass_g < 150.0f ? 0.1f : 0.0f), 0.0f, 1.0f);
    state->ventilation_demand = clampf((current->gas.ethylene_ppm * 0.9f) + (fmaxf(0.0f, current->gas.co2_ppm - 1100.0f) / 900.0f) + (1.0f - current->gas.purge_efficiency), 0.0f, 1.0f);
    state->shopper_value_left_usd = (current->mass.tray_mass_g / 1000.0f) * profile_value_per_kg(profile) * (state->freshness_score / 100.0f);

    if (state->freshness_score > 82.0f) {
        strncpy(state->stage, "fresh", sizeof(state->stage));
    } else if (state->freshness_score > 62.0f) {
        strncpy(state->stage, "ready-now", sizeof(state->stage));
    } else if (state->freshness_score > 40.0f) {
        strncpy(state->stage, "use-soon", sizeof(state->stage));
    } else {
        strncpy(state->stage, "rescue-immediately", sizeof(state->stage));
    }

    if (state->spoilage_risk > 0.82f || current->optical.mold_signature > 0.78f) {
        state->alert = ALERT_CRITICAL;
        snprintf(state->reason, sizeof(state->reason),
                 "surface spoilage signature high; isolate produce and sanitize bin");
    } else if (state->ventilation_demand > 0.72f || current->gas.ethylene_ppm > 0.72f) {
        state->alert = ALERT_WARNING;
        snprintf(state->reason, sizeof(state->reason),
                 "ripening gases accumulating faster than purge fan can clear");
    } else if (state->recipe_urgency > 0.58f || current->mass.moisture_loss_percent > 12.0f) {
        state->alert = ALERT_CAUTION;
        snprintf(state->reason, sizeof(state->reason),
                 "quality drop detected; suggest recipe rescue or dehydration control");
    } else if (fabsf(current->mass.tray_mass_g - previous->mass.tray_mass_g) > 60.0f) {
        state->alert = ALERT_INFO;
        snprintf(state->reason, sizeof(state->reason),
                 "inventory changed quickly; update app inventory estimate");
    } else {
        state->alert = ALERT_NONE;
        snprintf(state->reason, sizeof(state->reason),
                 "conditions stable for current produce class");
    }
}

const char *inference_primary_reason(const inference_state_t *state)
{
    return state->reason;
}
