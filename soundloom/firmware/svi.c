/*
 * svi.c — Soil Vitality Index computation
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 *
 * SoundLoom bioacoustic soil health monitor.
 * Computes the composite Soil Vitality Index (SVI) — a 0–100 score
 * derived from acoustic event rates, spectral diversity, environmental
 * context, and compaction/noise penalties.
 */

#include "svi.h"
#include "dsp.h"
#include "classifier.h"
#include "board.h"
#include <math.h>
#include <string.h>

/* SVI weights (learned from calibration data linking acoustic features
 * to biological measurements: earthworm counts, microbial biomass, root depth)
 */
static const float W_ROOT      = 0.20f;
static const float W_WORM      = 0.25f;
static const float W_MICROBE   = 0.15f;
static const float W_DIVERSITY = 0.20f;
static const float W_COMPACT   = 0.12f;  /* penalty */
static const float W_NOISE     = 0.08f;  /* penalty */

/* Running averages for normalisation */
static float baseline_rates[CLS_NUM_CLASSES];
static float svi_history[64];
static uint32_t svi_hist_idx;
static uint8_t  svi_initialised = 0;

void svi_init(void)
{
    memset(baseline_rates, 0, sizeof(baseline_rates));
    memset(svi_history, 0, sizeof(svi_history));
    svi_hist_idx = 0;
    svi_initialised = 1;
}

static float sigmoidf(float x) {
    return 1.0f / (1.0f + expf(-x));
}

static float clampf(float v, float lo, float hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

uint8_t svi_compute(const float event_rates[CLS_NUM_CLASSES],
                    float spectral_diversity,
                    float noise_floor,
                    const env_data_t *env)
{
    if (!svi_initialised) svi_init();

    /* Normalise event rates against running baseline (z-score) */
    float root_rate   = event_rates[0] + event_rates[1];  /* root_growth + root_hydraulic */
    float worm_rate   = event_rates[2] + event_rates[3];  /* earthworm_burrow + cast */
    float microbe_rate = event_rates[6];                  /* microbe_ebullition */
    float arthropod    = event_rates[4];                 /* arthropod_click */
    float compaction   = event_rates[8];                 /* compaction_crack */
    float pest_rate    = event_rates[5];                 /* grub_chew */
    float noise_rate   = event_rates[9];                 /* noise/unclassified */

    /* Update baselines (slow EMA) */
    float alpha = 0.01f;
    for (int i = 0; i < CLS_NUM_CLASSES; i++) {
        baseline_rates[i] = baseline_rates[i] * (1.0f - alpha) + event_rates[i] * alpha;
    }

    /* Normalised components (0–1 scale) */
    float n_root    = clampf(root_rate / (baseline_rates[0] + baseline_rates[1] + 0.001f), 0.0f, 2.0f) * 0.5f;
    float n_worm    = clampf(worm_rate / (baseline_rates[2] + baseline_rates[3] + 0.001f), 0.0f, 2.0f) * 0.5f;
    float n_microbe = clampf(microbe_rate / (baseline_rates[6] + 0.001f), 0.0f, 2.0f) * 0.5f;
    float n_div     = clampf(spectral_diversity, 0.0f, 1.0f);

    /* Penalties (higher = worse) */
    float p_compact = clampf(compaction / (baseline_rates[8] + 0.001f), 0.0f, 2.0f) * 0.5f;
    float p_noise   = clampf(noise_rate / (baseline_rates[9] + 0.001f), 0.0f, 2.0f) * 0.5f;

    /* Moisture modulator: very dry or very wet soil suppresses biological activity
     * and reduces the meaningfulness of acoustic rates. Optimal moisture: 20–40% VWC */
    float moisture = (env && (env->moisture[0] > 0.0f)) ? env->moisture[0] : 30.0f;
    float moisture_mod;
    if (moisture < 10.0f) moisture_mod = 0.6f;
    else if (moisture > 60.0f) moisture_mod = 0.7f;
    else moisture_mod = 1.0f - fabsf(moisture - 30.0f) / 60.0f;
    moisture_mod = clampf(moisture_mod, 0.5f, 1.0f);

    /* Temperature modulator: below 5 °C biological activity slows */
    float temp = (env && (env->depth_temp[0] > -40.0f)) ? env->depth_temp[0] : 15.0f;
    float temp_mod;
    if (temp < 5.0f) temp_mod = 0.4f;
    else if (temp < 10.0f) temp_mod = 0.7f;
    else temp_mod = 1.0f;

    /* Composite score (before sigmoid) */
    float score = W_ROOT     * n_root
                + W_WORM     * n_worm
                + W_MICROBE  * n_microbe
                + W_DIVERSITY * n_div
                - W_COMPACT  * p_compact
                - W_NOISE     * p_noise;

    /* Apply environmental modulators */
    score *= moisture_mod * temp_mod;

    /* Scale to 0–100 via sigmoid centred at 0.5 */
    float svi_float = 100.0f * sigmoidf((score - 0.5f) * 4.0f);

    /* Pest alert penalty: if grub_chew rate is high, reduce SVI */
    if (pest_rate > baseline_rates[5] * 2.0f + 0.1f) {
        svi_float *= 0.85f;
    }

    /* Store in history */
    svi_history[svi_hist_idx % 64] = svi_float;
    svi_hist_idx++;

    /* Smooth with 5-sample moving average */
    if (svi_hist_idx >= 5) {
        float avg = 0.0f;
        int start = (int)svi_hist_idx - 5;
        if (start < 0) start = 0;
        int n = (int)svi_hist_idx - start;
        for (int i = start; i < (int)svi_hist_idx; i++) {
            avg += svi_history[i % 64];
        }
        svi_float = avg / (float)n;
    }

    return (uint8_t)clampf(svi_float, 0.0f, 100.0f);
}

float svi_trend(void)
{
    /* Compute simple linear trend over history (slope) */
    if (svi_hist_idx < 2) return 0.0f;
    uint32_t n = (svi_hist_idx < 64) ? svi_hist_idx : 64;
    float sum_x = 0.0f, sum_y = 0.0f, sum_xy = 0.0f, sum_x2 = 0.0f;
    for (uint32_t i = 0; i < n; i++) {
        float x = (float)i;
        float y = svi_history[i];
        sum_x += x; sum_y += y; sum_xy += x * y; sum_x2 += x * x;
    }
    float denom = (float)n * sum_x2 - sum_x * sum_x;
    if (fabsf(denom) < 1e-9f) return 0.0f;
    return ((float)n * sum_xy - sum_x * sum_y) / denom;  /* slope = SVI/sample */
}

uint16_t svi_get_flags(const float event_rates[CLS_NUM_CLASSES],
                       uint16_t battery_mv)
{
    uint16_t flags = 0;
    /* Pest alert: grub_chew rate exceeds 2× baseline */
    if (event_rates[5] > baseline_rates[5] * 2.0f + 0.1f) {
        flags |= SVI_FLAG_PEST;
    }
    /* Compaction alert */
    if (event_rates[8] > baseline_rates[8] * 2.0f + 0.1f) {
        flags |= SVI_FLAG_COMPACTION;
    }
    /* Low battery: < 3.3 V */
    if (battery_mv < 3300) {
        flags |= SVI_FLAG_LOW_BATTERY;
    }
    return flags;
}