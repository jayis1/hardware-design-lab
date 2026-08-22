/*
 * Canopy Sentinel climate driver
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */
#include "climate.h"

#include <math.h>

void climate_init(void) {
}

float climate_compute_dew_point(float air_c, float rh_percent) {
    const float a = 17.625f;
    const float b = 243.04f;
    float rh = cs_clampf(rh_percent, 1.0f, 100.0f) / 100.0f;
    float gamma = logf(rh) + (a * air_c) / (b + air_c);
    return (b * gamma) / (a - gamma);
}

float climate_compute_vpd(float air_c, float rh_percent) {
    float svp = 0.6108f * expf((17.27f * air_c) / (air_c + 237.3f));
    float avp = svp * (cs_clampf(rh_percent, 0.0f, 100.0f) / 100.0f);
    return svp - avp;
}

cs_climate_sample_t climate_sample(uint32_t tick, cs_crop_profile_t crop) {
    cs_climate_sample_t sample;
    float crop_offset = (float)crop * 0.15f;
    sample.air_c = 17.5f + 5.0f * sinf(tick * 0.32f) + crop_offset + cs_rand_unit() * 0.5f;
    sample.rh_percent = cs_clampf(72.0f + 18.0f * cosf(tick * 0.27f) + crop_offset * 6.0f + cs_rand_unit() * 5.0f, 35.0f, 99.0f);
    sample.co2_ppm = 430.0f + 90.0f * sinf(tick * 0.18f + 0.6f) + (crop == CS_CROP_TOMATO ? 110.0f : 0.0f);
    sample.dew_point_c = climate_compute_dew_point(sample.air_c, sample.rh_percent);
    sample.vpd_kpa = climate_compute_vpd(sample.air_c, sample.rh_percent);
    return sample;
}
