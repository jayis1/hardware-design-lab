/*
 * thermal.c — exhaust thermal and humidity modeling
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#include <math.h>
#include "thermal.h"

static float g_exhaust = 28.0f;
static float g_ambient = 23.0f;
static float g_humidity = 42.0f;
static float g_skin = 26.0f;

static float filterf(float current, float sample, float alpha) {
    return current + alpha * (sample - current);
}

void thermal_init(void) {
    g_exhaust = 28.0f;
    g_ambient = 23.0f;
    g_humidity = 42.0f;
    g_skin = 26.0f;
}

void thermal_sample(dfg_sensor_frame_t *frame, uint32_t tick) {
    const float t = (float)tick;
    const float active = frame->run_state == DFG_RUN_ACTIVE ? 1.0f : 0.0f;
    const float drydown = frame->run_state == DFG_RUN_DRYDOWN ? 1.0f : 0.0f;
    const float raw_exhaust = 24.0f + active * (34.0f + 4.0f * sinf(t / 6.0f)) + drydown * 16.0f;
    const float raw_ambient = 22.8f + 0.7f * sinf(t / 22.0f);
    const float moisture_peak = active ? 38.0f * expf(-powf((t - 18.0f) / 15.0f, 2.0f)) : 0.0f;
    const float raw_humidity = 35.0f + moisture_peak + drydown * 9.0f;
    const float raw_skin = 23.0f + 0.42f * raw_exhaust;

    g_exhaust = filterf(g_exhaust, raw_exhaust, DFG_THERMAL_FILTER_ALPHA);
    g_ambient = filterf(g_ambient, raw_ambient, 0.06f);
    g_humidity = filterf(g_humidity, raw_humidity, DFG_HUMIDITY_FILTER_ALPHA);
    g_skin = filterf(g_skin, raw_skin, 0.10f);

    frame->exhaust_temp_c = g_exhaust;
    frame->ambient_temp_c = g_ambient;
    frame->humidity_rh = g_humidity;
    frame->duct_skin_temp_c = g_skin;
}

float thermal_estimate_dryness_minutes(const dfg_sensor_frame_t *history, uint32_t count) {
    if (!history || count == 0U) {
        return 0.0f;
    }

    float peak = history[0].humidity_rh;
    float threshold = 0.0f;
    uint32_t i;

    for (i = 0; i < count; ++i) {
        if (history[i].humidity_rh > peak) {
            peak = history[i].humidity_rh;
        }
    }

    threshold = peak - 12.0f;
    for (i = 0; i < count; ++i) {
        if (history[i].humidity_rh <= threshold && history[i].run_state >= DFG_RUN_ACTIVE) {
            return (float)i / (float)DFG_SAMPLE_RATE_HZ / 60.0f;
        }
    }

    return (float)count / (float)DFG_SAMPLE_RATE_HZ / 60.0f;
}
