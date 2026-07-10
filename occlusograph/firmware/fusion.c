/*
 * fusion.c — Complementary-filter force fusion for Occlusograph.
 *
 * Author: jayis1
 * Copyright © 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 *
 * The piezoelectric (PVDF) array is an excellent dynamic force sensor but has
 * essentially zero DC response — a constant bite produces no sustained charge.
 * The capacitive array measures steady-state pressure but is bandwidth-limited
 * (~100 Hz). By complementary-filtering the two we get a per-element force
 * estimate that is flat from DC to ~2 kHz.
 *
 * The filter is a classic two-band split:
 *
 *   F_fused = F_cap_hp + F_piezo_hp
 *
 * where F_cap_hp is the capacitive signal high-passed at f_cap (so it carries
 * the DC + low-frequency content) and F_piezo_hp is the piezo signal high-passed
 * at f_piezo (so it carries the mid/high-frequency content). Because the piezo
 * path is inherently high-passed by the charge amp's reset, f_piezo is set to
 * match the charge-amp's effective corner (~0.1 Hz). The capacitive path is
 * low-passed at f_cap (~50 Hz) to reject its own quantization noise above the
 * mechanical bandwidth of tooth contact.
 *
 * Implementation uses a one-pole IIR per element — cheap enough to run 64 of
 * them at 1 kHz on a Cortex-M33.
 */

#include "fusion.h"
#include "board.h"
#include "registers.h"
#include <string.h>
#include <math.h>

/* ---- Filter state ---- */
typedef struct {
    float cap_state[CAP_NUM_ELEMENTS];   /* low-pass state per cap element */
    float piezo_state[PIEZO_NUM_ELEMENTS]; /* high-pass state per piezo element */
    float last_cap[PIEZO_NUM_ELEMENTS];   /* last cap value (zero-order-held) */
} fusion_state_t;

static fusion_state_t g_state;
static float g_piezo_alpha_hp = 0.999f;  /* HP coefficient, set by cutoff */
static float g_cap_alpha_lp   = 0.0307f; /* LP coefficient */
static float g_piezo_gain[PIEZO_NUM_ELEMENTS];
static float g_cap_gain[CAP_NUM_ELEMENTS];

/* Convert 24-bit CDC code to pF: full-scale ±4.096 pF / 0x7FFFFF.
 * Then scale to mN using a per-element stiffness coefficient (mN/pF).
 */
#define CAP_CODE_TO_PF(code)  ((float)(code) * AD7746_FS_PF / (float)AD7746_CODE_MAX)

/* Convert 12-bit signed ADC count to charge (pC): charge amp gain maps
 * 1 V/pC, ADC is 1.8 V / 4096. The gain factor below is tuned at calibration.
 */
#define PIEZO_ADC_TO_PC(count, gain) ((float)(count) * 0.00044f * (gain))

void fusion_init(void)
{
    memset(&g_state, 0, sizeof(g_state));
    for (int i = 0; i < PIEZO_NUM_ELEMENTS; i++) g_piezo_gain[i] = 1.0f;
    for (int i = 0; i < CAP_NUM_ELEMENTS;  i++) g_cap_gain[i]   = 1.0f;
    fusion_set_cutoffs(0.1f, 50.0f);
}

void fusion_set_cutoffs(float piezo_hp_hz, float cap_lp_hz)
{
    /* One-pole HP: y[n] = alpha * (y[n-1] + x[n] - x[n-1]); alpha = RC/(RC+dt).
     * We only need the coefficient; the state stores the previous output.
     */
    float dt = 1.0f / (float)SAMPLE_RATE_HZ;
    float rc_p = 1.0f / (2.0f * 3.14159265f * piezo_hp_hz);
    g_piezo_alpha_hp = rc_p / (rc_p + dt);
    float rc_c = 1.0f / (2.0f * 3.14159265f * cap_lp_hz);
    g_cap_alpha_lp = dt / (rc_c + dt);
}

void fusion_set_gains(const float piezo_gain[PIEZO_NUM_ELEMENTS],
                      const float cap_gain[CAP_NUM_ELEMENTS])
{
    if (piezo_gain) memcpy(g_piezo_gain, piezo_gain, sizeof(g_piezo_gain));
    if (cap_gain)   memcpy(g_cap_gain,   cap_gain,   sizeof(g_cap_gain));
}

void fusion_update(const sensor_frame_t *raw, force_frame_t *out)
{
    if (!raw || !out) return;

    out->timestamp = raw->timestamp;

    /* Copy IMU through (calibration handled downstream). */
    out->imu_ax = raw->imu[0]; out->imu_ay = raw->imu[1]; out->imu_az = raw->imu[2];
    out->imu_gx = raw->imu[3]; out->imu_gy = raw->imu[4]; out->imu_gz = raw->imu[5];

    /* For each piezo element, run the complementary filter. The capacitive
     * array has 32 elements while piezo has 64, so we map each cap element to
     * two piezo elements (interleaved). Cap index = piezo_index / 2.
     */
    for (int i = 0; i < PIEZO_NUM_ELEMENTS; i++) {
        /* Piezo: convert raw count to charge (pC) then to mN via gain. */
        float p_pc = PIEZO_ADC_TO_PC(raw->piezo[i], g_piezo_gain[i]);
        float p_mN = p_pc * 1000.0f;   /* 1 pC ≈ 1 mN at nominal sensitivity */

        /* High-pass the piezo signal. */
        float prev = g_state.piezo_state[i];
        float hp = g_piezo_alpha_hp * (prev + p_mN - g_state.last_cap[i]);
        g_state.piezo_state[i] = hp;

        /* Capacitive: only valid on every 10th frame; zero-order-hold otherwise. */
        int cap_idx = i / 2;
        float cap_mN = 0.0f;
        if (raw->cap[cap_idx] != 0) {
            float pf = CAP_CODE_TO_PF(raw->cap[cap_idx]);
            cap_mN = pf * g_cap_gain[cap_idx] * 1000.0f;  /* pF -> mN via stiffness */
            g_state.last_cap[i] = cap_mN;
        } else {
            cap_mN = g_state.last_cap[i];
        }

        /* Low-pass the capacitive signal. */
        float lp = g_state.cap_state[cap_idx] + g_cap_alpha_lp *
                   (cap_mN - g_state.cap_state[cap_idx]);
        g_state.cap_state[cap_idx] = lp;

        /* Fused force = low-passed cap + high-passed piezo. */
        float fused = lp + hp;
        /* Clamp to sensor range and store as int16 mN. */
        int16_t mN = (int16_t)CLAMP(fused, -32000.0f, 32000.0f);
        out->force_mN[i] = mN;
    }
}