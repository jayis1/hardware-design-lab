/*
 * analytics.c — sensor fusion and risk scoring
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#include <math.h>
#include <string.h>
#include "analytics.h"
#include "thermal.h"

static float clamp01(float value) {
    if (value < 0.0f) {
        return 0.0f;
    }
    if (value > 1.0f) {
        return 1.0f;
    }
    return value;
}

void analytics_init(dfg_baseline_t *baseline) {
    if (!baseline) {
        return;
    }
    baseline->baseline_pressure_pa = 8.0f;
    baseline->baseline_flow_cfm = 92.0f;
    baseline->baseline_dryness_minutes = 18.0f;
    baseline->baseline_turbulence = 4.2f;
    baseline->baseline_exhaust_temp_c = 54.0f;
    baseline->baseline_humidity_peak = 64.0f;
    baseline->valid = true;
}

void analytics_update(const dfg_sensor_frame_t *history, uint32_t count,
                      dfg_baseline_t *baseline,
                      dfg_health_metrics_t *metrics,
                      dfg_sensor_frame_t *current) {
    float pressure_ratio;
    float flow_ratio;
    float turbulence_ratio;
    float dryness_minutes;
    float dryness_ratio;
    float co_ratio;
    float thermal_ratio;
    float vri;
    float ces;
    float bss;
    float lgr;
    float horizon;

    if (!history || !baseline || !metrics || !current || count == 0U) {
        return;
    }

    dryness_minutes = thermal_estimate_dryness_minutes(history, count);
    pressure_ratio = current->pressure_pa / fmaxf(baseline->baseline_pressure_pa, 0.1f);
    flow_ratio = baseline->baseline_flow_cfm / fmaxf(current->flow_cfm, 1.0f);
    turbulence_ratio = current->turbulence_score / fmaxf(baseline->baseline_turbulence, 0.1f);
    dryness_ratio = dryness_minutes / fmaxf(baseline->baseline_dryness_minutes, 1.0f);
    co_ratio = current->co_ppm / fmaxf(DFG_SAFE_CO_PPM, 0.1f);
    thermal_ratio = current->exhaust_temp_c / fmaxf(baseline->baseline_exhaust_temp_c, 1.0f);

    vri = 100.0f * clamp01(0.35f * (pressure_ratio - 0.7f)
                         + 0.30f * (flow_ratio - 0.7f)
                         + 0.20f * (turbulence_ratio - 0.5f)
                         + 0.15f * (dryness_ratio - 0.7f));

    ces = 100.0f * clamp01(1.15f
                         - 0.35f * (flow_ratio - 1.0f)
                         - 0.20f * (dryness_ratio - 1.0f)
                         - 0.10f * fabsf(thermal_ratio - 1.0f));

    bss = 100.0f * clamp01(0.50f * co_ratio
                         + 0.18f * (current->voc_index / 25.0f)
                         + 0.12f * (current->nox_index / 15.0f)
                         + 0.20f * (pressure_ratio - 0.8f));

    lgr = (vri - 34.0f) / 18.0f;
    if (lgr < 0.0f) {
        lgr = 0.0f;
    }

    horizon = (DFG_CRITICAL_SERVICE_VRI - vri) / fmaxf(lgr + 0.5f, 0.6f);
    if (horizon > DFG_MAX_SERVICE_LOADS) {
        horizon = DFG_MAX_SERVICE_LOADS;
    }
    if (horizon < 0.0f) {
        horizon = 0.0f;
    }

    metrics->vent_resistance_index = vri;
    metrics->cycle_efficiency_score = ces;
    metrics->backdraft_suspicion_score = bss;
    metrics->lint_growth_rate = lgr;
    metrics->service_horizon_loads = horizon;
    metrics->dryness_transition_minutes = dryness_minutes;
    metrics->confidence = 86.0f + 8.0f * baseline->valid;

    current->alerts = DFG_ALERT_NONE;
    if (vri >= DFG_RECOMMENDED_SERVICE_VRI) {
        current->alerts |= DFG_ALERT_SERVICE_SOON;
    }
    if (vri >= DFG_CRITICAL_SERVICE_VRI) {
        current->alerts |= DFG_ALERT_SERVICE_NOW;
        current->alerts |= DFG_ALERT_FLOW_RESTRICTED;
    }
    if (dryness_ratio > 1.25f) {
        current->alerts |= DFG_ALERT_DRYING_SLOW;
    }
    if (current->exhaust_temp_c > DFG_MAX_EXHAUST_TEMP_C) {
        current->alerts |= DFG_ALERT_OVERHEAT;
    }
    if (bss > 45.0f || current->co_ppm > DFG_SAFE_CO_PPM) {
        current->alerts |= DFG_ALERT_BACKDRAFT_RISK;
    }

    if (count > 0U) {
        dfg_sensor_frame_t *mutable_history = (dfg_sensor_frame_t *)history;
        mutable_history[count - 1].alerts = current->alerts;
    }
}
