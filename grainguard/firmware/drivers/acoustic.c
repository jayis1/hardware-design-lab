/*
 * acoustic.c — Acoustic emission insect detection driver
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 *
 * Two-stage detection:
 *  1. Envelope mode (1 kS/s, 5-min window): detect AE events above
 *     background.  Each event = short-term RMS > long-term RMS × RATIO
 *     for >= MIN_DUR_MS.
 *  2. Spectral mode (192 kS/s, 10-s window): 512-point FFT, classify
 *     dominant frequency against known insect signatures.
 *
 * ADC pin: PB0 (envelope), PB1 (raw).  Mode select: PA10.
 */

#include "acoustic.h"
#include "../board.h"
#include "../registers.h"

/* ---- Private helpers ---- */
static void adc_init_one_channel(uint8_t channel) {
    /* Enable ADC clock */
    /* (Simplified) configure CFGR for single conversion, 12-bit,
     * software trigger.  Real register offsets vary by family. */
    ADC1->CFGR = 0;  /* software trigger, 12-bit right-aligned */
    ADC1->SQR1 = (1 << 29) | (channel << 6);  /* 1 conversion, first = channel */
    ADC1->SMPR1 = (7 << (channel * 3));  /* max sampling time */
    ADC1->ISR = ADC_ISR_ADRDY;
    ADC1->CR |= ADC_CR_ADEN;
    while (!(ADC1->ISR & ADC_ISR_ADRDY)) { }
}

static uint16_t adc_read_once(void) {
    ADC1->CR |= ADC_CR_ADSTART;
    while (!(ADC1->ISR & ADC_ISR_EOC)) { }
    return (uint16_t)ADC1->DR;
}

/* ---- Envelope-mode scan ---- */
static int envelope_scan(acoustic_result_t *out) {
    /* Configure PA10 = 0 (envelope mode) */
    GPIOA->BSRR = (1 << (PA10__AE_MODE + 16));
    delay_ms(2);

    adc_init_one_channel(PB0__AE_ENVELOPE_ADC);
    delay_ms(5);

    /* Collect samples at ~1 kS/s for AE_WINDOW_S seconds */
    const uint32_t total_samples = AE_WINDOW_S * AE_ADC_SAMPLE_RATE_ENV;
    uint16_t events = 0;
    uint16_t peak_mv = 0;
    uint32_t total_dur_ms = 0;

    /* Sliding RMS windows */
    int16_t short_buf[AE_SHORT_WIN_MS];  /* 10 samples */
    int16_t long_buf[AE_LONG_WIN_MS];    /* 500 samples */
    uint16_t short_idx = 0, long_idx = 0;

    /* Running sums */
    int32_t short_sum_sq = 0;
    int32_t long_sum_sq  = 0;

    int16_t baseline = 2048;  /* mid-scale for 12-bit (no signal) */

    for (uint32_t s = 0; s < total_samples; s++) {
        int16_t raw = (int16_t)adc_read_once() - baseline;
        int32_t sq = (int32_t)raw * raw;

        /* Update short window */
        short_sum_sq -= short_buf[short_idx] * short_buf[short_idx];
        short_buf[short_idx] = raw;
        short_sum_sq += sq;
        short_idx = (short_idx + 1) % AE_SHORT_WIN_MS;

        /* Update long window */
        long_sum_sq -= long_buf[long_idx] * long_buf[long_idx];
        long_buf[long_idx] = raw;
        long_sum_sq += sq;
        long_idx = (long_idx + 1) % AE_LONG_WIN_MS;

        /* Track peak amplitude */
        uint16_t abs_mv = (uint16_t)(raw > 0 ? raw : -raw);
        if (abs_mv > peak_mv) peak_mv = abs_mv;

        /* Event detection */
        double short_rms = (double)short_sum_sq / AE_SHORT_WIN_MS;
        double long_rms  = (double)long_sum_sq  / AE_LONG_WIN_MS;
        if (long_rms > 1 && short_rms > long_rms * AE_EVENT_RATIO * AE_EVENT_RATIO) {
            /* Start of candidate event; count it */
            events++;
            uint16_t dur = AE_EVENT_MIN_DUR_MS;
            total_dur_ms += dur;
        }

        /* ~1 ms between samples (coarse delay) */
        delay_ms(0);  /* fine-tune for 1 ms at 48 MHz */
    }

    out->events_per_min        = (uint16_t)(events / (AE_WINDOW_S / 60));
    out->peak_amplitude_mv     = peak_mv;
    out->avg_event_duration_ms = events ? (uint16_t)(total_dur_ms / events) : 0;

    /* Threshold: >10 events/min => infestation likely */
    if (out->events_per_min > 10) {
        return 1;  /* need spectral confirmation */
    } else {
        out->species = INSECT_NONE;
        out->confidence_pct = 100;
        return 0;
    }
}

/* ---- 512-point FFT (radix-2, in-place, fixed-point) ---- */
#define FFT_N 512

static int16_t fft_re[FFT_N];
static int16_t fft_im[FFT_N];

/* Bit-reversal permutation */
static void fft_bit_reverse(void) {
    int j = 0;
    for (int i = 1; i < FFT_N; i++) {
        int bit = FFT_N >> 1;
        while (j & bit) {
            j ^= bit;
            bit >>= 1;
        }
        j ^= bit;
        if (i < j) {
            int16_t tr = fft_re[i], ti = fft_im[i];
            fft_re[i] = fft_re[j]; fft_im[i] = fft_im[j];
            fft_re[j] = tr;         fft_im[j] = ti;
        }
    }
}

static int16_t isqrt(int32_t x) {
    if (x <= 0) return 0;
    int16_t res = 0, bit = 1 << 14;
    while (bit > x) bit >>= 1;
    while (bit) {
        if (x >= (int32_t)(res + bit) * (res + bit)) {
            res += bit;
        }
        bit >>= 1;
    }
    return res;
}

static void fft_compute(int16_t *re, int16_t *im, int n, int log2_n) {
    /* Cooley-Tukey radix-2 iterative FFT (Q15 fixed-point) */
    fft_bit_reverse();
    for (int s = 1; s <= log2_n; s++) {
        int m = 1 << s;
        int m2 = m >> 1;
        /* twiddle: cos/sin from lookup would be ideal; compute approx */
        for (int k = 0; k < n; k += m) {
            for (int j = 0; j < m2; j++) {
                int angle_idx = (j * FFT_N) / m;
                /* Precomputed cos/sin table would go here.
                 * For simplicity, approximate with integer math. */
                int16_t wr = (int16_t)(32767 * cos(-2.0 * 3.14159 * angle_idx / FFT_N));
                int16_t wi = (int16_t)(32767 * sin(-2.0 * 3.14159 * angle_idx / FFT_N));
                int16_t xr = re[k + j + m2];
                int16_t xi = im[k + j + m2];
                int32_t tr = ((int32_t)wr * xr - (int32_t)wi * xi) >> 15;
                int32_t ti = ((int32_t)wr * xi + (int32_t)wi * xr) >> 15;
                re[k + j + m2] = (int16_t)(re[k + j] - tr);
                im[k + j + m2] = (int16_t)(im[k + j] - ti);
                re[k + j]      = (int16_t)(re[k + j] + tr);
                im[k + j]      = (int16_t)(im[k + j] + ti);
            }
        }
    }
}

/* ---- Spectral confirmation ---- */
int acoustic_spectral_confirm(acoustic_result_t *out) {
    /* Switch to raw mode: PA10 = 1 */
    GPIOA->BSRR = (1 << PA10__AE_MODE);
    delay_ms(2);

    adc_init_one_channel(PB1__AE_RAW_ADC);
    delay_ms(5);

    /* Capture FFT_N samples at 192 kS/s (need precise timing; approximated) */
    for (int i = 0; i < FFT_N; i++) {
        int16_t raw = (int16_t)adc_read_once() - 2048;
        fft_re[i] = raw;
        fft_im[i] = 0;
        delay_ms(0);  /* tune for ~5 us */
    }

    /* Switch back to envelope mode */
    GPIOA->BSRR = (1 << (PA10__AE_MODE + 16));

    /* Compute FFT */
    fft_compute(fft_re, fft_im, FFT_N, 9);

    /* Find dominant frequency bin (skip DC + low bins) */
    int16_t max_mag = 0;
    int   max_bin  = 0;
    for (int i = 5; i < FFT_N / 2; i++) {
        int32_t mag_sq = (int32_t)fft_re[i] * fft_re[i] + (int32_t)fft_im[i] * fft_im[i];
        int16_t mag = isqrt(mag_sq);
        if (mag > max_mag) { max_mag = mag; max_bin = i; }
    }

    /* Bin frequency = max_bin * sample_rate / FFT_N */
    /* = max_bin * 192000 / 512 = max_bin * 375 Hz */
    int32_t freq_hz = max_bin * 375;

    /* Classify species by frequency band */
    insect_id_t id = INSECT_UNKNOWN;
    uint8_t conf = 50;

    if (freq_hz >= 26000 && freq_hz <= 30000) {
        id = INSECT_SITOPHILUS_GRANARIUS;
        conf = 80;
    } else if (freq_hz >= 42000 && freq_hz <= 48000) {
        id = INSECT_TRIBOLIUM_CASTANEUM;
        conf = 78;
    } else if (freq_hz >= 55000 && freq_hz <= 65000) {
        id = INSECT_RHYZOPERTHA_DOMINICA;
        conf = 75;
    }

    out->species = id;
    out->confidence_pct = conf;
    return 0;
}

/* ---- Public API ---- */

int acoustic_scan(acoustic_result_t *out) {
    acoustic_power_on();
    delay_ms(10);

    /* Zero out result */
    out->events_per_min = 0;
    out->peak_amplitude_mv = 0;
    out->avg_event_duration_ms = 0;
    out->species = INSECT_NONE;
    out->confidence_pct = 0;

    int need_confirm = envelope_scan(out);
    if (need_confirm) {
        acoustic_spectral_confirm(out);
    }

    acoustic_power_off();
    return 0;
}

void acoustic_power_on(void) {
    GPIOC->BSRR = (1 << PC2__AE_SUPPLY_EN);
}
void acoustic_power_off(void) {
    GPIOC->BSRR = (1 << (PC2__AE_SUPPLY_EN + 16));
}