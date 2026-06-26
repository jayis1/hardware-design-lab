/*
 * seismology.c — STA/LTA trigger, one-bit cross-correlation, event classifier
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 *
 * Implements a recursive STA/LTA trigger (classic algorithm, Allen 1978)
 * and a one-bit normalized cross-correlation classifier that compares the
 * incoming sign-bit stream against a small library of pre-loaded templates
 * (basal slide, crevasse snap, calving thump, subglacial tremor).
 */

#include "seismology.h"
#include "board.h"
#include <string.h>
#include <math.h>

/* ---- State --------------------------------------------------------------- */
static float    g_sta[3]   = {0,0,0};   /* short-term average per component */
static float    g_lta[3]   = {0,0,0};   /* long-term average per component */
static uint32_t g_sta_n    = 0;         /* STA sample count */
static uint32_t g_lta_n    = 0;         /* LTA sample count */
static float    g_threshold = STA_LTA_THRESHOLD;

#define STA_N   100     /* 0.5 s @ 200 SPS */
#define LTA_N   2000    /* 10  s @ 200 SPS */

static event_meta_t g_last_event;
static bool         g_event_pending = false;

/* ---- Circular pre-trigger ring (5 s @ 200 SPS × 3 ch × 4 bytes) --------- */
#define PRE_TRIG_NSAMPLES  (int)(EVENT_PRE_TRIGGER_S * ADC_SAMPLE_RATE_HZ)
#define POST_TRIG_NSAMPLES (int)(EVENT_POST_TRIGGER_S * ADC_SAMPLE_RATE_HZ)
static float    g_ring_z [PRE_TRIG_NSAMPLES];
static float    g_ring_ns[PRE_TRIG_NSAMPLES];
static float    g_ring_ew[PRE_TRIG_NSAMPLES];
static uint32_t g_ring_ts[PRE_TRIG_NSAMPLES];
static int      g_ring_head = 0;

/* Post-trigger capture buffer */
static float    g_post_z [POST_TRIG_NSAMPLES];
static float    g_post_ns[POST_TRIG_NSAMPLES];
static float    g_post_ew[POST_TRIG_NSAMPLES];
static int      g_post_count = 0;
static bool     g_capturing = false;

/* ---- Template library ---------------------------------------------------- */
static template_t g_templates[EVENT_TEMPLATES_MAX];
static uint8_t    g_template_count = 0;

/* ---- Built-in default templates (synthetic, replaced in field) ---------- */
static const int8_t tmpl_basal[64] = {
    1, 1, 1, 1, 1, 1, 1, 1, -1,-1,-1,-1,-1,-1,-1,-1,
    1, 1, 1, 1, 1, 1, 1, 1, -1,-1,-1,-1,-1,-1,-1,-1,
    1, 1, 1, 1, 1, 1, 1, 1, -1,-1,-1,-1,-1,-1,-1,-1,
    1, 1, 1, 1, 1, 1, 1, 1, -1,-1,-1,-1,-1,-1,-1,-1
};
static const int8_t tmpl_crevasse[64] = {
    -1, 1,-1, 1,-1, 1,-1, 1,-1, 1,-1, 1,-1, 1,-1, 1,
     1,-1, 1,-1, 1,-1, 1,-1, 1,-1, 1,-1, 1,-1, 1,-1,
    -1, 1,-1, 1,-1, 1,-1, 1,-1, 1,-1, 1,-1, 1,-1, 1,
     1,-1, 1,-1, 1,-1, 1,-1, 1,-1, 1,-1, 1,-1, 1,-1
};
static const int8_t tmpl_calving[64] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
   -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
   -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
};

/* ---- Magnitude estimation (very rough, calibrated per-site) ------------- */
static float estimate_magnitude(float sta_lta_max)
{
    /* Empirical: Mv ≈ log2(sta_lta) - 1.0, clipped to [-3, 5] */
    float m = logf(sta_lta_max) / logf(2.0f) - 1.0f;
    if (m < -3.0f) m = -3.0f;
    if (m >  5.0f) m =  5.0f;
    return m;
}

/* ---- One-bit cross-correlation (popcount-style on M4) ------------------- */
static float onebit_xcorr(const int8_t *a, const int8_t *b, int n)
{
    int agree = 0;
    int i;
    for (i = 0; i < n; i++) {
        if (a[i] == b[i]) agree++;
    }
    return (float)agree / (float)n;
}

uint8_t seismo_classify(const int8_t *sign_stream, uint16_t n)
{
    uint8_t best = 0;
    float   best_score = 0.0f;
    uint8_t i;
    int     cmp_len = (n < EVENT_TEMPLATE_LEN) ? n : EVENT_TEMPLATE_LEN;
    if (cmp_len > 64) cmp_len = 64;   /* built-in templates are 64 long */

    for (i = 0; i < g_template_count; i++) {
        float s = onebit_xcorr(sign_stream, g_templates[i].sign, cmp_len);
        if (s > best_score) {
            best_score = s;
            best = i;
        }
    }
    g_last_event.correl_score  = best_score;
    g_last_event.template_id   = best;
    return best;
}

/* ---- Recursive STA/LTA --------------------------------------------------- */
static float recursive_sta_lta(float x, float *sta, float *lta,
                                uint32_t *n_sta, uint32_t *n_lta)
{
    /* STA: sliding 0.5 s window, recursive IIR approximation */
    if (*n_sta < STA_N) {
        *sta = *sta + (x - *sta) / (float)(*n_sta + 1);
        (*n_sta)++;
    } else {
        *sta = *sta + (x - *sta) / (float)STA_N;
    }
    /* LTA: 10 s window, recursive */
    if (*n_lta < LTA_N) {
        *lta = *lta + (x - *lta) / (float)(*n_lta + 1);
        (*n_lta)++;
    } else {
        *lta = *lta + (x - *lta) / (float)LTA_N;
    }
    if (fabsf(*lta) < 1e-9f) return 0.0f;
    return fabsf(*sta) / fabsf(*lta);
}

/* ---- Ring buffer push ---------------------------------------------------- */
static void ring_push(float z, float ns, float ew, uint32_t ts)
{
    g_ring_z [g_ring_head] = z;
    g_ring_ns[g_ring_head] = ns;
    g_ring_ew[g_ring_head] = ew;
    g_ring_ts[g_ring_head] = ts;
    g_ring_head = (g_ring_head + 1) % PRE_TRIG_NSAMPLES;
}

void seismo_frame_to_signbits(adc_frame_t *f, int8_t *out_z,
                              int8_t *out_ns, int8_t *out_ew)
{
    uint16_t i;
    for (i = 0; i < f->len; i++) {
        out_z [i] = (f->samples[i].volts[0] >= 0) ? 1 : -1;
        out_ns[i] = (f->samples[i].volts[1] >= 0) ? 1 : -1;
        out_ew[i] = (f->samples[i].volts[2] >= 0) ? 1 : -1;
    }
}

/* ---- Main per-frame processor ------------------------------------------- */
bool seismo_process_frame(adc_frame_t *f)
{
    uint16_t i;
    float    ratio[3] = {0,0,0};
    float    ratio_max = 0;
    bool     triggered = false;

    for (i = 0; i < f->len; i++) {
        float z  = f->samples[i].volts[0];
        float ns = f->samples[i].volts[1];
        float ew = f->samples[i].volts[2];
        uint32_t ts = f->samples[i].ts_us;

        ratio[0] = recursive_sta_lta(z,  &g_sta[0], &g_lta[0], &g_sta_n, &g_lta_n);
        /* Per-channel STA/LTA would need separate state; share LTA for simplicity */
        ratio[1] = fabsf(ns) / (fabsf(g_lta[0]) + 1e-9f);
        ratio[2] = fabsf(ew) / (fabsf(g_lta[0]) + 1e-9f);

        for (int c = 0; c < 3; c++) {
            if (ratio[c] > ratio_max) ratio_max = ratio[c];
        }

        /* Push to ring buffer (pre-trigger cache) */
        ring_push(z, ns, ew, ts);

        /* If currently capturing post-trigger, append */
        if (g_capturing) {
            if (g_post_count < POST_TRIG_NSAMPLES) {
                g_post_z [g_post_count] = z;
                g_post_ns[g_post_count] = ns;
                g_post_ew[g_post_count] = ew;
                g_post_count++;
            } else {
                /* Capture complete — finalize event */
                g_capturing = false;
                g_event_pending = true;
            }
        }

        /* Trigger check */
        if (!g_capturing && ratio_max > g_threshold) {
            triggered = true;
            g_capturing = true;
            g_post_count = 0;
            g_last_event.ts_utc_ms       = ts / 1000;
            g_last_event.sta_lta_ratio[0] = ratio[0];
            g_last_event.sta_lta_ratio[1] = ratio[1];
            g_last_event.sta_lta_ratio[2] = ratio[2];
            g_last_event.magnitude_est   = estimate_magnitude(ratio_max);
            g_last_event.sample_count    = PRE_TRIG_NSAMPLES + POST_TRIG_NSAMPLES;
        }
    }
    return triggered;
}

event_meta_t *seismo_last_event(void)
{
    if (g_event_pending) return &g_last_event;
    return NULL;
}

void seismo_add_template(const template_t *t)
{
    if (g_template_count < EVENT_TEMPLATES_MAX) {
        memcpy(&g_templates[g_template_count], t, sizeof(template_t));
        g_template_count++;
    }
}

void seismo_set_threshold(float sta_lta) { g_threshold = sta_lta; }
float seismo_get_threshold(void)         { return g_threshold; }

void seismo_init(void)
{
    memset(g_sta, 0, sizeof(g_sta));
    memset(g_lta, 0, sizeof(g_lta));
    g_sta_n = g_lta_n = 0;
    g_ring_head = 0;
    g_post_count = 0;
    g_capturing = false;
    g_event_pending = false;
    g_threshold = STA_LTA_THRESHOLD;
    g_template_count = 0;

    /* Load default templates */
    template_t t;
    memset(&t, 0, sizeof(t));

    memcpy(t.sign, tmpl_basal,    sizeof(tmpl_basal));    t.type = EVT_BASAL_SLIDE;       t.name = "basal";
    seismo_add_template(&t);
    memcpy(t.sign, tmpl_crevasse, sizeof(tmpl_crevasse)); t.type = EVT_CREVASSE_SNAP;     t.name = "crevasse";
    seismo_add_template(&t);
    memcpy(t.sign, tmpl_calving,  sizeof(tmpl_calving));  t.type = EVT_CALVING_THUMP;     t.name = "calving";
    seismo_add_template(&t);
}

/* ---- Seal event into a flat packet for LoRa ------------------------------ */
void seismo_seal_event(uint8_t *out_buf, uint32_t *out_len, uint32_t max_len)
{
    /* Header (16 B) + pre-trigger (3 × 1000 × 2 B = 6000 B) +
       post-trigger (3 × 1000 × 2 B = 6000 B) + meta (16 B) = ~12 KB.
       LZ4 compression (see lz4.c) reduces ~3-5× before LoRa fragmentation. */
    uint32_t off = 0;
    int i;

    if (max_len < 32) { *out_len = 0; return; }

    /* Header */
    out_buf[off++] = 0xA5;             /* sync byte */
    out_buf[off++] = MESH_MY_NODE_ID;
    out_buf[off++] = (g_last_event.ts_utc_ms >> 24) & 0xFF;
    out_buf[off++] = (g_last_event.ts_utc_ms >> 16) & 0xFF;
    out_buf[off++] = (g_last_event.ts_utc_ms >>  8) & 0xFF;
    out_buf[off++] = (g_last_event.ts_utc_ms >>  0) & 0xFF;
    out_buf[off++] = (uint8_t)g_last_event.type;
    /* Float magnitude → 4 bytes */
    memcpy(&out_buf[off], &g_last_event.magnitude_est, 4); off += 4;
    memcpy(&out_buf[off], &g_last_event.correl_score,  4); off += 4;
    out_buf[off++] = g_last_event.template_id;
    out_buf[off++] = 0; /* reserved */

    /* Append pre-trigger ring (downsampled to 16-bit) */
    for (i = 0; i < PRE_TRIG_NSAMPLES && off + 6 < max_len; i++) {
        int idx = (g_ring_head + i) % PRE_TRIG_NSAMPLES;
        int16_t sz  = (int16_t)(g_ring_z [idx] * 10000.0f);
        int16_t sns = (int16_t)(g_ring_ns[idx] * 10000.0f);
        int16_t sew = (int16_t)(g_ring_ew[idx] * 10000.0f);
        memcpy(&out_buf[off], &sz,  2); off += 2;
        memcpy(&out_buf[off], &sns, 2); off += 2;
        memcpy(&out_buf[off], &sew, 2); off += 2;
    }
    /* Append post-trigger buffer */
    for (i = 0; i < g_post_count && off + 6 < max_len; i++) {
        int16_t sz  = (int16_t)(g_post_z [i] * 10000.0f);
        int16_t sns = (int16_t)(g_post_ns[i] * 10000.0f);
        int16_t sew = (int16_t)(g_post_ew[i] * 10000.0f);
        memcpy(&out_buf[off], &sz,  2); off += 2;
        memcpy(&out_buf[off], &sns, 2); off += 2;
        memcpy(&out_buf[off], &sew, 2); off += 2;
    }

    *out_len = off;
    g_event_pending = false;
}