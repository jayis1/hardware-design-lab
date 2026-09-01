/*
 * CordCanary strain driver simulation
 * Author: jayis1
 */

#include <math.h>

#include "strain.h"

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

void strain_init(cc_strain_frame_t *frame)
{
    frame->bend_radius_mm = 46.0f;
    frame->pull_force_n = 0.5f;
    frame->torsion_deg = 2.0f;
    frame->fatigue_index = 0.04f;
    frame->clipped_securely = true;
}

void strain_sample(cc_strain_frame_t *frame, cc_mode_t mode, unsigned tick)
{
    float bend = 44.0f + sinf((float) tick * 0.15f) * 3.0f;
    float pull = 0.6f + fabsf(cosf((float) tick * 0.21f)) * 0.6f;
    float torsion = sinf((float) tick * 0.2f) * 5.0f;
    float fatigue = 0.05f + fabsf(sinf((float) tick * 0.08f)) * 0.03f;
    bool secure = true;

    if (tick >= 19U && tick <= 24U) {
        bend = 18.0f - (float) (tick - 19U) * 1.2f;
        pull = 4.1f + (float) (tick - 19U) * 0.35f;
        torsion = 18.0f;
        fatigue = 0.44f + (float) (tick - 19U) * 0.05f;
    }

    if (mode == CC_MODE_RV && tick >= 25U && tick <= 29U) {
        bend = 16.0f;
        pull = 5.0f;
        torsion = 22.0f;
        fatigue = 0.66f;
        secure = false;
    }

    if (tick >= 32U) {
        bend = 22.0f;
        pull = 2.8f;
        torsion = 10.0f + sinf((float) tick * 0.33f) * 2.0f;
        fatigue = 0.38f;
        secure = true;
    }

    frame->bend_radius_mm = clampf(bend, 8.0f, 90.0f);
    frame->pull_force_n = clampf(pull, 0.0f, 15.0f);
    frame->torsion_deg = clampf(torsion, -30.0f, 30.0f);
    frame->fatigue_index = clampf(fatigue, 0.0f, 1.0f);
    frame->clipped_securely = secure;
}
