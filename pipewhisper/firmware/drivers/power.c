/*
 * PipeWhisper power driver
 * Author: jayis1
 */
#include "power.h"

static float clampf(float value, float minimum, float maximum)
{
    if (value < minimum) return minimum;
    if (value > maximum) return maximum;
    return value;
}

void power_init(power_frame_t *frame, pipe_profile_t profile)
{
    frame->battery_percent = profile == PIPE_PROFILE_LAUNDRY_HOT ? 96.0f : 94.0f;
    frame->battery_mv = 4140.0f;
    frame->charging = 0u;
    frame->low_power_hint = 0u;
}

void power_update(power_frame_t *frame, const power_frame_t *previous, const pipe_snapshot_t *snapshot)
{
    float activity_cost = snapshot->acoustic.acoustic_rms * 0.22f + snapshot->pressure.impulse_count * 0.014f + snapshot->flow.signature_drift * 0.07f;
    frame->battery_percent = clampf(previous->battery_percent - activity_cost, 0.0f, 100.0f);
    frame->battery_mv = 3600.0f + frame->battery_percent * 5.7f;
    frame->charging = 0u;
    frame->low_power_hint = frame->battery_percent < 20.0f ? 1u : 0u;
}

float power_status_register(const power_frame_t *frame)
{
    return (frame->charging ? 2.0f : 0.0f) + (frame->low_power_hint ? 1.0f : 0.0f);
}
