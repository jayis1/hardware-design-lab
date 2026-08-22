/*
 * Canopy Sentinel leaf wetness driver
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */
#include "leaf.h"

static float g_leaf_gain = 1.0f;

void leaf_init(float calibration_gain) {
    g_leaf_gain = calibration_gain;
}

cs_leaf_sample_t leaf_sample(uint32_t tick, cs_crop_profile_t crop, float rh_percent, float dew_margin_c, bool attach_clip) {
    (void)tick;
    (void)crop;
    cs_leaf_sample_t sample;
    float humidity_factor = rh_percent / 100.0f;
    float condensation_push = dew_margin_c < 0.0f ? (-dew_margin_c * 0.3f) : 0.0f;
    sample.raw_conductive = (humidity_factor + condensation_push + cs_rand_unit() * 0.06f) * g_leaf_gain;
    sample.raw_capacitive = (0.4f + humidity_factor * 0.7f + condensation_push * 0.6f + cs_rand_unit() * 0.04f) * g_leaf_gain;
    sample.normalized_wetness = cs_clampf((sample.raw_conductive * 0.65f + sample.raw_capacitive * 0.35f) * 100.0f, 0.0f, 100.0f);
    sample.persistence_score = cs_clampf(sample.normalized_wetness * 0.75f + humidity_factor * 18.0f, 0.0f, 100.0f);
    sample.clip_attached = attach_clip;
    if (!attach_clip) {
        sample.normalized_wetness *= 0.85f;
        sample.persistence_score *= 0.82f;
    }
    if (dew_margin_c < -1.2f) {
        sample.normalized_wetness = cs_clampf(sample.normalized_wetness + 12.0f, 0.0f, 100.0f);
    }
    return sample;
}
