/*
 * Pantry Warden shelf driver simulation
 * Author: jayis1
 */

#include <math.h>

#include "shelf.h"

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

void shelf_init(pw_shelf_driver_t *driver, pw_shelf_frame_t *frame)
{
    driver->baseline_mass_kg = 6.20f;
    driver->baseline_gap_mm = 68.0f;
    driver->baseline_freshness_pct = 93.0f;

    frame->total_mass_kg = driver->baseline_mass_kg;
    frame->left_mass_kg = 3.10f;
    frame->right_mass_kg = 3.10f;
    frame->front_gap_mm = driver->baseline_gap_mm;
    frame->optical_freshness_pct = driver->baseline_freshness_pct;
    frame->moisture_strip_pct = 14.0f;
    frame->package_tilt_deg = 0.8f;
    frame->disturbance_score = 4.0f;
}

static float restock_mass_event(unsigned tick)
{
    if (tick >= 8U && tick <= 10U) {
        return ((float)(tick - 7U)) * 0.38f;
    }
    return (tick > 10U) ? 1.14f : 0.0f;
}

static float spoilage_bulge_event(unsigned tick)
{
    if (tick >= 29U && tick <= 35U) {
        return ((float)(tick - 28U)) * 1.8f;
    }
    if (tick > 35U) {
        return 12.6f;
    }
    return 0.0f;
}

static float pest_disturbance_event(unsigned tick)
{
    if (tick >= 36U && tick <= 44U) {
        return 10.0f + sinf((float)(tick - 36U) * 0.8f) * 7.0f;
    }
    return 0.0f;
}

void shelf_sample(pw_shelf_driver_t *driver,
                  pw_shelf_frame_t *frame,
                  const pw_gas_frame_t *gas,
                  pw_mode_t mode,
                  unsigned tick)
{
    const float restock_gain = restock_mass_event(tick);
    const float bulge = spoilage_bulge_event(tick);
    const float pest_disturbance = pest_disturbance_event(tick);
    const float moisture_follow = fmaxf(0.0f, gas->humidity_pct - 55.0f) * 1.5f;
    const float cleanout_shift = (mode == PW_MODE_CLEANOUT) ? -0.42f : 0.0f;
    const float optical_penalty = (bulge * 0.7f) + (moisture_follow * 0.12f);

    frame->total_mass_kg = driver->baseline_mass_kg + restock_gain + cleanout_shift + sinf((float)tick * 0.06f) * 0.03f;
    frame->left_mass_kg = frame->total_mass_kg * (0.51f + sinf((float)tick * 0.13f) * 0.02f);
    frame->right_mass_kg = frame->total_mass_kg - frame->left_mass_kg;
    frame->front_gap_mm = clampf(driver->baseline_gap_mm - (restock_gain * 3.2f) - bulge + sinf((float)tick * 0.11f) * 0.6f,
                                 42.0f,
                                 75.0f);
    frame->optical_freshness_pct = clampf(driver->baseline_freshness_pct - optical_penalty + cosf((float)tick * 0.09f) * 0.7f,
                                          32.0f,
                                          96.0f);
    frame->moisture_strip_pct = clampf(14.0f + moisture_follow + (tick >= 16U && tick <= 24U ? 12.0f : 0.0f),
                                       8.0f,
                                       88.0f);
    frame->package_tilt_deg = clampf(0.8f + (bulge * 0.22f) + sinf((float)tick * 0.2f) * 0.4f,
                                     -2.0f,
                                     9.0f);
    frame->disturbance_score = clampf(4.0f + pest_disturbance + (mode == PW_MODE_NIGHT_SWEEP ? 5.5f : 0.0f),
                                      0.0f,
                                      100.0f);
}

float shelf_mass_delta(const pw_shelf_driver_t *driver, const pw_shelf_frame_t *frame)
{
    return frame->total_mass_kg - driver->baseline_mass_kg;
}
