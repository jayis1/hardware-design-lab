/*
 * PipeWhisper pressure and impulse driver
 * Author: jayis1
 */
#include <math.h>
#include "pressure.h"

static float clampf(float value, float minimum, float maximum)
{
    if (value < minimum) return minimum;
    if (value > maximum) return maximum;
    return value;
}

void pressure_init(pressure_frame_t *frame, pipe_profile_t profile)
{
    frame->hammer_score = 0.08f + (float)profile * 0.02f;
    frame->impulse_count = 0.0f;
    frame->ring_decay_ms = 24.0f;
    frame->burst_risk = 0.02f;
    frame->strain_peak = 0.18f;
}

void pressure_sample(pressure_frame_t *frame, pipe_profile_t profile, uint32_t minute, const acoustic_frame_t *acoustic)
{
    float t = (float)minute;
    float hammer_event = 0.0f;
    float usage_event = 0.0f;
    float drip_event = 0.0f;
    (void)profile;
    if (minute >= 9u && minute <= 12u) hammer_event = 0.34f + 0.03f * cosf(t * 2.2f);
    if (minute >= 4u && minute <= 7u) usage_event = 0.18f;
    if (minute >= 18u) drip_event = 0.06f;
    frame->strain_peak = clampf(0.16f + usage_event + hammer_event * 0.8f + drip_event + acoustic->acoustic_rms * 0.25f, 0.0f, 1.0f);
    frame->hammer_score = clampf(0.07f + hammer_event * 1.85f + acoustic->chatter_index * 0.24f, 0.0f, 1.0f);
    frame->impulse_count = 1.0f + hammer_event * 16.0f + usage_event * 5.0f + 0.8f * fabsf(sinf(t));
    frame->ring_decay_ms = 21.0f + hammer_event * 98.0f + usage_event * 24.0f + drip_event * 9.0f;
    frame->burst_risk = clampf(frame->hammer_score * 0.7f + frame->ring_decay_ms / 220.0f - 0.08f, 0.0f, 1.0f);
}
