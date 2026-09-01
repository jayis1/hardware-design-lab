/*
 * CordCanary power driver simulation
 * Author: jayis1
 */

#include "power.h"

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

void power_init(cc_power_frame_t *frame)
{
    frame->battery_pct = 96.0f;
    frame->battery_v = 4.12f;
    frame->rail_v = 3.30f;
    frame->estimated_runtime_h = 280.0f;
    frame->usb_present = false;
    frame->charging = false;
}

void power_step(cc_power_frame_t *frame, const cc_current_frame_t *current, const cc_inference_t *inf, unsigned tick)
{
    float drain = 0.18f;
    if (current->load_present) {
        drain += 0.06f;
    }
    if (inf->state != CC_STATE_NOMINAL) {
        drain += 0.04f;
    }
    if (inf->urgent_unplug) {
        drain += 0.03f;
    }

    if (tick == 0U) {
        frame->usb_present = false;
        frame->charging = false;
    }

    frame->battery_pct = clampf(frame->battery_pct - drain, 5.0f, 100.0f);
    frame->battery_v = 3.55f + frame->battery_pct * 0.0062f;
    frame->rail_v = 3.30f;
    frame->estimated_runtime_h = frame->battery_pct * 2.7f - current->rms_current_a * 0.2f;
    if (frame->estimated_runtime_h < 8.0f) {
        frame->estimated_runtime_h = 8.0f;
    }
}
