/*
 * SplintSense sensor hub and fusion logic
 * Author: jayis1
 */
#include <math.h>
#include <stdbool.h>
#include "sensor_hub.h"
#include "pressure.h"
#include "moisture.h"

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

void sensor_hub_init(env_frame_t *env, splint_profile_t profile)
{
    env->temperature_c = profile == SPLINT_PROFILE_ANKLE ? 31.2f : 30.3f;
    env->humidity_rh = profile == SPLINT_PROFILE_ANKLE ? 47.0f : 42.0f;
    env->voc_index = 72.0f;
    env->acceleration_g = 0.08f;
    env->orientation_drift = 1.4f;
    env->impact_g = 0.4f;
    env->step_count = 0u;
}

void sensor_hub_sample(env_frame_t *env, splint_profile_t profile, uint32_t minute_index)
{
    const float motion_block = (minute_index % 6u >= 2u && minute_index % 6u <= 4u) ? 1.0f : 0.2f;
    const float heat_offset = profile == SPLINT_PROFILE_ANKLE ? 1.0f : 0.0f;
    const float impact_event = (minute_index == 11u || minute_index == 27u) ? 2.1f : 0.5f;

    env->temperature_c = 29.6f + heat_offset + 2.6f * motion_block + 0.8f * sinf((float)minute_index * 0.21f);
    env->humidity_rh = 40.0f + 18.0f * motion_block + 5.5f * cosf((float)minute_index * 0.16f);
    env->voc_index = 62.0f + 6.0f * (float)minute_index * 0.25f + 7.0f * motion_block + 5.0f * sinf((float)minute_index * 0.12f);
    env->acceleration_g = 0.05f + motion_block * 0.8f;
    env->orientation_drift = fabsf(3.0f * sinf((float)minute_index * 0.09f)) + (minute_index > 22u ? 1.5f : 0.3f);
    env->impact_g = impact_event + 0.35f * sinf((float)minute_index * 0.53f);
    env->step_count += motion_block > 0.5f ? (uint32_t)(18u + (minute_index % 5u) * 7u) : 2u;
}

static alert_level_t derive_alert_level(const recovery_snapshot_t *snapshot, const recovery_snapshot_t *previous)
{
    const bool repeated_issue = previous->alert >= ALERT_CAUTION && snapshot->comfort_score < 68.0f;
    if (snapshot->power.battery_percent < 12.0f) {
        return ALERT_WARNING;
    }
    if (snapshot->pressure.max_zone > 54.0f || snapshot->moisture.persistence_minutes > 20.0f || snapshot->env.impact_g > 2.2f) {
        return ALERT_CRITICAL;
    }
    if (snapshot->fit_score < 50.0f || snapshot->odor_risk > 74.0f || repeated_issue) {
        return ALERT_WARNING;
    }
    if (snapshot->recovery_stability_index < 70.0f || snapshot->moisture.average > 26.0f) {
        return ALERT_CAUTION;
    }
    if (snapshot->env.humidity_rh > 55.0f || snapshot->env.orientation_drift > 3.5f) {
        return ALERT_INFO;
    }
    return ALERT_NONE;
}

void sensor_hub_fuse(recovery_snapshot_t *snapshot, const recovery_snapshot_t *previous)
{
    const float pressure_score = pressure_fit_score(&snapshot->pressure);
    const float moisture_score = 100.0f - moisture_burden_score(&snapshot->moisture);
    const float odor_score = 100.0f - clampf((snapshot->env.voc_index - 55.0f) * 1.3f + snapshot->moisture.average * 0.6f, 0.0f, 100.0f);
    const float temp_penalty = fabsf(snapshot->env.temperature_c - 32.0f) * 6.0f;
    const float humidity_penalty = fmaxf(snapshot->env.humidity_rh - 48.0f, 0.0f) * 1.4f;
    const float compliance_penalty = snapshot->env.impact_g * 12.0f + snapshot->env.orientation_drift * 5.0f;

    snapshot->fit_score = pressure_score;
    snapshot->odor_risk = 100.0f - odor_score;
    snapshot->comfort_score = clampf(100.0f - temp_penalty - humidity_penalty - snapshot->pressure.dwell_risk * 0.4f, 0.0f, 100.0f);
    snapshot->compliance_score = clampf(100.0f - compliance_penalty, 0.0f, 100.0f);
    snapshot->recovery_stability_index = clampf(
        pressure_score * 0.33f + moisture_score * 0.22f + odor_score * 0.15f + snapshot->comfort_score * 0.15f + snapshot->compliance_score * 0.15f,
        0.0f,
        100.0f
    );
    snapshot->alert = derive_alert_level(snapshot, previous);
}
