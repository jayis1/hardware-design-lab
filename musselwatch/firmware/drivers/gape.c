/*
 * drivers/gape.c — Bivalve shell-gape (valvometric) analysis engine
 *
 * Converts raw Hall-effect ADC readings into inferred shell opening
 * (micrometres), maintains a rolling baseline, computes an activity
 * score (short-term variability), and flags two anomaly classes:
 *
 *   - CLAMP:   sustained closure (gape < 5% of baseline) > 60 s
 *   - GAPE_STALL: gape frozen (stdev < 1 ADC count) > 120 s
 *
 * Both behaviours are ecologically meaningful: freshwater mussels
 * clamp shut in response to toxicants, heavy metals, or sudden pH
 * drops, and stop valve movements when stressed.  The device's value
 * is that it detects these within minutes — far faster than chemical
 * sensors deployed downstream.
 *
 * Magnet model: N42 NdFeB disc, Ø3 x 1 mm, bonded to one valve; the
 * DRV5053 ratiometric linear Hall sensor is fixed to the shell of the
 * other valve ~ 2 mm away at full closure.  Field at sensor
 * B(G) ~ 1500 * (1 / (d + g)^2) for small air gap g (mm), so the ADC
 * reading is approximately linearly proportional to gape over the
 * 0..3 mm range.
 *
 * Author:  jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#include "gape.h"

/* ---- Ring buffer -------------------------------------------------- */

void ring_init(gape_ring_t *r)
{
    r->head = 0u;
    r->count = 0u;
    r->sum = 0u;
    for (uint8_t i = 0; i < GAPE_BUF_LEN; i++) r->buf[i] = 0u;
}

void ring_push(gape_ring_t *r, uint16_t val)
{
    if (r->count == GAPE_BUF_LEN) {
        r->sum -= r->buf[r->head];
    } else {
        r->count++;
    }
    r->buf[r->head] = val;
    r->sum += val;
    r->head = (r->head + 1u) % GAPE_BUF_LEN;
}

uint16_t ring_mean(const gape_ring_t *r)
{
    if (r->count == 0u) return 0u;
    return (uint16_t)(r->sum / r->count);
}

uint16_t ring_stdev(const gape_ring_t *r)
{
    if (r->count < 2u) return 0u;
    uint32_t mean = r->sum / r->count;
    uint64_t var = 0u;
    for (uint8_t i = 0; i < r->count; i++) {
        int32_t d = (int32_t)r->buf[i] - (int32_t)mean;
        var += (uint64_t)(d * d);
    }
    var /= r->count;
    /* Integer sqrt */
    uint32_t s = 0u;
    uint32_t t = 0u;
    for (uint32_t bit = 0x8000u; bit > 0u; bit >>= 1) {
        t = s + bit;
        if (t * t <= var) s = t;
    }
    return (uint16_t)s;
}

/* ---- Per-channel state & per-channel ring buffers ----------------- */

static gape_ring_t g_rings[NUM_CHANNELS];
/* Time (in samples) since each anomaly condition began */
static uint32_t clamp_duration_s[NUM_CHANNELS];
static uint32_t stall_duration_s[NUM_CHANNELS];

void gape_init(channel_state_t *ch)
{
    for (uint8_t i = 0; i < NUM_CHANNELS; i++) {
        ch[i].channel = i;
        ch[i].raw_hall = 0u;
        ch[i].raw_baseline = 2048u;  /* mid-scale default */
        ch[i].gape_um = 0;
        ch[i].activity_score = 0u;
        ch[i].anomaly_flag = 0u;
        ch[i].last_event_s = 0u;
        ring_init(&g_rings[i]);
        clamp_duration_s[i] = 0u;
        stall_duration_s[i] = 0u;
    }
}

/*
 * Calibrate the baseline: record the ADC reading when the shell is
 * fully closed (operator holds mussel shut during calibration).
 */
void gape_calibrate(channel_state_t *ch, uint16_t raw_closed)
{
    ch->raw_baseline = raw_closed;
    /* Reset rolling stats */
    ring_init(&g_rings[ch->channel]);
}

/*
 * Convert raw ADC (12-bit) to inferred shell opening in micrometres.
 *
 * The DRV5053 output at Vcc=3.0V has sensitivity ~ 1.65 mV/G at 0 A,
 * linear over +- 80 G.  The N42 magnet's field at the sensor follows
 * an inverse-square law with gap distance.  Over the 0..3 mm working
 * range, the relationship is approximately linear with slope ~
 * 400 ADC counts / mm (= 0.4 counts / um), and the closed baseline
 * corresponds to the field at d = 2 mm.
 *
 *  gape_um = (raw - baseline) * 2.5  (calibrated constant)
 *
 * Negative values (raw < baseline, i.e., sensor moves closer) clamp to 0.
 */
int16_t gape_raw_to_um(uint16_t raw, uint16_t baseline)
{
    int32_t diff = (int32_t)raw - (int32_t)baseline;
    if (diff < 0) return 0;
    int32_t um = diff * 25 / 10;  /* *2.5 */
    if (um > 4000) um = 4000;  /* saturate at 4 mm */
    return (int16_t)um;
}

/*
 * Process a new raw ADC sample for one channel:
 *   - update gape_um
 *   - push to ring buffer, compute activity score
 *   - evaluate anomaly flags
 */
void gape_update(channel_state_t *ch, uint16_t raw_adc)
{
    uint8_t c = ch->channel;
    ch->raw_hall = raw_adc;
    ch->gape_um = gape_raw_to_um(raw_adc, ch->raw_baseline);

    ring_push(&g_rings[c], raw_adc);

    /* Activity score = normalised short-term stdev (0..100) */
    uint16_t sd = ring_stdev(&g_rings[c]);
    if (sd > 100u) sd = 100u;
    ch->activity_score = (uint8_t)sd;

    /* Anomaly: CLAMP - gape < 5% of typical max (assume baseline+1600) */
    uint32_t nominal_max = (uint32_t)ch->raw_baseline + 1600u;
    uint32_t clamp_thresh = ch->raw_baseline + (nominal_max - ch->raw_baseline) / 20u;
    if (raw_adc < clamp_thresh) {
        clamp_duration_s[c] += SAMPLE_PERIOD_MS / 1000u;
        if (clamp_duration_s[c] > 60u) {
            ch->anomaly_flag |= 0x01u;  /* set CLAMP */
        }
    } else {
        clamp_duration_s[c] = 0u;
        ch->anomaly_flag &= ~0x01u;  /* clear CLAMP */
    }

    /* Anomaly: GAPE_STALL - stdev < 1 ADC count for > 120 s */
    if (sd < 1u) {
        stall_duration_s[c] += SAMPLE_PERIOD_MS / 1000u;
        if (stall_duration_s[c] > 120u) {
            ch->anomaly_flag |= 0x02u;  /* set STALL */
        }
    } else {
        stall_duration_s[c] = 0u;
        ch->anomaly_flag &= ~0x02u;  /* clear STALL */
    }

    if (ch->anomaly_flag) {
        ch->last_event_s = 0u;  /* event is active now */
    } else {
        ch->last_event_s += SAMPLE_PERIOD_MS / 1000u;
    }
}

/*
 * Overall anomaly score for a channel (0..100), combining activity
 * depression and anomaly-flag duration.  Used for gateway triage.
 */
uint8_t gape_anomaly_score(const channel_state_t *ch)
{
    uint8_t score = 0u;
    if (ch->anomaly_flag & 0x01u) score += 50u;  /* clamp is severe */
    if (ch->anomaly_flag & 0x02u) score += 30u;  /* stall is moderate */
    /* Low activity contributes up to 20 */
    if (ch->activity_score < 5u) score += 20u;
    else if (ch->activity_score < 20u) score += 10u;
    if (score > 100u) score = 100u;
    return score;
}