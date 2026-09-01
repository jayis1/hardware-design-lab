/*
 * CordCanary motion driver simulation
 * Author: jayis1
 */

#include <math.h>

#include "motion.h"

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

void motion_init(cc_motion_frame_t *frame)
{
    frame->vibration_rms_g = 0.01f;
    frame->orientation_deg = 90.0f;
    frame->wobble_score = 0.03f;
    frame->drop_events = 0U;
    frame->recently_moved = false;
}

void motion_sample(cc_motion_frame_t *frame, const cc_strain_frame_t *strain, cc_mode_t mode, unsigned tick)
{
    float vibration = 0.015f + fabsf(sinf((float) tick * 0.24f)) * 0.01f;
    float orientation = 90.0f + sinf((float) tick * 0.1f) * 2.0f;
    float wobble = 0.04f + strain->pull_force_n * 0.015f;
    unsigned drops = frame->drop_events;
    bool moved = false;

    if (tick >= 13U && tick <= 18U) {
        vibration = 0.08f;
        wobble = 0.24f + (float) (tick - 13U) * 0.04f;
        moved = true;
    }

    if (tick == 20U) {
        drops += 1U;
        moved = true;
    }

    if (mode == CC_MODE_WORKSHOP && tick >= 25U && tick <= 31U) {
        vibration = 0.12f;
        wobble = 0.28f;
        orientation = 100.0f;
        moved = true;
    }

    if (tick >= 32U) {
        vibration = 0.14f;
        wobble = 0.51f;
        moved = true;
    }

    frame->vibration_rms_g = clampf(vibration, 0.0f, 2.0f);
    frame->orientation_deg = clampf(orientation, 0.0f, 180.0f);
    frame->wobble_score = clampf(wobble, 0.0f, 1.0f);
    frame->drop_events = drops;
    frame->recently_moved = moved;
}
