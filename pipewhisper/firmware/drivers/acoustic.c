/*
 * PipeWhisper acoustic driver
 * Author: jayis1
 */
#include <math.h>
#include "acoustic.h"

static float clampf(float value, float minimum, float maximum)
{
    if (value < minimum) return minimum;
    if (value > maximum) return maximum;
    return value;
}

void acoustic_init(acoustic_frame_t *frame, pipe_profile_t profile)
{
    frame->acoustic_rms = 0.12f + (float)profile * 0.03f;
    frame->dominant_hz = 180.0f + (float)profile * 22.0f;
    frame->impulsiveness = 0.18f;
    frame->texture = 0.32f;
    frame->chatter_index = 0.11f;
    frame->drip_period_s = 0.0f;
}

void acoustic_sample(acoustic_frame_t *frame, pipe_profile_t profile, uint32_t minute)
{
    float t = (float)minute;
    float baseline = 0.11f + (float)profile * 0.025f;
    float use_burst = 0.0f;
    float washer_chatter = 0.0f;
    float drip_mode = 0.0f;

    if (minute >= 4u && minute <= 7u) use_burst = 0.22f;
    if (minute >= 9u && minute <= 12u) washer_chatter = 0.18f + 0.02f * sinf(t * 3.0f);
    if (minute >= 18u) drip_mode = 0.09f + 0.01f * cosf(t * 1.8f);

    frame->acoustic_rms = baseline + 0.03f * sinf(t * 0.41f) + use_burst + washer_chatter + drip_mode;
    frame->dominant_hz = 170.0f + 12.0f * sinf(t * 0.5f) + use_burst * 410.0f + washer_chatter * 620.0f + drip_mode * 270.0f;
    frame->impulsiveness = clampf(0.16f + 0.04f * cosf(t * 0.37f) + washer_chatter * 1.6f + drip_mode * 0.8f, 0.0f, 1.0f);
    frame->texture = clampf(0.28f + 0.12f * use_burst + 0.42f * washer_chatter + 0.18f * drip_mode + 0.05f * sinf(t * 0.2f), 0.0f, 1.0f);
    frame->chatter_index = clampf(0.10f + washer_chatter * 2.8f + 0.03f * cosf(t * 0.73f), 0.0f, 1.0f);
    frame->drip_period_s = minute >= 18u ? 5.5f + 0.3f * sinf(t * 0.7f) : 0.0f;
}

const char *acoustic_event_label(const acoustic_frame_t *frame)
{
    if (frame->drip_period_s > 0.0f && frame->acoustic_rms > 0.17f) return "periodic-drip";
    if (frame->chatter_index > 0.42f) return "valve-chatter";
    if (frame->acoustic_rms > 0.28f) return "steady-draw";
    return "quiet-baseline";
}
