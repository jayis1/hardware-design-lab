/*
 * PipeWhisper inference engine
 * Author: jayis1
 */
#include <math.h>
#include "inference.h"

static float clampf(float value, float minimum, float maximum)
{
    if (value < minimum) return minimum;
    if (value > maximum) return maximum;
    return value;
}

void inference_init(inference_frame_t *frame)
{
    frame->leak_confidence = 0.05f;
    frame->freeze_risk = 0.07f;
    frame->install_quality = 0.92f;
    frame->maintenance_priority = 0.08f;
    frame->appliance_drift = 0.06f;
    frame->health_index = 96.0f;
    frame->alert = ALERT_NONE;
}

void inference_update(inference_frame_t *frame, const pipe_snapshot_t *current, const pipe_snapshot_t *previous)
{
    float drip_component = current->flow.drip_confidence * 0.55f;
    float quiet_usage_component = current->flow.draw_estimate_lpm < 1.0f ? 0.16f : 0.0f;
    float humidity_component = current->environment.humidity_rh > 50.0f ? 0.10f : 0.0f;
    float hammer_component = current->pressure.hammer_score * 0.6f + current->pressure.burst_risk * 0.2f;
    float thermal_component = current->environment.surface_temp_c < 10.0f ? 0.35f : 0.0f;
    float slope_component = current->environment.freeze_slope_cph < -1.8f ? 0.26f : 0.0f;
    frame->leak_confidence = clampf(drip_component + quiet_usage_component + humidity_component + current->acoustic.impulsiveness * 0.08f, 0.0f, 1.0f);
    frame->freeze_risk = clampf(thermal_component + slope_component + current->environment.condensation_risk * 0.22f + (current->flow.draw_estimate_lpm < 0.3f ? 0.06f : 0.0f), 0.0f, 1.0f);
    frame->install_quality = clampf(0.94f - fabsf(current->acoustic.dominant_hz - previous->acoustic.dominant_hz) / 1200.0f - current->flow.signature_drift * 0.12f, 0.0f, 1.0f);
    frame->appliance_drift = clampf(current->flow.signature_drift * 0.72f + hammer_component * 0.16f, 0.0f, 1.0f);
    frame->maintenance_priority = clampf(frame->leak_confidence * 0.44f + frame->freeze_risk * 0.24f + hammer_component * 0.28f + frame->appliance_drift * 0.16f, 0.0f, 1.0f);
    frame->health_index = clampf(100.0f - frame->leak_confidence * 34.0f - frame->freeze_risk * 28.0f - hammer_component * 19.0f - frame->appliance_drift * 11.0f, 0.0f, 100.0f);
    if (frame->freeze_risk > 0.70f || frame->leak_confidence > 0.82f) frame->alert = ALERT_CRITICAL;
    else if (frame->maintenance_priority > 0.65f) frame->alert = ALERT_WARNING;
    else if (frame->maintenance_priority > 0.42f) frame->alert = ALERT_CAUTION;
    else if (frame->maintenance_priority > 0.22f) frame->alert = ALERT_INFO;
    else frame->alert = ALERT_NONE;
}

const char *inference_primary_reason(const pipe_snapshot_t *snapshot)
{
    if (snapshot->inference.freeze_risk >= snapshot->inference.leak_confidence && snapshot->inference.freeze_risk > 0.45f) return "freeze-risk";
    if (snapshot->pressure.hammer_score > 0.50f) return "hammer-severity";
    if (snapshot->inference.leak_confidence > 0.35f) return "persistent-drip";
    return "baseline-observation";
}
