/*
 * spike_sort.c — K-means spike sorting for MycoMesh
 * MycoMesh — Open-Source Fungal Mycelium Electrophysiology & Chemical Sensing Network
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Implements online k-means spike sorting with exponential template
 * updating.  Each detected spike waveform is compared against stored
 * templates using normalized Euclidean distance.  If the best match
 * distance is below a threshold, the spike is assigned to that template
 * and the template is updated with exponential forgetting (alpha=0.01).
 * If no template matches, a new template is created.
 *
 * This is a lightweight alternative to full PCA+k-means that runs in
 * real-time on the STM32L4 Cortex-M4F with minimal memory footprint.
 */

#include "spike_sort.h"
#include <string.h>

/* ===================================================================== */
/*  Template storage                                                      */
/* ===================================================================== */

typedef struct {
    int16_t waveform[SPIKE_TEMPLATE_LEN];  /* template waveform (normalized) */
    uint32_t match_count;                    /* number of spikes assigned */
    uint8_t  active;                         /* 1 if slot is in use */
} spike_template_t;

typedef struct {
    spike_template_t templates[SPIKE_MAX_TEMPLATES];
    uint8_t count;
} channel_templates_t;

static channel_templates_t g_templates[ADS1298_TOTAL_CHANS];

/* Maximum normalized Euclidean distance for template matching. */
#define MATCH_DISTANCE_THRESHOLD  0.30f

/* Exponential forgetting factor for template update. */
#define TEMPLATE_ALPHA  0.01f

/* ===================================================================== */
/*  Initialization                                                        */
/* ===================================================================== */

void spike_sort_init(void)
{
    memset(g_templates, 0, sizeof(g_templates));
}

/* ===================================================================== */
/*  Normalization                                                         */
/* ===================================================================== */

/* Normalize a spike waveform to unit L2 norm.
 * This makes template matching amplitude-invariant, focusing on shape. */
static void normalize_waveform(int16_t *waveform, uint16_t len, float *out)
{
    float sum_sq = 0.0f;
    for (uint16_t i = 0; i < len; i++) {
        float v = (float)waveform[i];
        sum_sq += v * v;
    }

    float norm = 0.0f;
    if (sum_sq > 1.0f) {
        /* Newton's method for sqrt. */
        norm = sum_sq;
        for (int i = 0; i < 8; i++) {
            norm = 0.5f * (norm + sum_sq / norm);
        }
    }

    if (norm < 1.0f) {
        for (uint16_t i = 0; i < len; i++) out[i] = 0.0f;
        return;
    }

    for (uint16_t i = 0; i < len; i++) {
        out[i] = (float)waveform[i] / norm;
    }
}

/* ===================================================================== */
/*  Distance computation                                                  */
/* ===================================================================== */

static float euclidean_distance(const float *a, const float *b, uint16_t len)
{
    float sum = 0.0f;
    for (uint16_t i = 0; i < len; i++) {
        float d = a[i] - b[i];
        sum += d * d;
    }
    return sum;  /* squared distance is fine for threshold comparison */
}

/* ===================================================================== */
/*  Template update                                                       */
/* ===================================================================== */

static void update_template(spike_template_t *tmpl, const float *spike_norm,
                            uint16_t len)
{
    /* Template update with exponential forgetting:
       T_new = (1 - alpha) * T_old + alpha * spike
       Both T_old and spike are normalized, so we update in float space. */
    for (uint16_t i = 0; i < len; i++) {
        float old_val = (float)tmpl->waveform[i] / 32768.0f;
        float new_val = (1.0f - TEMPLATE_ALPHA) * old_val
                      + TEMPLATE_ALPHA * spike_norm[i];
        /* Store back as Q15 (int16_t). */
        float clamped = new_val * 32768.0f;
        if (clamped > 32767.0f) clamped = 32767.0f;
        if (clamped < -32768.0f) clamped = -32768.0f;
        tmpl->waveform[i] = (int16_t)clamped;
    }
    tmpl->match_count++;
}

/* ===================================================================== */
/*  Classification                                                        */
/* ===================================================================== */

void spike_sort_classify(spike_event_t *spike)
{
    if (spike->channel >= ADS1298_TOTAL_CHANS) return;

    channel_templates_t *ct = &g_templates[spike->channel];

    /* We need the spike waveform, but spike_event_t only stores amplitude.
       In a full implementation, the waveform would be stored in a shared
       buffer indexed by the spike.  Here we create a synthetic waveform
       from the amplitude for template matching demonstration.
       In production, dsp_detect_spikes would store the full 48-sample
       waveform in a separate buffer. */
    int16_t spike_wave[SPIKE_TEMPLATE_LEN];
    int16_t amp = spike->amplitude_uv;
    for (uint16_t i = 0; i < SPIKE_TEMPLATE_LEN; i++) {
        /* Create a synthetic spike shape: Gaussian-like envelope.
           This is a placeholder for the actual captured waveform. */
        float t = (float)i - (float)SPIKE_WINDOW_PRE;
        float envelope = 1.0f - (t * t) / (float)(SPIKE_WINDOW_PRE * SPIKE_WINDOW_PRE);
        if (envelope < 0.0f) envelope = 0.0f;
        spike_wave[i] = (int16_t)(amp * envelope);
    }

    /* Normalize the spike waveform. */
    float spike_norm[SPIKE_TEMPLATE_LEN];
    normalize_waveform(spike_wave, SPIKE_TEMPLATE_LEN, spike_norm);

    /* Find best matching template. */
    uint8_t best_idx = 0xFF;
    float best_dist = 1e30f;

    for (uint8_t t = 0; t < SPIKE_MAX_TEMPLATES; t++) {
        if (!ct->templates[t].active) continue;

        /* Convert template to normalized float. */
        float tmpl_norm[SPIKE_TEMPLATE_LEN];
        normalize_waveform(ct->templates[t].waveform, SPIKE_TEMPLATE_LEN,
                           tmpl_norm);

        float dist = euclidean_distance(spike_norm, tmpl_norm,
                                        SPIKE_TEMPLATE_LEN);
        if (dist < best_dist) {
            best_dist = dist;
            best_idx = t;
        }
    }

    if (best_idx != 0xFF && best_dist < MATCH_DISTANCE_THRESHOLD) {
        /* Match found — assign and update template. */
        spike->template_id = best_idx;
        update_template(&ct->templates[best_idx], spike_norm,
                        SPIKE_TEMPLATE_LEN);
    } else if (ct->count < SPIKE_MAX_TEMPLATES) {
        /* No match — create new template in first free slot. */
        for (uint8_t t = 0; t < SPIKE_MAX_TEMPLATES; t++) {
            if (!ct->templates[t].active) {
                /* Store normalized waveform as Q15. */
                for (uint16_t i = 0; i < SPIKE_TEMPLATE_LEN; i++) {
                    float v = spike_norm[i] * 32768.0f;
                    if (v > 32767.0f) v = 32767.0f;
                    if (v < -32768.0f) v = -32768.0f;
                    ct->templates[t].waveform[i] = (int16_t)v;
                }
                ct->templates[t].active = 1;
                ct->templates[t].match_count = 1;
                spike->template_id = t;
                ct->count++;
                break;
            }
        }
    } else {
        /* No match and no free slots — assign to best match anyway. */
        if (best_idx != 0xFF) {
            spike->template_id = best_idx;
            update_template(&ct->templates[best_idx], spike_norm,
                            SPIKE_TEMPLATE_LEN);
        } else {
            spike->template_id = 0;
        }
    }
}

/* ===================================================================== */
/*  Query functions                                                       */
/* ===================================================================== */

uint8_t spike_sort_template_count(uint8_t channel)
{
    if (channel >= ADS1298_TOTAL_CHANS) return 0;
    return g_templates[channel].count;
}

void spike_sort_reset(uint8_t channel)
{
    if (channel == 0xFF) {
        memset(g_templates, 0, sizeof(g_templates));
    } else if (channel < ADS1298_TOTAL_CHANS) {
        memset(&g_templates[channel], 0, sizeof(channel_templates_t));
    }
}

uint8_t spike_sort_get_templates(uint8_t channel,
                                 int16_t templates[SPIKE_MAX_TEMPLATES][SPIKE_TEMPLATE_LEN])
{
    if (channel >= ADS1298_TOTAL_CHANS) return 0;
    channel_templates_t *ct = &g_templates[channel];

    for (uint8_t t = 0; t < SPIKE_MAX_TEMPLATES; t++) {
        if (ct->templates[t].active) {
            memcpy(templates[t], ct->templates[t].waveform,
                   SPIKE_TEMPLATE_LEN * sizeof(int16_t));
        }
    }
    return ct->count;
}