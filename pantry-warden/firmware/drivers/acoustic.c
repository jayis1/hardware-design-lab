/*
 * Pantry Warden acoustic driver simulation
 * Author: jayis1
 */

#include <math.h>

#include "acoustic.h"

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

void acoustic_init(pw_acoustic_driver_t *driver, pw_acoustic_frame_t *frame)
{
    driver->baseline_airborne = 12.0f;
    driver->baseline_structure = 9.0f;

    frame->wingbeat_score = 3.0f;
    frame->chew_score = 2.5f;
    frame->structure_energy = driver->baseline_structure;
    frame->airborne_energy = driver->baseline_airborne;
    frame->transient_count = 0U;
}

static float night_wingbeat(unsigned tick)
{
    if (tick >= 37U && tick <= 45U) {
        return 34.0f + ((float)(tick - 37U) * 4.8f);
    }
    return 0.0f;
}

static float chew_event(unsigned tick)
{
    if (tick >= 39U && tick <= 45U) {
        return 18.0f + sinf((float)(tick - 39U) * 0.9f) * 10.0f;
    }
    return 0.0f;
}

void acoustic_sample(pw_acoustic_driver_t *driver,
                     pw_acoustic_frame_t *frame,
                     const pw_shelf_frame_t *shelf,
                     pw_mode_t mode,
                     unsigned tick)
{
    const float sweep_gain = (mode == PW_MODE_NIGHT_SWEEP) ? 8.0f : 0.0f;
    const float quiet_drop = (mode == PW_MODE_QUIET) ? -2.0f : 0.0f;
    const float wingbeat = night_wingbeat(tick);
    const float chew = chew_event(tick);
    const float disturbances = shelf->disturbance_score * 0.42f;

    frame->airborne_energy = clampf(driver->baseline_airborne + sweep_gain + quiet_drop + sinf((float)tick * 0.27f) * 1.8f + wingbeat * 0.20f,
                                    3.0f,
                                    55.0f);
    frame->structure_energy = clampf(driver->baseline_structure + disturbances * 0.25f + chew * 0.60f + cosf((float)tick * 0.18f) * 1.5f,
                                     2.0f,
                                     72.0f);
    frame->wingbeat_score = clampf(wingbeat + frame->airborne_energy * 0.55f + sweep_gain,
                                   0.0f,
                                   100.0f);
    frame->chew_score = clampf(chew + frame->structure_energy * 0.65f,
                               0.0f,
                               100.0f);
    frame->transient_count = (unsigned)(shelf->disturbance_score / 8.0f) + ((tick == 8U || tick == 9U) ? 2U : 0U);
}
