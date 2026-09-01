/*
 * CordCanary thermal driver simulation
 * Author: jayis1
 */

#include <math.h>

#include "thermal.h"

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

static float base_ambient_for_mode(cc_mode_t mode)
{
    switch (mode) {
    case CC_MODE_GARAGE:
        return 18.5f;
    case CC_MODE_RV:
        return 27.0f;
    case CC_MODE_WORKSHOP:
        return 23.5f;
    case CC_MODE_HOME:
    default:
        return 22.0f;
    }
}

void thermal_init(cc_thermal_frame_t *frame)
{
    frame->ambient_c = 22.0f;
    frame->humidity_pct = 45.0f;
    frame->dew_margin_c = 11.0f;
    frame->shell_temp_c = 23.1f;
    frame->plug_face_temp_c = 24.0f;
    frame->cord_neck_temp_c = 23.5f;
    frame->hotspot_delta_c = 0.5f;
    frame->rise_rate_cpm = 0.0f;
}

void thermal_sample(cc_thermal_frame_t *frame, cc_mode_t mode, unsigned tick)
{
    const float phase = (float) tick * 0.23f;
    const float ambient = base_ambient_for_mode(mode) + sinf(phase) * 0.8f;
    float humidity = 42.0f + cosf((float) tick * 0.19f) * 6.0f;
    float shell = ambient + 1.1f;
    float plug = shell + 0.8f;
    float cord = shell + 0.4f;
    float rate = 0.1f + fabsf(sinf((float) tick * 0.12f)) * 0.25f;

    if (tick >= 6U && tick <= 12U) {
        plug += 3.5f + (float) (tick - 6U) * 0.8f;
        cord += 1.8f + (float) (tick - 6U) * 0.25f;
        shell += 1.5f + (float) (tick - 6U) * 0.2f;
        rate += 0.7f;
    }

    if (tick >= 13U && tick <= 18U) {
        plug += 7.0f + (float) (18U - tick) * 0.35f;
        cord += 2.2f;
        shell += 2.0f;
        rate += 0.9f;
    }

    if (tick >= 19U && tick <= 24U) {
        cord += 5.2f;
        plug += 3.2f;
        shell += 2.6f;
        rate += 0.55f;
    }

    if (mode == CC_MODE_GARAGE && tick >= 25U && tick <= 31U) {
        humidity += 26.0f;
        shell -= 0.4f;
        plug += 1.2f;
        cord += 0.8f;
        rate += 0.3f;
    }

    if (tick >= 32U) {
        plug += 4.8f;
        cord += 2.9f;
        shell += 2.4f;
        rate += 1.25f;
    }

    frame->ambient_c = ambient;
    frame->humidity_pct = clampf(humidity, 25.0f, 92.0f);
    frame->shell_temp_c = shell;
    frame->plug_face_temp_c = plug;
    frame->cord_neck_temp_c = cord;
    frame->hotspot_delta_c = plug - cord;
    frame->dew_margin_c = clampf(18.0f - frame->humidity_pct * 0.13f - (ambient - 18.0f) * 0.1f, 1.5f, 15.0f);
    frame->rise_rate_cpm = rate;
}
