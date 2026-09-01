/*
 * CordCanary current driver simulation
 * Author: jayis1
 */

#include <math.h>

#include "current.h"

static float clampf(float value, float low, float high)
{
    if (value < low) {
        return low;
    }
    if (value > high) {
        return high;
    }
    return value;
}

void current_init(cc_current_frame_t *frame)
{
    frame->rms_current_a = 0.2f;
    frame->crest_factor = 1.15f;
    frame->hf_noise_score = 0.05f;
    frame->leakage_ma = 0.2f;
    frame->estimated_power_w = 24.0f;
    frame->transient_density = 0.02f;
    frame->load_present = false;
}

void current_sample(cc_current_frame_t *frame, const cc_thermal_frame_t *thermal, cc_mode_t mode, unsigned tick)
{
    float rms = 0.18f + fabsf(sinf((float) tick * 0.17f)) * 0.35f;
    float crest = 1.2f + fabsf(cosf((float) tick * 0.11f)) * 0.08f;
    float noise = 0.06f + fabsf(sinf((float) tick * 0.09f)) * 0.04f;
    float leakage = 0.18f + thermal->humidity_pct * 0.002f;
    float transients = 0.03f + fabsf(cosf((float) tick * 0.23f)) * 0.02f;

    if (tick >= 6U && tick <= 12U) {
        rms = 11.4f + (float) (tick - 6U) * 0.45f;
        crest = 1.32f;
        noise = 0.12f;
        transients = 0.09f;
    }

    if (tick >= 13U && tick <= 18U) {
        rms = 8.6f;
        crest = 1.52f;
        noise = 0.26f + (float) (tick - 13U) * 0.03f;
        transients = 0.16f;
    }

    if (tick >= 19U && tick <= 24U) {
        rms = 9.1f;
        crest = 1.27f;
        noise = 0.11f;
        transients = 0.05f;
    }

    if (mode == CC_MODE_GARAGE && tick >= 25U && tick <= 31U) {
        rms = 2.1f;
        crest = 1.36f;
        noise = 0.18f;
        leakage += 1.2f + (float) (tick - 25U) * 0.18f;
        transients = 0.08f;
    }

    if (tick >= 32U) {
        rms = 6.8f + fabsf(sinf((float) tick * 0.4f)) * 1.4f;
        crest = 1.88f;
        noise = 0.58f + fabsf(cosf((float) tick * 0.25f)) * 0.12f;
        leakage += 0.4f;
        transients = 0.31f;
    }

    frame->rms_current_a = clampf(rms, 0.0f, 16.0f);
    frame->crest_factor = clampf(crest, 1.0f, 3.0f);
    frame->hf_noise_score = clampf(noise, 0.0f, 1.0f);
    frame->leakage_ma = clampf(leakage, 0.0f, 8.0f);
    frame->estimated_power_w = frame->rms_current_a * 118.0f * (0.84f + frame->hf_noise_score * 0.08f);
    frame->transient_density = clampf(transients, 0.0f, 1.0f);
    frame->load_present = frame->rms_current_a > 0.6f;
}
