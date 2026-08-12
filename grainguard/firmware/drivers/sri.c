/*
 * sri.c — Spoilage Risk Index fusion
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 *
 * Fuses five sensor signals into a single 0-100 Spoilage Risk Index:
 *
 *   CO2 trend/absolute     35 %
 *   Temperature gradient   25 %  (max zone delta-T)
 *   Temperature absolute   15 %  (above safe storage temp)
 *   EMC above safe MC      15 %
 *   Acoustic insect events 10 %
 *
 * Each sub-score is 0..N (its weight), and the total is the sum.
 */

#include "sri.h"
#include "../board.h"
#include "emc.h"

static uint8_t clamp_u8(int32_t v, uint8_t lo, uint8_t hi) {
    if (v < lo) return lo;
    if (v > hi) return (uint8_t)v;
    return (uint8_t)v;
}

void sri_compute(sri_result_t *s,
                 const co2_meas_t *co2,
                 const temp_profile_t *tp,
                 const humid_meas_t *hm,
                 const acoustic_result_t *ae,
                 uint8_t grain_type,
                 int16_t safe_mc_x1000,
                 uint8_t caution_thresh,
                 uint8_t critical_thresh)
{
    /* Clear */
    s->co2_contribution = 0;
    s->temp_grad_contribution = 0;
    s->temp_abs_contribution = 0;
    s->emc_contribution = 0;
    s->acoustic_contribution = 0;

    /* ---- CO2 contribution (max 35) ---- */
    /* < 600 ppm: 0; 600-1000: linear 0-15; 1000-2000: 15-28; 2000-5000: 28-35 */
    uint16_t co2_ppm = co2->co2_ppm;
    if (co2_ppm > 2000) {
        s->co2_contribution = 28 + clamp_u8((co2_ppm - 2000) * 7 / 3000, 0, 7);
    } else if (co2_ppm > 1000) {
        s->co2_contribution = 15 + (uint8_t)((co2_ppm - 1000) * 13 / 1000);
    } else if (co2_ppm > 600) {
        s->co2_contribution = (uint8_t)((co2_ppm - 600) * 15 / 400);
    }

    /* ---- Temperature gradient (max 25) ---- */
    /* delta_x10 is ×10 C. >2 C gradient (20) = full 25. */
    int16_t delta = tp->delta_x10;
    if (delta > 50) {
        s->temp_grad_contribution = 25;
    } else if (delta > 20) {
        s->temp_grad_contribution = (uint8_t)((delta - 20) * 25 / 30);
    } else if (delta > 5) {
        s->temp_grad_contribution = (uint8_t)((delta - 5) * 5 / 15);
    }

    /* ---- Temperature absolute (max 15) ---- */
    /* >30 C (temperate) or >25 C (tropical): full. Linear above 15 C baseline. */
    if (tp->max_zone >= 0) {
        int16_t tmax = tp->celsius_x10[tp->max_zone];  /* ×10 C */
        if (tmax > 300) {
            s->temp_abs_contribution = 15;
        } else if (tmax > 150) {
            s->temp_abs_contribution = (uint8_t)((tmax - 150) * 15 / 150);
        }
    }

    /* ---- EMC contribution (max 15) ---- */
    if (safe_mc_x1000 > 0 && grain_type >= 1 && grain_type <= GRAIN_COUNT) {
        int32_t emc = emc_compute(grain_type, hm->temperature_x100 / 10,
                                  hm->humidity_x100);
        int32_t over = emc - safe_mc_x1000;   /* ×1000 */
        if (over > 0) {
            /* Full 15 at +2% MC over safe; linear below */
            int32_t score = over * 15 / 2000;
            s->emc_contribution = clamp_u8(score, 0, 15);
        }
    }

    /* ---- Acoustic contribution (max 10) ---- */
    /* >10 events/min: start. Full at 50 events/min. */
    if (ae->events_per_min > 0) {
        int32_t score = (int32_t)ae->events_per_min * 10 / 50;
        s->acoustic_contribution = clamp_u8(score, 0, 10);
        /* If species identified with high confidence, bump to max */
        if (ae->species != INSECT_NONE && ae->confidence_pct > 70) {
            s->acoustic_contribution = 10;
        }
    }

    /* ---- Total ---- */
    int32_t total = (int32_t)s->co2_contribution
                  + (int32_t)s->temp_grad_contribution
                  + (int32_t)s->temp_abs_contribution
                  + (int32_t)s->emc_contribution
                  + (int32_t)s->acoustic_contribution;
    s->sri = clamp_u8(total, 0, 100);

    /* ---- Alert level ---- */
    s->alert_level = 0;
    if (s->sri >= critical_thresh)      s->alert_level = 2;
    else if (s->sri >= caution_thresh) s->alert_level = 1;
}