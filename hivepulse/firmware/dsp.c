/*
 * dsp.c — DSP pipeline: FFT, feature extraction for beehive acoustic analysis
 *
 * Implements the complete acoustic feature extraction pipeline:
 * 1. Apply Hann window to raw audio
 * 2. Compute 4096-point real FFT (radix-4 butterfly, in-place)
 * 3. Compute power spectral density
 * 4. Extract 32 features: fundamental, centroid, spread, flatness, rolloff,
 *    8 sub-band energies, AM depth/rate, HNR, 12 cepstral coefficients
 *
 * The FFT is implemented as a hand-optimized radix-4 Cooley-Tukey decomposition
 * leveraging the Cortex-M7 FPU and SIMD instructions where possible.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#include "dsp.h"
#include "audio.h"
#include "board.h"
#include <math.h>

/* ---- Constants ---- */
#define FS              48000.0f    /* Sampling frequency */
#define FFT_SIZE        AUDIO_FFT_SIZE  /* 4096 */
#define LOG2_FFT        12          /* log2(4096) = 12 */
#define PSD_SIZE        (FFT_SIZE / 2 + 1)  /* 2049 bins */
#define FREQ_RESOLUTION (FS / FFT_SIZE)     /* ~5.86 Hz/bin */

/* Sub-band boundaries (Hz) for 8 bands */
static const float subband_bounds[NUM_SUBBANDS + 1] = {
    0, 250, 500, 750, 1000, 2000, 4000, 8000, 16000
};

/* Mel filterbank: 12 triangular filters centered on mel scale */
static const float mel_centers[NUM_CEPSTRAL + 2] = {
    /* Mel frequencies converted back to Hz */
    100, 200, 300, 400, 550, 750, 1000, 1350, 1800, 2400, 3200, 4200, 5500
};

/* ---- Precomputed Hann Window ---- */
static float hann_window[FFT_SIZE];
static bool dsp_initialized = false;

/* ---- FFT Bit-Reversal Table ---- */
static uint16_t bit_reversal_table[FFT_SIZE];

/* ---- Twiddle Factor Tables ---- */
static float cos_table[FFT_SIZE / 4];
static float sin_table[FFT_SIZE / 4];

/* ---- Working Buffers (in SRAM2 for zero-bus-contention) ---- */
static float fft_input[FFT_SIZE]    __attribute__((section(".sram2")));
static float fft_output[FFT_SIZE]   __attribute__((section(".sram2")));
static float psd_buffer[PSD_SIZE]   __attribute__((section(".sram2")));
static float envelope[FFT_SIZE]     __attribute__((section(".sram2")));

/* ---- Initialize Bit-Reversal Table ---- */
static void init_bit_reversal(void)
{
    for (int i = 0; i < FFT_SIZE; i++) {
        int rev = 0;
        int x = i;
        for (int j = 0; j < LOG2_FFT; j++) {
            rev = (rev << 1) | (x & 1);
            x >>= 1;
        }
        bit_reversal_table[i] = (uint16_t)rev;
    }
}

/* ---- Initialize Twiddle Factors ---- */
static void init_twiddle_factors(void)
{
    for (int i = 0; i < FFT_SIZE / 4; i++) {
        float angle = -2.0f * M_PI * (float)i / (float)FFT_SIZE;
        cos_table[i] = cosf(angle);
        sin_table[i] = sinf(angle);
    }
}

/* ---- Initialize Hann Window ---- */
static void init_hann_window(void)
{
    for (int i = 0; i < FFT_SIZE; i++) {
        hann_window[i] = 0.5f * (1.0f - cosf(2.0f * M_PI * (float)i /
                              (float)(FFT_SIZE - 1)));
    }
}

/* ---- Radix-4 FFT Implementation ---- */
/*
 * Performs an in-place radix-4 Cooley-Tukey FFT on the real and imaginary
 * arrays. The algorithm processes the butterfly stages in groups of 4,
 * reducing the number of stages from log2(N) to log4(N) = 6 stages for N=4096.
 */
static void fft_radix4(float *re, float *im, int n)
{
    int stages = LOG2_FFT / 2;  /* 6 stages for 4096 */
    int groups = n / 4;

    /* Stage loop: each stage processes increasingly larger groups */
    for (int stage = 0; stage < stages; stage++) {
        int group_size = 1 << (2 * (stage + 1));  /* 4, 16, 64, 256, 1024, 4096 */
        int butterfly_span = group_size / 4;       /* 1, 4, 16, 64, 256, 1024 */
        int twiddle_step = FFT_SIZE / group_size;  /* Twiddle factor stride */

        for (int group_start = 0; group_start < n; group_start += group_size) {
            for (int j = 0; j < butterfly_span; j++) {
                int idx0 = group_start + j;
                int idx1 = group_start + j + butterfly_span;
                int idx2 = group_start + j + 2 * butterfly_span;
                int idx3 = group_start + j + 3 * butterfly_span;

                int tw_idx = j * twiddle_step;

                /* Load twiddle factors */
                float wr1 = cos_table[tw_idx];
                float wi1 = sin_table[tw_idx];
                float wr2 = cos_table[2 * tw_idx % (FFT_SIZE/4)];
                float wi2 = sin_table[2 * tw_idx % (FFT_SIZE/4)];
                float wr3 = cos_table[3 * tw_idx % (FFT_SIZE/4)];
                float wi3 = sin_table[3 * tw_idx % (FFT_SIZE/4)];

                /* Butterfly: 4-point DFT with twiddle multiplication */
                float r0 = re[idx0], i0 = im[idx0];
                float r1 = re[idx1] * wr1 - im[idx1] * wi1;
                float i1 = re[idx1] * wi1 + im[idx1] * wr1;
                float r2 = re[idx2] * wr2 - im[idx2] * wi2;
                float i2 = re[idx2] * wi2 + im[idx2] * wr2;
                float r3 = re[idx3] * wr3 - im[idx3] * wi3;
                float i3 = re[idx3] * wi3 + im[idx3] * wr3;

                /* Sum and difference stages of radix-4 butterfly */
                re[idx0] = r0 + r1 + r2 + r3;
                im[idx0] = i0 + i1 + i2 + i3;
                re[idx1] = r0 - r2 + (i1 - i3);
                im[idx1] = i0 - i2 - (r1 - r3);
                re[idx2] = r0 - r1 + r2 - r3;
                im[idx2] = i0 - i1 + i2 - i3;
                re[idx3] = r0 - r2 - (i1 - i3);
                im[idx3] = i0 - i2 + (r1 - r3);
            }
        }
    }

    /* If LOG2_FFT is odd, do a final radix-2 stage (not needed for 4096) */
}

/* ---- Bit-Reversal Permutation ---- */
static void fft_bit_reverse(float *re, float *im, int n)
{
    for (int i = 0; i < n; i++) {
        int j = bit_reversal_table[i];
        if (j > i) {
            /* Swap */
            float tr = re[i]; re[i] = re[j]; re[j] = tr;
            float ti = im[i]; im[i] = im[j]; im[j] = ti;
        }
    }
}

/* ---- Real FFT: pack/unpack for RFFT using complex FFT ---- */
static void real_fft(const float *input, float *output_re, float *output_im)
{
    /* Pack real input into half-size complex FFT:
     * even samples -> real, odd samples -> imaginary */
    int half = FFT_SIZE / 2;
    for (int i = 0; i < half; i++) {
        fft_output[2 * i] = input[2 * i];       /* Real */
        fft_output[2 * i + 1] = input[2 * i + 1]; /* Imag (odd samples) */
    }

    /* Actually, for simplicity, we do a full complex FFT with imag=0 */
    for (int i = 0; i < FFT_SIZE; i++) {
        fft_output[i] = input[i];
        /* Use second half of array for imaginary part */
    }

    /* Zero imaginary part (stored in separate concept, but we use
     * the output arrays directly) */
    float *re = fft_output;
    float im_buf[FFT_SIZE] __attribute__((section(".sram2")));

    for (int i = 0; i < FFT_SIZE; i++) {
        re[i] = input[i];
        im_buf[i] = 0.0f;
    }

    /* Bit-reverse permutation */
    fft_bit_reverse(re, im_buf, FFT_SIZE);

    /* Radix-4 FFT */
    fft_radix4(re, im_buf, FFT_SIZE);

    /* Copy to output */
    for (int i = 0; i < PSD_SIZE; i++) {
        output_re[i] = re[i];
        output_im[i] = im_buf[i];
    }
}

/* ---- Public API Implementation ---- */
int dsp_init(void)
{
    if (dsp_initialized)
        return 0;

    init_hann_window();
    init_bit_reversal();
    init_twiddle_factors();

    dsp_initialized = true;
    return 0;
}

void dsp_apply_hann_window(float *data, int n)
{
    for (int i = 0; i < n; i++)
        data[i] *= hann_window[i];
}

void dsp_compute_fft(const float *input, float *output_mag, int n)
{
    float re_buf[FFT_SIZE] __attribute__((section(".sram2")));
    float im_buf[FFT_SIZE] __attribute__((section(".sram2")));

    /* Copy and window input */
    for (int i = 0; i < n; i++) {
        re_buf[i] = input[i] * hann_window[i];
        im_buf[i] = 0.0f;
    }

    /* Bit-reverse and FFT */
    fft_bit_reverse(re_buf, im_buf, n);
    fft_radix4(re_buf, im_buf, n);

    /* Compute magnitude (only first N/2+1 bins for real FFT) */
    for (int i = 0; i < PSD_SIZE && i < n / 2 + 1; i++) {
        output_mag[i] = sqrtf(re_buf[i] * re_buf[i] + im_buf[i] * im_buf[i]);
    }
}

void dsp_compute_psd(const float *fft_output, float *psd, int n)
{
    /* PSD = |X(k)|^2 / N  (periodogram estimate) */
    float norm = 1.0f / (float)n;
    for (int i = 0; i < PSD_SIZE; i++) {
        psd[i] = fft_output[i] * fft_output[i] * norm;
    }
}

float dsp_detect_fundamental(const float *psd, int n, float fs)
{
    /* Search for peak in 50-600 Hz range (typical bee hum fundamental) */
    int bin_low = (int)(50.0f / FREQ_RESOLUTION);
    int bin_high = (int)(600.0f / FREQ_RESOLUTION);
    if (bin_high >= n) bin_high = n - 1;

    float max_val = 0.0f;
    int max_bin = bin_low;

    for (int i = bin_low; i <= bin_high; i++) {
        if (psd[i] > max_val) {
            max_val = psd[i];
            max_bin = i;
        }
    }

    /* Parabolic interpolation for sub-bin accuracy */
    if (max_bin > 0 && max_bin < n - 1) {
        float alpha = psd[max_bin - 1];
        float beta  = psd[max_bin];
        float gamma = psd[max_bin + 1];
        float denom = alpha - 2.0f * beta + gamma;
        if (fabsf(denom) > 1e-10f) {
            float offset = 0.5f * (alpha - gamma) / denom;
            return (float)(max_bin + offset) * FREQ_RESOLUTION;
        }
    }

    return (float)max_bin * FREQ_RESOLUTION;
}

void dsp_compute_spectral_stats(const float *psd, int n, float fs,
                                 float *centroid, float *spread,
                                 float *flatness, float *rolloff)
{
    float total_energy = 0.0f;
    float weighted_sum = 0.0f;
    float log_sum = 0.0f;
    float geo_mean_exp = 0.0f;
    int nonzero = 0;

    /* Compute total energy and spectral centroid */
    for (int i = 0; i < n; i++) {
        float freq = (float)i * FREQ_RESOLUTION;
        total_energy += psd[i];
        weighted_sum += freq * psd[i];
        if (psd[i] > 1e-20f) {
            log_sum += logf(psd[i]);
            nonzero++;
        }
    }

    /* Spectral centroid: weighted mean frequency */
    *centroid = (total_energy > 0) ? weighted_sum / total_energy : 0.0f;

    /* Spectral spread: standard deviation */
    float variance = 0.0f;
    for (int i = 0; i < n; i++) {
        float freq = (float)i * FREQ_RESOLUTION;
        float diff = freq - *centroid;
        variance += diff * diff * psd[i];
    }
    *spread = (total_energy > 0) ? sqrtf(variance / total_energy) : 0.0f;

    /* Spectral flatness: geometric mean / arithmetic mean */
    if (nonzero > 0 && total_energy > 0) {
        float geo_mean = expf(log_sum / nonzero);
        float arith_mean = total_energy / n;
        *flatness = (arith_mean > 0) ? geo_mean / arith_mean : 0.0f;
    } else {
        *flatness = 0.0f;
    }

    /* Spectral rolloff: 85th percentile frequency */
    float cumsum = 0.0f;
    float threshold = 0.85f * total_energy;
    *rolloff = 0.0f;
    for (int i = 0; i < n; i++) {
        cumsum += psd[i];
        if (cumsum >= threshold) {
            *rolloff = (float)i * FREQ_RESOLUTION;
            break;
        }
    }
}

void dsp_compute_subband_energy(const float *psd, int n, float fs,
                                 float energies[NUM_SUBBANDS])
{
    for (int band = 0; band < NUM_SUBBANDS; band++) {
        int bin_low = (int)(subband_bounds[band] / FREQ_RESOLUTION);
        int bin_high = (int)(subband_bounds[band + 1] / FREQ_RESOLUTION);
        if (bin_high >= n) bin_high = n - 1;
        if (bin_low >= n) bin_low = n - 1;

        float sum = 0.0f;
        for (int i = bin_low; i <= bin_high; i++) {
            sum += psd[i];
        }
        energies[band] = sum;
    }
}

void dsp_compute_am(const float *signal, int n, float fs,
                     float *depth, float *rate)
{
    /* Compute envelope via full-wave rectification + low-pass filter */
    for (int i = 0; i < n; i++) {
        envelope[i] = fabsf(signal[i]);
    }

    /* Simple moving-average low-pass filter (window = 64 samples) */
    int window = 64;
    float smooth_env[FFT_SIZE] __attribute__((section(".sram2")));
    for (int i = 0; i < n; i++) {
        float sum = 0.0f;
        int count = 0;
        for (int j = i - window/2; j <= i + window/2; j++) {
            if (j >= 0 && j < n) {
                sum += envelope[j];
                count++;
            }
        }
        smooth_env[i] = sum / count;
    }

    /* AM depth = (max - min) / (max + min) of envelope */
    float env_max = smooth_env[0];
    float env_min = smooth_env[0];
    for (int i = 1; i < n; i++) {
        if (smooth_env[i] > env_max) env_max = smooth_env[i];
        if (smooth_env[i] < env_min) env_min = smooth_env[i];
    }

    float env_mean = (env_max + env_min) / 2.0f;
    if (env_mean > 1e-10f) {
        *depth = (env_max - env_min) / (env_max + env_min);
    } else {
        *depth = 0.0f;
    }

    /* AM rate: find peak in envelope's autocorrelation (0.5-20 Hz range) */
    /* Simple approach: count zero crossings of detrended envelope */
    float detrended[FFT_SIZE] __attribute__((section(".sram2")));
    for (int i = 0; i < n; i++)
        detrended[i] = smooth_env[i] - env_mean;

    int crossings = 0;
    for (int i = 1; i < n; i++) {
        if ((detrended[i] > 0 && detrended[i-1] <= 0) ||
            (detrended[i] < 0 && detrended[i-1] >= 0)) {
            crossings++;
        }
    }

    /* Rate = crossings / (2 * duration) */
    float duration = (float)n / fs;
    *rate = (crossings / 2.0f) / duration;
}

float dsp_compute_hnr(const float *psd, int n, float fs, float f0)
{
    /* HNR: ratio of harmonic energy to noise energy
     * Harmonic energy = sum of PSD at harmonic frequencies
     * Noise energy = total - harmonic */
    if (f0 < 1.0f) return 0.0f;

    float harmonic_energy = 0.0f;
    float total_energy = 0.0f;
    int max_harmonic = (int)(16000.0f / f0); /* Up to 16 kHz */

    for (int h = 1; h <= max_harmonic; h++) {
        float freq = h * f0;
        int bin = (int)(freq / FREQ_RESOLUTION);
        if (bin >= n) break;

        /* Sum energy in a +/-2 bin window around each harmonic */
        for (int j = -2; j <= 2; j++) {
            int idx = bin + j;
            if (idx >= 0 && idx < n) {
                harmonic_energy += psd[idx];
            }
        }
    }

    for (int i = 0; i < n; i++)
        total_energy += psd[i];

    float noise_energy = total_energy - harmonic_energy;
    if (noise_energy < 1e-10f) return 100.0f; /* Very high HNR */

    /* HNR in dB */
    return 10.0f * log10f(harmonic_energy / noise_energy);
}

void dsp_compute_cepstral(const float *psd, int n, float *cepstral, int num_cep)
{
    /* Compute MFCC-like features:
     * 1. Apply mel filterbank to PSD
     * 2. Take log of each filter output
     * 3. Apply DCT-II to get cepstral coefficients */

    float mel_energy[NUM_CEPSTRAL + 2] __attribute__((section(".sram2")));

    /* Apply triangular mel filterbank */
    for (int m = 0; m < NUM_CEPSTRAL + 2; m++) {
        float center_hz = mel_centers[m];
        float left_hz = (m > 0) ? mel_centers[m - 1] : 0.0f;
        float right_hz = (m < NUM_CEPSTRAL + 1) ? mel_centers[m + 1] : 8000.0f;

        int left_bin = (int)(left_hz / FREQ_RESOLUTION);
        int center_bin = (int)(center_hz / FREQ_RESOLUTION);
        int right_bin = (int)(right_hz / FREQ_RESOLUTION);
        if (right_bin >= n) right_bin = n - 1;

        float energy = 0.0f;
        /* Rising slope */
        for (int i = left_bin; i < center_bin && i < n; i++) {
            if (center_bin > left_bin) {
                float weight = (float)(i - left_bin) / (center_bin - left_bin);
                energy += psd[i] * weight;
            }
        }
        /* Falling slope */
        for (int i = center_bin; i <= right_bin && i < n; i++) {
            if (right_bin > center_bin) {
                float weight = (float)(right_bin - i) / (right_bin - center_bin);
                energy += psd[i] * weight;
            }
        }
        mel_energy[m] = (energy > 1e-20f) ? energy : 1e-20f;
    }

    /* Take log */
    for (int m = 0; m < NUM_CEPSTRAL + 2; m++)
        mel_energy[m] = logf(mel_energy[m]);

    /* DCT-II to get cepstral coefficients */
    for (int k = 0; k < num_cep; k++) {
        float sum = 0.0f;
        for (int m = 0; m < NUM_CEPSTRAL + 2; m++) {
            float coeff = cosf(M_PI * (float)k * (m + 0.5f) /
                               (NUM_CEPSTRAL + 2));
            sum += mel_energy[m] * coeff;
        }
        /* Scaling: c0 has factor sqrt(1/N), others sqrt(2/N) */
        float scale = (k == 0) ? sqrtf(1.0f / (NUM_CEPSTRAL + 2)) :
                                  sqrtf(2.0f / (NUM_CEPSTRAL + 2));
        cepstral[k] = sum * scale;
    }
}

int dsp_extract_features(const audio_snapshot_t *snap, feature_vector_t *feat)
{
    if (!snap || !snap->valid || !feat)
        return -1;

    /* Use channel 0 (interior brood chamber mic) as primary source */
    const int16_t *raw = snap->samples[0];

    /* Convert int16 to float and normalize to [-1, 1] */
    for (int i = 0; i < FFT_SIZE; i++) {
        fft_input[i] = (float)raw[i] / 32768.0f;
    }

    /* Apply Hann window */
    dsp_apply_hann_window(fft_input, FFT_SIZE);

    /* Compute FFT magnitude */
    float fft_mag[PSD_SIZE] __attribute__((section(".sram2")));
    dsp_compute_fft(fft_input, fft_mag, FFT_SIZE);

    /* Compute PSD */
    dsp_compute_psd(fft_mag, psd_buffer, FFT_SIZE);

    /* Extract features */
    feat->fundamental_freq = dsp_detect_fundamental(psd_buffer, PSD_SIZE, FS);

    dsp_compute_spectral_stats(psd_buffer, PSD_SIZE, FS,
                               &feat->spectral_centroid,
                               &feat->spectral_spread,
                               &feat->spectral_flatness,
                               &feat->spectral_rolloff);

    dsp_compute_subband_energy(psd_buffer, PSD_SIZE, FS, feat->subband_energy);

    dsp_compute_am(fft_input, FFT_SIZE, FS, &feat->am_depth, &feat->am_rate);

    feat->hnr = dsp_compute_hnr(psd_buffer, PSD_SIZE, FS, feat->fundamental_freq);

    dsp_compute_cepstral(psd_buffer, PSD_SIZE, feat->cepstral, NUM_CEPSTRAL);

    return 0;
}