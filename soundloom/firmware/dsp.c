/*
 * dsp.c — DSP pipeline: per-channel FFT, mel-spectrogram, event detection
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 *
 * SoundLoom bioacoustic soil health monitor.
 * Implements 1024-point Hann-windowed FFT, 40-bin mel filterbank,
 * log-mel spectrogram generation, energy-based event detection with
 * adaptive threshold, and event capture buffering for the classifier.
 */

#include "dsp.h"
#include "board.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define FFT_N           1024
#define FFT_N_LOG2      10
#define MEL_BINS        CLS_MEL_BINS
#define MEL_FMIN        20.0f
#define MEL_FMAX        8000.0f
#define SAMPLE_RATE     8000.0f

/* ---- Hann window ---- */
static float hann_window[FFT_N];
static uint8_t window_initialised = 0;

/* ---- Mel filterbank (MEL_BINS × FFT_N/2+1) ---- */
static float mel_filterbank[MEL_BINS][FFT_N / 2 + 1];
static uint8_t mel_initialised = 0;

/* ---- FFT: radix-2 Cooley-Tukey, in-place, float ---- */

typedef struct { float re, im; } cpx_t;

static int is_power_of_two(int n) {
    return n > 0 && (n & (n - 1)) == 0;
}

/* Bit reversal */
static void bit_reverse(cpx_t *x, int n) {
    int j = 0;
    for (int i = 1; i < n; i++) {
        int bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) {
            cpx_t tmp = x[i]; x[i] = x[j]; x[j] = tmp;
        }
    }
}

/* In-place iterative FFT, n must be power of two */
static void fft(cpx_t *x, int n, int inverse)
{
    if (!is_power_of_two(n)) return;
    bit_reverse(x, n);
    for (int len = 2; len <= n; len <<= 1) {
        float ang = (inverse ? 2.0f : -2.0f) * (float)M_PI / (float)len;
        float wlen_re = cosf(ang), wlen_im = sinf(ang);
        for (int i = 0; i < n; i += len) {
            float w_re = 1.0f, w_im = 0.0f;
            for (int j = 0; j < len / 2; j++) {
                cpx_t u = x[i + j];
                cpx_t v;
                v.re = x[i + j + len/2].re * w_re - x[i + j + len/2].im * w_im;
                v.im = x[i + j + len/2].re * w_im + x[i + j + len/2].im * w_re;
                x[i + j].re = u.re + v.re;
                x[i + j].im = u.im + v.im;
                x[i + j + len/2].re = u.re - v.re;
                x[i + j + len/2].im = u.im - v.im;
                float nw_re = w_re * wlen_re - w_im * wlen_im;
                w_im = w_re * wlen_im + w_im * wlen_re;
                w_re = nw_re;
            }
        }
    }
    if (inverse) {
        float inv = 1.0f / (float)n;
        for (int i = 0; i < n; i++) { x[i].re *= inv; x[i].im *= inv; }
    }
}

/* ---- Initialise Hann window ---- */

static void init_window(void)
{
    if (window_initialised) return;
    for (int i = 0; i < FFT_N; i++) {
        hann_window[i] = 0.5f * (1.0f - cosf(2.0f * (float)M_PI * (float)i / (float)(FFT_N - 1)));
    }
    window_initialised = 1;
}

/* ---- Mel scale helpers ---- */

static float hz_to_mel(float hz) {
    return 2595.0f * log10f(1.0f + hz / 700.0f);
}
static float mel_to_hz(float mel) {
    return 700.0f * (powf(10.0f, mel / 2595.0f) - 1.0f);
}

/* ---- Initialise mel filterbank ---- */

static void init_mel_filterbank(void)
{
    if (mel_initialised) return;
    float mel_min = hz_to_mel(MEL_FMIN);
    float mel_max = hz_to_mel(MEL_FMAX);
    int nfft_half = FFT_N / 2 + 1;

    /* MEL_BINS triangular filters */
    for (int b = 0; b < MEL_BINS; b++) {
        float mel_lo = mel_min + (mel_max - mel_min) * (float)b / (float)MEL_BINS;
        float mel_ctr = mel_min + (mel_max - mel_min) * (float)(b + 1) / (float)MEL_BINS;
        float mel_hi = mel_min + (mel_max - mel_min) * (float)(b + 2) / (float)MEL_BINS;
        float f_lo = mel_to_hz(mel_lo);
        float f_ctr = mel_to_hz(mel_ctr);
        float f_hi = mel_to_hz(mel_hi);

        memset(mel_filterbank[b], 0, sizeof(mel_filterbank[b]));

        for (int k = 0; k < nfft_half; k++) {
            float f = (float)k * SAMPLE_RATE / (float)FFT_N;
            if (f < f_lo || f > f_hi) continue;
            float w;
            if (f <= f_ctr) {
                w = (f - f_lo) / (f_ctr - f_lo + 1e-9f);
            } else {
                w = (f_hi - f) / (f_hi - f_ctr + 1e-9f);
            }
            if (w < 0.0f) w = 0.0f;
            if (w > 1.0f) w = 1.0f;
            mel_filterbank[b][k] = w;
        }
    }
    mel_initialised = 1;
}

/* ---- Per-channel magnitude spectrum ---- */

static void compute_magnitude_spectrum(const float *samples, int n, float *mag_out)
{
    cpx_t x[FFT_N];
    for (int i = 0; i < n && i < FFT_N; i++) {
        x[i].re = samples[i] * hann_window[i];
        x[i].im = 0.0f;
    }
    fft(x, n, 0);
    int half = n / 2 + 1;
    for (int k = 0; k < half; k++) {
        mag_out[k] = sqrtf(x[k].re * x[k].re + x[k].im * x[k].im);
    }
}

/* ---- Compute mel spectrogram for one channel, one frame ---- */

static void compute_mel_spectrum(const float *samples, int n, float *mel_out)
{
    float mag[FFT_N / 2 + 1];
    compute_magnitude_spectrum(samples, n, mag);
    int nfft_half = FFT_N / 2 + 1;
    for (int b = 0; b < MEL_BINS; b++) {
        float e = 0.0f;
        for (int k = 0; k < nfft_half; k++) {
            e += mag[k] * mel_filterbank[b][k];
        }
        mel_out[b] = log10f(e + 1e-6f);  /* log-mel */
    }
}

/* ---- Event detector state ---- */

typedef struct {
    float    rms_baseline;
    float    rms_alpha;
    float    threshold_factor;
    uint8_t  armed;
    uint32_t frame_count;
} event_detector_t;

static event_detector_t detectors[ADC_USED_CHS];

static float compute_rms(const float *s, int n)
{
    float sum = 0.0f;
    for (int i = 0; i < n; i++) sum += s[i] * s[i];
    return sqrtf(sum / (float)n);
}

static int detect_event(uint8_t ch, float rms)
{
    event_detector_t *d = &detectors[ch];
    /* Update baseline (exponential moving average), but don't update during events */
    float baseline = d->rms_baseline;
    float threshold = baseline * d->threshold_factor;

    int is_event = 0;
    if (rms > threshold && d->armed) {
        is_event = 1;
        d->armed = 0;  /* debounce: won't re-trigger immediately */
    } else if (rms < baseline * 1.2f) {
        d->armed = 1;
    }

    /* Baseline update (only when not in an event) */
    if (!is_event) {
        d->rms_baseline = d->rms_baseline * d->rms_alpha + rms * (1.0f - d->rms_alpha);
    }

    d->frame_count++;
    return is_event;
}

/* ---- Public DSP state ---- */

typedef struct {
    float    channel_buffer[ADC_USED_CHS][FFT_N];   /* ring buffer per channel */
    uint32_t write_idx;
    float    mel_spectrogram[ADC_USED_CHS][CLS_FRAMES][MEL_BINS];
    uint32_t event_counts[CLS_NUM_CLASSES];
    uint32_t total_events;
    dsp_event_t captured[8];
    uint32_t   captured_count;
} dsp_state_t;

static dsp_state_t dsp_state;

/* ---- Initialise DSP ---- */

void dsp_init(void)
{
    init_window();
    init_mel_filterbank();
    memset(&dsp_state, 0, sizeof(dsp_state));
    for (int c = 0; c < ADC_USED_CHS; c++) {
        detectors[c].rms_baseline = 1.0f;
        detectors[c].rms_alpha = 0.99f;
        detectors[c].threshold_factor = 3.0f;  /* 3 sigma */
        detectors[c].armed = 1;
    }
}

/* ---- Process one block of multi-channel samples ---- */

void dsp_process(const int32_t *samples, uint32_t n_ch)
{
    if (n_ch > ADC_USED_CHS) n_ch = ADC_USED_CHS;

    /* Convert int32 samples to float and push into per-channel ring buffers */
    for (uint32_t c = 0; c < n_ch; c++) {
        float s = (float)samples[c] / (float)(1 << 23);  /* normalise 24-bit */
        dsp_state.channel_buffer[c][dsp_state.write_idx % FFT_N] = s;
    }
    dsp_state.write_idx++;

    /* Every FFT_N samples, compute mel spectrogram for all channels */
    if (dsp_state.write_idx % (FFT_N / 2) == 0 && dsp_state.write_idx >= FFT_N) {
        for (uint32_t c = 0; c < n_ch; c++) {
            /* Copy windowed segment */
            float seg[FFT_N];
            uint32_t start = (dsp_state.write_idx - FFT_N) % FFT_N;
            for (int i = 0; i < FFT_N; i++) {
                seg[i] = dsp_state.channel_buffer[c][(start + i) % FFT_N];
            }
            /* Compute mel spectrum */
            float mel[MEL_BINS];
            compute_mel_spectrum(seg, FFT_N, mel);

            /* Shift mel spectrogram (ring of CLS_FRAMES frames) */
            for (int f = CLS_FRAMES - 1; f > 0; f--) {
                memcpy(dsp_state.mel_spectrogram[c][f], dsp_state.mel_spectrogram[c][f-1], sizeof(mel));
            }
            memcpy(dsp_state.mel_spectrogram[c][0], mel, sizeof(mel));

            /* Event detection on this channel */
            float rms = compute_rms(seg, FFT_N);
            if (detect_event((uint8_t)c, rms)) {
                /* Capture event: copy mel spectrogram for this channel */
                if (dsp_state.captured_count < 8) {
                    dsp_event_t *ev = &dsp_state.captured[dsp_state.captured_count++];
                    ev->channel = (uint8_t)c;
                    ev->timestamp = dsp_state.write_idx;
                    ev->rms = rms;
                    /* Flatten mel spectrogram into the event feature vector */
                    int idx = 0;
                    for (int f = 0; f < CLS_FRAMES && idx < (int)(CLS_FRAMES * MEL_BINS); f++) {
                        memcpy(&ev->features[idx], dsp_state.mel_spectrogram[c][f], sizeof(float) * MEL_BINS);
                        idx += MEL_BINS;
                    }
                }
            }
        }
    }
}

/* ---- Get a captured event (called by main loop to feed classifier) ---- */

int dsp_get_event(dsp_event_t *out)
{
    if (dsp_state.captured_count == 0) return 0;
    *out = dsp_state.captured[--dsp_state.captured_count];
    return 1;
}

/* ---- Get the latest mel spectrogram for a specific channel ---- */

void dsp_get_mel_spectrogram(uint8_t ch, float out[CLS_FRAMES][MEL_BINS])
{
    if (ch >= ADC_USED_CHS) return;
    memcpy(out, dsp_state.mel_spectrogram[ch], sizeof(float) * CLS_FRAMES * MEL_BINS);
}

/* ---- Compute spectral entropy (for SVI diversity feature) ---- */

float dsp_spectral_entropy(uint8_t ch)
{
    if (ch >= ADC_USED_CHS) return 0.0f;
    float p[MEL_BINS];
    float sum = 0.0f;
    for (int b = 0; b < MEL_BINS; b++) {
        p[b] = dsp_state.mel_spectrogram[ch][0][b];
        if (p[b] < 0.0f) p[b] = 0.0f;
        sum += p[b];
    }
    if (sum < 1e-9f) return 0.0f;
    float entropy = 0.0f;
    for (int b = 0; b < MEL_BINS; b++) {
        float pi = p[b] / sum;
        if (pi > 1e-9f) entropy -= pi * log2f(pi);
    }
    /* Normalise to 0–1 */
    return entropy / log2f((float)MEL_BINS);
}

/* ---- Update event count for a class ---- */

void dsp_record_class(int class_id)
{
    if (class_id < 0 || class_id >= CLS_NUM_CLASSES) return;
    dsp_state.event_counts[class_id]++;
    dsp_state.total_events++;
}

/* ---- Get event rate per class over a window ---- */

void dsp_get_event_rates(float rates[CLS_NUM_CLASSES], uint32_t window_minutes)
{
    /* Simplified: total counts / window (in real firmware, ring buffer of timestamps) */
    float scale = 1.0f / (float)(window_minutes * 60u * 1000u / 128u);  /* blocks per window */
    if (window_minutes == 0) scale = 1.0f;
    for (int i = 0; i < CLS_NUM_CLASSES; i++) {
        rates[i] = (float)dsp_state.event_counts[i] * scale;
    }
}

/* ---- Reset event counters ---- */

void dsp_reset_counters(void)
{
    memset(dsp_state.event_counts, 0, sizeof(dsp_state.event_counts));
    dsp_state.total_events = 0;
}

/* ---- Get total events detected ---- */

uint32_t dsp_get_total_events(void)
{
    return dsp_state.total_events;
}

/* ---- Set detector sensitivity (threshold factor) ---- */

void dsp_set_sensitivity(float factor)
{
    for (int c = 0; c < ADC_USED_CHS; c++) {
        detectors[c].threshold_factor = factor;
    }
}