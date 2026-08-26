/*
 * PipeWhisper flow inference driver
 * Author: jayis1
 */
#include <math.h>
#include "flow.h"

static float clampf(float value, float minimum, float maximum)
{
    if (value < minimum) return minimum;
    if (value > maximum) return maximum;
    return value;
}

void flow_init(flow_frame_t *frame, pipe_profile_t profile)
{
    frame->draw_estimate_lpm = 0.0f;
    frame->drip_confidence = 0.0f;
    frame->steady_flow_confidence = 0.05f;
    frame->fixture_similarity_sink = 0.66f + 0.05f * (float)profile;
    frame->fixture_similarity_washer = 0.18f;
    frame->fixture_similarity_icemaker = 0.14f;
    frame->signature_drift = 0.06f;
}

void flow_update(flow_frame_t *frame, const acoustic_frame_t *acoustic, const pressure_frame_t *pressure, uint32_t minute)
{
    float draw_core = acoustic->acoustic_rms * 18.0f + pressure->strain_peak * 2.2f;
    frame->draw_estimate_lpm = clampf(draw_core - 1.8f, 0.0f, 21.0f);
    frame->drip_confidence = clampf((acoustic->drip_period_s > 0.1f ? 0.45f : 0.0f) + acoustic->texture * 0.2f + (frame->draw_estimate_lpm < 1.2f ? 0.22f : 0.0f), 0.0f, 1.0f);
    frame->steady_flow_confidence = clampf(acoustic->acoustic_rms * 1.9f + pressure->strain_peak * 0.35f - frame->drip_confidence * 0.4f, 0.0f, 1.0f);
    frame->fixture_similarity_sink = clampf(0.72f + 0.08f * sinf((float)minute * 0.31f) - acoustic->chatter_index * 0.22f, 0.0f, 1.0f);
    frame->fixture_similarity_washer = clampf(0.18f + acoustic->chatter_index * 0.95f + pressure->hammer_score * 0.22f, 0.0f, 1.0f);
    frame->fixture_similarity_icemaker = clampf(0.20f + frame->drip_confidence * 0.55f + 0.04f * cosf((float)minute * 0.73f), 0.0f, 1.0f);
    frame->signature_drift = clampf(fabsf(frame->fixture_similarity_sink - 0.72f) + acoustic->chatter_index * 0.22f + pressure->hammer_score * 0.16f + (minute > 20u ? 0.14f : 0.0f), 0.0f, 1.0f);
}

const char *flow_fixture_label(const flow_frame_t *frame)
{
    if (frame->fixture_similarity_washer >= frame->fixture_similarity_sink && frame->fixture_similarity_washer >= frame->fixture_similarity_icemaker) return "washer-or-solenoid";
    if (frame->fixture_similarity_icemaker >= frame->fixture_similarity_sink) return "drip-or-small-valve";
    return "sink-or-general-draw";
}
