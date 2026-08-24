/*
 * SplintSense moisture strip model
 * Author: jayis1
 */
#include <math.h>
#include <stddef.h>
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

void moisture_init(moisture_frame_t *frame)
{
    size_t i;
    for (i = 0; i < SPLINTSENSE_MOISTURE_ZONES; ++i) {
        frame->zones[i] = 11.0f + (float)i * 0.8f;
    }
    frame->average = 12.8f;
    frame->peak = 15.0f;
    frame->persistence_minutes = 0.0f;
    frame->drying_rate = 1.2f;
}

void moisture_sample(moisture_frame_t *frame, uint32_t minute_index, float humidity_rh, float motion_factor)
{
    size_t i;
    float total = 0.0f;
    float peak = 0.0f;
    const float sweat_burst = (minute_index % 7u == 4u) ? 9.0f : 0.0f;
    const float accidental_splash = (minute_index == 23u) ? 16.0f : 0.0f;

    for (i = 0; i < SPLINTSENSE_MOISTURE_ZONES; ++i) {
        float zone = 8.0f + 0.14f * humidity_rh;
        zone += 2.8f * sinf((float)minute_index * 0.25f + (float)i * 0.61f);
        zone += sweat_burst * (i < 3u ? 1.0f : 0.55f);
        zone += accidental_splash * (i == 0u || i == 1u ? 1.0f : 0.3f);
        zone += 2.5f * motion_factor;
        zone = 0.72f * frame->zones[i] + 0.28f * zone;
        zone = clampf(zone, 2.0f, 92.0f);
        frame->zones[i] = zone;
        total += zone;
        if (zone > peak) {
            peak = zone;
        }
    }

    frame->average = total / (float)SPLINTSENSE_MOISTURE_ZONES;
    frame->peak = peak;
    if (frame->average > 28.0f || frame->peak > 38.0f) {
        frame->persistence_minutes += SPLINTSENSE_SIMULATION_MINUTE_SECONDS / 60.0f;
    } else {
        frame->persistence_minutes = fmaxf(frame->persistence_minutes - 2.0f, 0.0f);
    }
    frame->drying_rate = clampf(3.5f - motion_factor - (humidity_rh / 40.0f), 0.1f, 4.0f);
}

float moisture_burden_score(const moisture_frame_t *frame)
{
    float burden = frame->average * 1.2f + frame->peak * 0.8f + frame->persistence_minutes * 1.1f;
    return clampf(burden, 0.0f, 100.0f);
}
