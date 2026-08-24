/*
 * SplintSense pressure simulation and analytics
 * Author: jayis1
 */
#include <math.h>
#include <stddef.h>
#include "pressure.h"

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

static float zone_bias(splint_profile_t profile, size_t zone)
{
    static const float wrist_bias[SPLINTSENSE_PRESSURE_ZONES] = {0.85f, 0.92f, 1.04f, 1.10f, 0.96f, 1.05f, 0.88f, 0.82f};
    static const float ankle_bias[SPLINTSENSE_PRESSURE_ZONES] = {1.15f, 1.12f, 0.94f, 0.90f, 0.98f, 1.05f, 1.09f, 0.93f};
    return profile == SPLINT_PROFILE_ANKLE ? ankle_bias[zone] : wrist_bias[zone];
}

void pressure_init(pressure_frame_t *frame, splint_profile_t profile)
{
    size_t i;
    for (i = 0; i < SPLINTSENSE_PRESSURE_ZONES; ++i) {
        frame->zones[i] = 18.0f * zone_bias(profile, i);
    }
    frame->max_zone = 18.0f;
    frame->average_zone = 18.0f;
    frame->asymmetry = 0.0f;
    frame->dwell_risk = 4.0f;
}

void pressure_sample(pressure_frame_t *frame, splint_profile_t profile, uint32_t minute_index, float motion_factor)
{
    size_t i;
    float total = 0.0f;
    float max_value = 0.0f;
    float left_total = 0.0f;
    float right_total = 0.0f;
    const float gait_wave = 7.5f * sinf((float)minute_index * 0.45f);
    const float rest_wave = 3.0f * cosf((float)minute_index * 0.17f);
    const float fit_drift = (minute_index > 18u) ? 4.0f : 0.0f;

    for (i = 0; i < SPLINTSENSE_PRESSURE_ZONES; ++i) {
        const float phase = (float)i * 0.72f;
        float sample = 20.0f;
        sample += gait_wave * motion_factor * sinf((float)minute_index * 0.30f + phase);
        sample += rest_wave * cosf((float)minute_index * 0.11f + phase * 0.4f);
        sample += fit_drift * (i == 0u || i == 1u ? 1.0f : 0.2f);
        sample *= zone_bias(profile, i);
        sample = clampf(sample, 4.0f, 68.0f);
        frame->zones[i] = sample;
        total += sample;
        if (sample > max_value) {
            max_value = sample;
        }
        if (i < SPLINTSENSE_PRESSURE_ZONES / 2u) {
            left_total += sample;
        } else {
            right_total += sample;
        }
    }

    frame->max_zone = max_value;
    frame->average_zone = total / (float)SPLINTSENSE_PRESSURE_ZONES;
    frame->asymmetry = fabsf(left_total - right_total) / (total + 0.01f) * 100.0f;
    frame->dwell_risk = clampf((frame->max_zone - frame->average_zone) * 1.4f + frame->asymmetry * 0.5f, 0.0f, 100.0f);
}

float pressure_fit_score(const pressure_frame_t *frame)
{
    float score = 100.0f;
    score -= frame->asymmetry * 0.9f;
    score -= frame->dwell_risk * 0.55f;
    score -= fmaxf(frame->max_zone - 42.0f, 0.0f) * 0.6f;
    return clampf(score, 0.0f, 100.0f);
}
