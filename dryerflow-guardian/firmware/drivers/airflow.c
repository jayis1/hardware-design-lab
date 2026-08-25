/*
 * airflow.c — airflow estimation and vent resistance support
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#include <math.h>
#include "airflow.h"

static float g_flow_filtered = 84.0f;

static float clampf(float value, float minimum, float maximum) {
    if (value < minimum) {
        return minimum;
    }
    if (value > maximum) {
        return maximum;
    }
    return value;
}

void airflow_init(void) {
    g_flow_filtered = 84.0f;
}

void airflow_estimate(dfg_sensor_frame_t *frame, const dfg_baseline_t *baseline) {
    const float density_correction = DFG_STANDARD_AIR_DENSITY * (293.15f / (frame->exhaust_temp_c + 273.15f));
    const float root_term = sqrtf(fmaxf(frame->pressure_pa, 0.1f) / fmaxf(density_correction, 0.2f));
    const float nominal_cms = 0.0305f * root_term;
    const float raw_cfm = nominal_cms * 2118.88f;
    const float humidity_penalty = 1.0f - 0.0018f * fmaxf(frame->humidity_rh - 40.0f, 0.0f);
    const float acoustic_penalty = 1.0f - 0.0030f * frame->turbulence_score;
    const float corrected = raw_cfm * humidity_penalty * acoustic_penalty;
    const float bounded = clampf(corrected, 20.0f, 220.0f);

    g_flow_filtered += DFG_FLOW_FILTER_ALPHA * (bounded - g_flow_filtered);
    frame->flow_cfm = g_flow_filtered;

    if (baseline && baseline->valid) {
        const float ratio = baseline->baseline_flow_cfm / fmaxf(frame->flow_cfm, 1.0f);
        frame->static_pressure_pa += 0.12f * ratio;
    }
}
