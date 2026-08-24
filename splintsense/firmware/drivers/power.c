/*
 * SplintSense power model
 * Author: jayis1
 */
#include <math.h>
#include "../registers.h"
#include "power.h"

static float clampf(float value, float minimum, float maximum)
{
    if (value < minimum) {
        return minimum;
    }
    if (value > maximum) {
        return maximum;
    }
    return value;
}

void power_init(power_state_t *state, splint_profile_t profile)
{
    state->battery_mv = profile == SPLINT_PROFILE_ANKLE ? 4140.0f : 4090.0f;
    state->battery_percent = 100.0f;
    state->charging = false;
    state->estimated_hours_remaining = profile == SPLINT_PROFILE_ANKLE ? 240.0f : 168.0f;
}

void power_update(power_state_t *state, const recovery_snapshot_t *previous, float load_factor, uint32_t minute_index)
{
    const float radio_penalty = (previous->alert >= ALERT_WARNING) ? 0.35f : 0.18f;
    const float activity_penalty = 0.22f * load_factor;
    const float baseline_penalty = 0.40f;
    const float discharge = baseline_penalty + activity_penalty + radio_penalty;

    state->charging = ((minute_index % 19u) == 0u && minute_index != 0u);
    if (state->charging) {
        state->battery_percent = clampf(state->battery_percent + 4.2f, 0.0f, 100.0f);
    } else {
        state->battery_percent = clampf(state->battery_percent - discharge, 0.0f, 100.0f);
    }

    {
        const float span = SPLINTSENSE_BATTERY_FULL_MV - SPLINTSENSE_BATTERY_EMPTY_MV;
        state->battery_mv = SPLINTSENSE_BATTERY_EMPTY_MV + span * (state->battery_percent / 100.0f);
    }
    state->estimated_hours_remaining = clampf(state->battery_percent * 1.8f, 0.0f, 240.0f);
}

float power_status_register(const power_state_t *state)
{
    float status = 0.0f;
    if (state->charging) {
        status += STATUS_CHARGING_MASK;
    }
    if (state->battery_percent <= SPLINTSENSE_LOW_BATTERY_PERCENT) {
        status += STATUS_LOW_BATT_MASK;
    }
    return status;
}
