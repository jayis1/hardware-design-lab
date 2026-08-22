/*
 * Canopy Sentinel thermal driver
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */
#include "thermal.h"

#include <math.h>

void thermal_init(void) {
}

cs_thermal_frame_t thermal_capture(uint32_t tick, cs_crop_profile_t crop, float air_c) {
    cs_thermal_frame_t frame;
    frame.min_c = 999.0f;
    frame.max_c = -999.0f;
    float sum = 0.0f;
    float sum_sq = 0.0f;
    float crop_bias = 0.18f * (float)crop;

    for (int y = 0; y < CS_THERMAL_HEIGHT; ++y) {
        for (int x = 0; x < CS_THERMAL_WIDTH; ++x) {
            size_t idx = (size_t)y * CS_THERMAL_WIDTH + (size_t)x;
            float gradient = ((float)y / (float)CS_THERMAL_HEIGHT) * -0.8f;
            float ripple = 0.9f * sinf(0.19f * (float)x + tick * 0.21f) + 0.6f * cosf(0.13f * (float)y + tick * 0.16f);
            float value = air_c - 0.7f + gradient + ripple * 0.35f - crop_bias + cs_rand_unit() * 0.15f;
            if ((x > 10 && x < 16) && (y > 9 && y < 15)) {
                value -= 0.8f;
            }
            frame.pixels[idx] = value;
            if (value < frame.min_c) frame.min_c = value;
            if (value > frame.max_c) frame.max_c = value;
            sum += value;
            sum_sq += value * value;
        }
    }

    frame.mean_c = sum / (float)CS_THERMAL_PIXELS;
    frame.variance = (sum_sq / (float)CS_THERMAL_PIXELS) - (frame.mean_c * frame.mean_c);
    frame.leaf_c = frame.mean_c - 0.35f - 0.6f * sinf(tick * 0.24f);
    return frame;
}
