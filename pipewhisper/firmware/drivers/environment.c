/*
 * PipeWhisper environment driver
 * Author: jayis1
 */
#include <math.h>
#include "environment.h"

static float clampf(float value, float minimum, float maximum)
{
    if (value < minimum) return minimum;
    if (value > maximum) return maximum;
    return value;
}

void environment_init(environment_frame_t *frame, pipe_profile_t profile)
{
    frame->surface_temp_c = profile == PIPE_PROFILE_LAUNDRY_HOT ? 27.5f : 21.2f;
    frame->ambient_temp_c = 22.0f;
    frame->humidity_rh = 46.0f;
    frame->dew_risk = 0.12f;
    frame->freeze_slope_cph = -0.1f;
    frame->condensation_risk = 0.09f;
}

void environment_sample(environment_frame_t *frame, pipe_profile_t profile, uint32_t minute, const flow_frame_t *flow)
{
    float t = (float)minute;
    float cool_down = minute >= 20u ? -0.22f * (float)(minute - 19u) : 0.0f;
    float hot_usage = (profile == PIPE_PROFILE_LAUNDRY_HOT && minute >= 4u && minute <= 7u) ? 5.2f : 0.0f;
    float active_draw = flow->draw_estimate_lpm * 0.28f;
    frame->ambient_temp_c = 22.1f + 0.4f * sinf(t * 0.17f) + (minute >= 22u ? -1.8f : 0.0f);
    frame->humidity_rh = 45.0f + 2.2f * sinf(t * 0.23f) + (minute >= 18u ? 6.0f : 0.0f);
    frame->surface_temp_c = 20.8f + hot_usage + active_draw + cool_down + 0.5f * cosf(t * 0.12f);
    frame->dew_risk = clampf((frame->humidity_rh - 48.0f) / 35.0f + (frame->surface_temp_c < frame->ambient_temp_c ? 0.15f : 0.0f), 0.0f, 1.0f);
    frame->freeze_slope_cph = -0.12f + cool_down * 0.8f - (active_draw > 1.0f ? -0.2f : 0.0f);
    frame->condensation_risk = clampf(frame->dew_risk * 0.78f + (frame->surface_temp_c + 2.0f < frame->ambient_temp_c ? 0.18f : 0.0f), 0.0f, 1.0f);
}
