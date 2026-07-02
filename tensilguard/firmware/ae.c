/*
 * ae.c — Acoustic-emission wire-break detection
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 *
 * Continuously samples the piezo contact transducer at 200 kcps via ADC1.
 * A hardware comparator (COMP1) watches the amplified signal and wakes the
 * MCU from STOP2 when the amplitude exceeds the configured threshold. The
 * firmware then captures the pre-trigger ring buffer plus 24 ms of post-
 * trigger, extracts a small feature set (amplitude, rise-time, duration,
 * centroid frequency, hi/lo band power ratio), and runs a decision-tree
 * classifier to label the event as noise / fretting / impact / wire-break.
 *
 * Confirmed wire breaks are time-stamped, the waveform summary is stored to
 * SPI flash, and an urgent uplink is queued. The classifier uses physically-
 * motivated thresholds derived from published AE-on-cable studies; the app
 * exposes the thresholds so installers can tune for a specific structure.
 */
#include <math.h>
#include <string.h>
#include "tensilguard.h"
#include "board.h"
#include "registers.h"
#include "hal.h"

/* ring buffer continuously fed by DMA while MCU sleeps */
static int16_t  s_ringbuf[AE_RINGBUF_NSAMP];
static volatile uint32_t s_ring_head;       /* DMA writes here              */
static int16_t  s_capture[AE_NSAMP];        /* pre+post trigger snapshot     */
static ae_event_t s_last_event;
static uint32_t s_ae_count_since_uplink = 0;
static float    s_threshold_uv = AE_COMP_THRESH_UV;

/* ----------------------------- Prototypes --------------------------------- */
static void extract_features(ae_event_t *ev, const int16_t *buf, uint32_t n);
static uint8_t classify(const ae_event_t *ev);
static void downsample_summary(uint8_t *out, const int16_t *in, uint32_t n);

/* ----------------------------- Public API --------------------------------- */

/*
 * ae_init() — enable the charge amplifier, arm the comparator wake.
 */
void ae_init(void)
{
    gpio_set(PIN_AE_AMP_EN, 1);
    delay_ms(5);
    /* start ADC1 in circular DMA mode at AE_SAMPLE_RATE_HZ into s_ringbuf */
    adc_start_circular(ADC_AE_CH, s_ringbuf, AE_RINGBUF_NSAMP);
    /* arm COMP1 on the AE channel to wake via EXTI1 */
    comp1_arm(s_threshold_uv);
    s_ae_count_since_uplink = 0;
}

/*
 * ae_set_threshold() — adjust comparator threshold (µV at transducer).
 */
void ae_set_threshold(float uv)
{
    s_threshold_uv = uv;
    comp1_arm(uv);
}

/*
 * ae_isr() — comparator-triggered wake handler. Captures the waveform,
 * extracts features, classifies, logs, and queues an urgent uplink if
 * the event is classified as a wire break.
 */
void ae_isr(void)
{
    /* snapshot pre-trigger from ring buffer */
    uint32_t pretrig = (AE_PRETRIG_MS * AE_SAMPLE_RATE_HZ) / 1000UL;
    uint32_t start = (s_ring_head + AE_RINGBUF_NSAMP - pretrig) % AE_RINGBUF_NSAMP;
    for (uint32_t i = 0; i < pretrig; i++) {
        s_capture[i] = s_ringbuf[(start + i) % AE_RINGBUF_NSAMP];
    }
    /* continue sampling post-trigger (blocking, 24 ms) */
    uint32_t posttrig = (AE_POSTTRIG_MS * AE_SAMPLE_RATE_HZ) / 1000UL;
    adc_capture_single(ADC_AE_CH, s_capture + pretrig, posttrig);

    /* build event record */
    s_last_event.unix_time = rtc_get_unix();
    extract_features(&s_last_event, s_capture, AE_NSAMP);
    s_last_event.score = classify(&s_last_event);
    s_last_event.classification =
        (s_last_event.score >= 60) ? AE_BREAK :
        (s_last_event.score >= 30) ? AE_IMPACT :
        (s_last_event.score >= 15) ? AE_FRET  : AE_NOISE;

    downsample_summary(s_last_event.waveform, s_capture, AE_NSAMP);

    /* log every event to flash; queue urgent uplink on confirmed break */
    flash_log_ae(&s_last_event);
    s_ae_count_since_uplink++;

    if (s_last_event.classification == AE_BREAK) {
        g_urgent_remaining = TG_URGENT_WINDOW_S / (g_interval_s ? g_interval_s : 1);
    }
    /* re-arm comparator */
    comp1_arm(s_threshold_uv);
}

uint8_t ae_count_drain(void)
{
    uint8_t c = (uint8_t)s_ae_count_since_uplink;
    s_ae_count_since_uplink = 0;
    return c;
}

const ae_event_t *ae_last_event(void)
{
    return &s_last_event;
}

/* ----------------------------- Internals ---------------------------------- */

/*
 * extract_features — compute the physical features used by the classifier.
 *   peak_uv:    peak absolute amplitude in µV (at transducer)
 *   rise_us:    time from first-cross to peak in µs
 *   dur_us:     time above threshold total in µs
 *   centroid:   spectral centroid in kHz
 *   hi_lo:      partial power ratio hi band (>80 kHz) / lo band (20-80 kHz)
 */
static void extract_features(ae_event_t *ev, const int16_t *buf, uint32_t n)
{
    /* amplitude features */
    float peak = 0.0f;
    int   peak_idx = 0;
    for (uint32_t i = 0; i < n; i++) {
        float v = fabsf((float)buf[i]);
        if (v > peak) { peak = v; peak_idx = (int)i; }
    }
    ev->peak_uv = peak;

    /* first-cross: first sample exceeding 0.4 × peak */
    int first_cross = 0;
    float cross_lvl = 0.4f * peak;
    for (uint32_t i = 0; i < n; i++) {
        if (fabsf((float)buf[i]) > cross_lvl) { first_cross = (int)i; break; }
    }
    ev->rise_us = (float)(peak_idx - first_cross) * 1e6f / (float)AE_SAMPLE_RATE_HZ;
    if (ev->rise_us < 0.0f) ev->rise_us = 0.0f;

    /* duration: count of samples above 0.3 × peak */
    float dur_lvl = 0.3f * peak;
    int dur_count = 0;
    for (uint32_t i = 0; i < n; i++) {
        if (fabsf((float)buf[i]) > dur_lvl) dur_count++;
    }
    ev->dur_us = (float)dur_count * 1e6f / (float)AE_SAMPLE_RATE_HZ;

    /* spectral features: small DFT over 0..200 kHz using Goertzel at bands */
    /* lo band 20-80 kHz, hi band 80-200 kHz — compute power via Goertzel
     * at 16 logarithmically spaced centre frequencies. */
    float lo_pwr = 0.0f, hi_pwr = 0.0f;
    const int nband_lo = 4, nband_hi = 8;
    float lo_centres[] = {25000.0f, 40000.0f, 55000.0f, 70000.0f};
    float hi_centres[] = {90000.0f, 110000.0f, 130000.0f, 150000.0f,
                          170000.0f, 185000.0f, 195000.0f, 199000.0f};
    float lo_cen_sum = 0.0f, hi_cen_sum = 0.0f;
    for (int b = 0; b < nband_lo; b++) {
        float p = goertzel_power(buf, n, lo_centres[b], (float)AE_SAMPLE_RATE_HZ);
        lo_pwr += p;
        lo_cen_sum += p * lo_centres[b];
    }
    for (int b = 0; b < nband_hi; b++) {
        float p = goertzel_power(buf, n, hi_centres[b], (float)AE_SAMPLE_RATE_HZ);
        hi_pwr += p;
        hi_cen_sum += p * hi_centres[b];
    }
    float total = lo_pwr + hi_pwr;
    if (total > 0.0f) {
        ev->centroid_khz = (lo_cen_sum + hi_cen_sum) / total / 1000.0f;
        ev->hi_lo_ratio = (lo_pwr > 0.0f) ? (hi_pwr / lo_pwr) : 100.0f;
    } else {
        ev->centroid_khz = 0.0f;
        ev->hi_lo_ratio = 0.0f;
    }
}

/*
 * classify — simple additive decision-tree scoring. Returns 0..100.
 * A confirmed wire-break burst is: short rise (20-600 µs), short-to-medium
 * duration (40-4000 µs), high amplitude (> 6 mV), high centroid (> 80 kHz),
 * high hi/lo ratio (> 1.5). Each criterion contributes points.
 */
static uint8_t classify(const ae_event_t *ev)
{
    int score = 0;

    /* amplitude (peak_uv in µV; threshold ~6 mV = 6000 µV) */
    if (ev->peak_uv > 20000.0f)      score += 25;
    else if (ev->peak_uv > 10000.0f)  score += 18;
    else if (ev->peak_uv > 6000.0f)   score += 10;

    /* rise time */
    if (ev->rise_us >= AE_MIN_RISE_US && ev->rise_us <= AE_MAX_RISE_US)
        score += 20;
    else if (ev->rise_us < AE_MIN_RISE_US)
        score += 5;   /* too fast: probably impact noise */

    /* duration */
    if (ev->dur_us >= AE_MIN_DUR_US && ev->dur_us <= AE_MAX_DUR_US)
        score += 20;

    /* spectral centroid */
    if (ev->centroid_khz > 80.0f) score += 20;
    else if (ev->centroid_khz > 60.0f) score += 10;

    /* hi/lo ratio */
    if (ev->hi_lo_ratio > 1.5f) score += 15;

    if (score > 100) score = 100;
    return (uint8_t)score;
}

/*
 * downsample_summary — 8:1 average decimation for the uplink packet.
 */
static void downsample_summary(uint8_t *out, const int16_t *in, uint32_t n)
{
    uint32_t out_n = n / 8;
    for (uint32_t i = 0; i < out_n && i < 24; i++) {
        int32_t acc = 0;
        for (int j = 0; j < 8; j++) acc += in[i * 8 + j];
        int32_t avg = acc / 8;
        /* scale to uint8 (±32768 → 0..255, midpoint 128) */
        int32_t v = 128 + (avg >> 8);
        if (v < 0) v = 0;
        if (v > 255) v = 255;
        out[i] = (uint8_t)v;
    }
}

/*
 * goertzel_power — efficient single-frequency power via Goertzel algorithm.
 * Returns the magnitude squared of the DFT bin at f_hz.
 */
float goertzel_power(const int16_t *buf, uint32_t n, float f_hz, float fs)
{
    float k = f_hz * (float)n / fs;
    float omega = 2.0f * 3.14159265f * k / (float)n;
    float coeff = 2.0f * cosf(omega);
    float s_prev = 0.0f, s_prev2 = 0.0f;
    for (uint32_t i = 0; i < n; i++) {
        float s = (float)buf[i] + coeff * s_prev - s_prev2;
        s_prev2 = s_prev;
        s_prev = s;
    }
    float re = s_prev2 * cosf(omega) - s_prev;
    float im = s_prev2 * sinf(omega);
    return re * re + im * im;
}

/* ---- HAL stubs ---- */
void adc_start_circular(uint8_t ch, int16_t *buf, uint32_t n)
{
    (void)ch; (void)buf; (void)n;
    /* real firmware: configure ADC1 DMA circular mode, TIM3 trigger at 200 kHz */
}

void adc_capture_single(uint8_t ch, int16_t *buf, uint32_t n)
{
    (void)ch; (void)buf; (void)n;
}

void comp1_arm(float threshold_uv)
{
    (void)threshold_uv;
    /* real firmware: COMP1 inverting input from DAC, non-inverting from AE amp,
     * output routed to EXTI line 1 with rising-edge interrupt. */
}

uint32_t rtc_get_unix(void)
{
    return 0;  /* real firmware reads RTC_TR/DR and converts */
}

void flash_log_ae_local(const ae_event_t *ev)
{
    (void)ev;
    /* real firmware writes ae_event_t + waveform to the AE flash region.
     * The public flash_log_ae() is defined in flash_log.c. */
}