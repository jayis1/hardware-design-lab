/*
 * Canopy Sentinel spore-event driver
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */
#include "spore.h"

#include <math.h>

static float g_spore_threshold = 0.32f;

void spore_init(float threshold) {
    g_spore_threshold = threshold;
}

cs_spore_sample_t spore_sample(uint32_t tick, cs_crop_profile_t crop, float humidity_percent, float airflow_score) {
    cs_spore_sample_t sample;
    float crop_factor = 0.12f * (float)crop;
    float humidity_factor = humidity_percent / 100.0f;
    float activity = humidity_factor * (1.0f - airflow_score * 0.5f) + crop_factor;
    float signal = 0.22f + 0.3f * sinf(tick * 0.41f) + activity * 0.7f + cs_rand_unit() * 0.08f;
    sample.baseline = 0.14f + humidity_factor * 0.12f;
    sample.peak = signal;
    sample.fluorescence_index = cs_clampf((signal - g_spore_threshold) * 180.0f, 0.0f, 100.0f);
    sample.pulse_count = (uint32_t)(sample.fluorescence_index * 0.8f + 4.0f + activity * 12.0f);
    sample.event_rate_hz = sample.pulse_count / 3.0f;
    return sample;
}
