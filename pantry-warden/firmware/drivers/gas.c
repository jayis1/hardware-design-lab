/*
 * Pantry Warden gas driver simulation
 * Author: jayis1
 */

#include <math.h>

#include "gas.h"

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

static float drift_wave(unsigned tick, float rate, float phase)
{
    return sinf(((float)tick * rate) + phase);
}

void gas_init(pw_gas_driver_t *driver, pw_gas_frame_t *frame)
{
    driver->baseline_temp_c = 22.4f;
    driver->baseline_humidity_pct = 49.0f;
    driver->baseline_co2_ppm = 508.0f;
    driver->baseline_voc_index = 11.0f;
    driver->fan_bias = 18.0f;

    frame->temp_c = driver->baseline_temp_c;
    frame->humidity_pct = driver->baseline_humidity_pct;
    frame->co2_ppm = driver->baseline_co2_ppm;
    frame->voc_index = driver->baseline_voc_index;
    frame->ethanol_ppm = 0.9f;
    frame->stale_air_index = 8.0f;
    frame->fan_duty_pct = driver->fan_bias;
    frame->dew_margin_c = 7.0f;
}

static float humidity_event(unsigned tick)
{
    if (tick >= 14U && tick <= 22U) {
        return 14.0f + ((float)(tick - 14U) * 1.7f);
    }
    if (tick > 22U && tick <= 27U) {
        return 28.0f - ((float)(tick - 22U) * 2.0f);
    }
    return 0.0f;
}

static float voc_event(unsigned tick)
{
    if (tick >= 28U && tick <= 34U) {
        return 10.0f + ((float)(tick - 28U) * 4.6f);
    }
    if (tick > 34U && tick <= 40U) {
        return 37.6f - ((float)(tick - 34U) * 2.4f);
    }
    return 0.0f;
}

void gas_sample(pw_gas_driver_t *driver,
                pw_gas_frame_t *frame,
                pw_mode_t mode,
                unsigned tick)
{
    const float quiet_bias = (mode == PW_MODE_QUIET) ? -2.5f : 0.0f;
    const float sweep_bias = (mode == PW_MODE_NIGHT_SWEEP) ? 9.0f : 0.0f;
    const float cleanout_bias = (mode == PW_MODE_CLEANOUT) ? 12.0f : 0.0f;
    const float humidity_spike = humidity_event(tick);
    const float voc_spike = voc_event(tick);
    const float thermal_wave = drift_wave(tick, 0.20f, 0.1f) * 0.55f;
    const float stale_wave = drift_wave(tick, 0.16f, 1.7f) * 18.0f;

    frame->temp_c = driver->baseline_temp_c + thermal_wave + (humidity_spike * 0.04f);
    frame->humidity_pct = clampf(driver->baseline_humidity_pct + humidity_spike + drift_wave(tick, 0.12f, 0.3f) * 2.4f,
                                 35.0f,
                                 92.0f);
    frame->co2_ppm = clampf(driver->baseline_co2_ppm + 35.0f + stale_wave + (voc_spike * 2.8f) - sweep_bias,
                            420.0f,
                            980.0f);
    frame->voc_index = clampf(driver->baseline_voc_index + voc_spike + (humidity_spike * 0.18f) + drift_wave(tick, 0.22f, 0.8f) * 1.8f,
                              6.0f,
                              80.0f);
    frame->ethanol_ppm = clampf(0.8f + (frame->voc_index * 0.11f) + (voc_spike * 0.09f), 0.5f, 11.0f);
    frame->fan_duty_pct = clampf(driver->fan_bias + quiet_bias + sweep_bias + cleanout_bias + (frame->humidity_pct > 70.0f ? 10.0f : 0.0f),
                                 8.0f,
                                 62.0f);
    frame->stale_air_index = clampf((frame->co2_ppm - 400.0f) / 8.5f + (frame->humidity_pct - 45.0f) * 0.7f - (frame->fan_duty_pct - 18.0f) * 0.4f,
                                    0.0f,
                                    100.0f);
    frame->dew_margin_c = clampf(11.0f - ((frame->humidity_pct - 45.0f) * 0.18f) - fabsf(thermal_wave) * 0.5f,
                                 0.5f,
                                 12.0f);
}

float gas_spoilage_lift(const pw_gas_frame_t *frame)
{
    const float voc_component = fmaxf(0.0f, frame->voc_index - 18.0f) * 1.8f;
    const float ethanol_component = fmaxf(0.0f, frame->ethanol_ppm - 2.0f) * 4.5f;
    const float humidity_component = fmaxf(0.0f, frame->humidity_pct - 62.0f) * 0.9f;
    return clampf(voc_component + ethanol_component + humidity_component, 0.0f, 100.0f);
}
