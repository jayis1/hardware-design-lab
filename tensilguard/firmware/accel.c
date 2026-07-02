/*
 * accel.c — Accelerometer-based cable tension estimation
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 *
 * Drives the TDK ICM-42688-P 6-axis IMU over SPI3, collects a 32-second
 * ambient-vibration window at 100 Hz, runs a Hann-windowed real FFT, applies
 * a harmonic product spectrum to robustly pick the cable's fundamental
 * transverse vibration frequency f1, and inverts the taut-string formula
 *   T = mu * (2*L*f1)^2
 * (with an optional Irvine sag correction) to produce an independent
 * tension estimate T_vib used to cross-check the magnetoelastic T_mag.
 *
 * The FFT is a portable radix-2 Cooley-Tukey implementation; production
 * builds can swap in the ARM CMSIS-DSP arm_rfft_f32 for ~3× speedup, but
 * this standalone version keeps the file self-contained and testable on
 * a host compiler.
 */
#include <math.h>
#include <string.h>
#include "tensilguard.h"
#include "board.h"
#include "registers.h"
#include "hal.h"

/* real and imaginary working buffers (zero-padded to ACC_FFT_SIZE) */
static float s_real[ACC_FFT_SIZE];
static float s_imag[ACC_FFT_SIZE];
static float s_win[ACC_NSAMP];            /* Hann window                */
static float s_mag[ACC_FFT_SIZE / 2];     /* magnitude spectrum         */

/* ----------------------------- Prototypes --------------------------------- */
static void  fft_radix2(float *re, float *im, int n);
static float pick_fundamental_hps(const float *mag, int nbins, float df);
static void  hann_window(float *w, int n);
/* spi3_read_burst is defined at the bottom of this file (HAL stub); it is
 * declared non-static in hal.h so other modules could share it. */

/* ----------------------------- Public API --------------------------------- */

/*
 * accel_init() — configure ICM-42688-P for low-noise 100 Hz accel-only mode.
 */
bool accel_init(void)
{
    uint8_t whoami = spi3_read_reg(0x75);   /* WHO_AM_I = 0x47 */
    if (whoami != 0x47) return false;

    spi3_write_reg(0x4C, 0x00);             /* PWR_MGMT0: OFF  */
    delay_ms(2);
    spi3_write_reg(0x4C, 0x0C);             /* PWR_MGMT0: LN mode, accel on */
    spi3_write_reg(0x4F, 0x01);             /* ACCEL_CONFIG0: ODR 100 Hz, UI filter */
    spi3_write_reg(0x50, 0x03);             /* ACCEL_CONFIG1: full-scale ±2g */
    spi3_write_reg(0x4D, 0x03);             /* LPF: ~50 Hz BW */
    delay_ms(10);

    hann_window(s_win, ACC_NSAMP);
    return true;
}

/*
 * accel_measure_tension() — collect window, FFT, pick f1, compute T_vib.
 * Returns true on success.
 */
bool accel_measure_tension(float *f1_out, float *t_vib_out)
{
    /* read accelerometer FIFO into a static buffer */
    static int16_t accel_x[ACC_NSAMP];
    static int16_t accel_y[ACC_NSAMP];
    static int16_t accel_z[ACC_NSAMP];

    spi3_read_burst(0x1F, (uint8_t *)accel_x, ACC_NSAMP * 2);
    spi3_read_burst(0x21, (uint8_t *)accel_y, ACC_NSAMP * 2);
    spi3_read_burst(0x23, (uint8_t *)accel_z, ACC_NSAMP * 2);

    /* combine axes into a magnitude (cable vibration is omnidirectional) */
    for (int i = 0; i < ACC_NSAMP; i++) {
        float x = (float)accel_x[i] / 16384.0f;   /* ±2g, 16-bit: 16384 LSB/g */
        float y = (float)accel_y[i] / 16384.0f;
        float z = (float)accel_z[i] / 16384.0f;
        s_real[i] = sqrtf(x * x + y * y + z * z);
    }

    /* apply Hann window and zero-pad to ACC_FFT_SIZE */
    for (int i = 0; i < ACC_NSAMP; i++) s_real[i] *= s_win[i];
    for (int i = ACC_NSAMP; i < ACC_FFT_SIZE; i++) s_real[i] = 0.0f;
    memset(s_imag, 0, sizeof(s_imag));

    /* run FFT */
    fft_radix2(s_real, s_imag, ACC_FFT_SIZE);

    /* magnitude spectrum (single-sided) */
    int nbins = ACC_FFT_SIZE / 2;
    float df = (float)ACC_RATE_HZ / (float)ACC_FFT_SIZE;  /* Hz/bin */
    for (int i = 0; i < nbins; i++) {
        s_mag[i] = sqrtf(s_real[i] * s_real[i] + s_imag[i] * s_imag[i]);
    }

    /* pick fundamental via harmonic product spectrum */
    float f1 = pick_fundamental_hps(s_mag, nbins, df);
    if (f1 < ACC_FMIN_HZ || f1 > ACC_FMAX_HZ) {
        *f1_out = 0.0f;
        *t_vib_out = 0.0f;
        return false;
    }

    *f1_out = f1;

    /* temperature-corrected free length (thermal expansion of steel) */
    float alpha = 12e-6f;                  /* 1/°C for steel */
    float dT = g_last.temp_c - 20.0f;       /* relative to 20 °C ref */
    float L_corr = g_cal.cable_len_m * (1.0f + alpha * dT);

    /* Irvine sag correction (small): f1^2 ≈ f10^2 * (1 + 0.5*(g*cos/L)^2/T^2 ...)
     * For most stays sag is small; apply a first-order correction iteratively. */
    float mu_lin = g_cal.cable_lin_mass;     /* kg/m */
    float g = 9.81f;
    float T_est = mu_lin * (2.0f * L_corr * f1) * (2.0f * L_corr * f1);
    /* sag correction: f1_measured^2 = T/(4 L^2 mu) * (1 + (mu g L cos)^2 / (8 T^2)) */
    float sag = g_cal.sag_m;
    if (sag > 0.0f && T_est > 0.0f) {
        float corr = 1.0f + (mu_lin * g * sag * sag) / (8.0f * T_est);
        T_est /= corr;
    }
    *t_vib_out = T_est / 1000.0f;           /* N → kN */
    return true;
}

/*
 * accel_rms_mg() — short-window RMS in milli-g, used for vibration trending.
 */
int16_t accel_rms_mg(void)
{
    /* read 200 samples (2 s) and compute RMS of the magnitude */
    int16_t buf[600];
    spi3_read_burst(0x1F, (uint8_t *)buf, 600);
    float acc = 0.0f;
    for (int i = 0; i < 200; i++) {
        float x = (float)buf[i] / 16384.0f * 1000.0f;  /* mg */
        acc += x * x;
    }
    return (int16_t)sqrtf(acc / 200.0f);
}

/* ----------------------------- Internals ---------------------------------- */

/*
 * fft_radix2 — iterative in-place radix-2 Cooley-Tukey FFT.
 * n must be a power of two. re/im are modified in place.
 */
static void fft_radix2(float *re, float *im, int n)
{
    /* bit-reversal permutation */
    int j = 0;
    for (int i = 1; i < n; i++) {
        int bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) {
            float tr = re[i]; re[i] = re[j]; re[j] = tr;
            float ti = im[i]; im[i] = im[j]; im[j] = ti;
        }
    }

    /* butterflies */
    for (int len = 2; len <= n; len <<= 1) {
        float ang = -2.0f * 3.14159265f / (float)len;
        float wlr = cosf(ang), wli = sinf(ang);
        int half = len >> 1;
        for (int i = 0; i < n; i += len) {
            float wr = 1.0f, wi = 0.0f;
            for (int k = 0; k < half; k++) {
                int a = i + k;
                int b = i + k + half;
                float tr = re[b] * wr - im[b] * wi;
                float ti = re[b] * wi + im[b] * wr;
                re[b] = re[a] - tr;
                im[b] = im[a] - ti;
                re[a] += tr;
                im[a] += ti;
                /* twiddle: w *= wl */
                float nwr = wr * wlr - wi * wli;
                float nwi = wr * wli + wi * wlr;
                wr = nwr; wi = nwi;
            }
        }
    }
}

static void hann_window(float *w, int n)
{
    for (int i = 0; i < n; i++) {
        w[i] = 0.5f * (1.0f - cosf(2.0f * 3.14159265f * (float)i / (float)(n - 1)));
    }
}

/*
 * pick_fundamental_hps — harmonic product spectrum.
 * Downsample the spectrum by 2,3,4,5 and multiply; the fundamental bin is
 * where all harmonics align, making it robust against picking a harmonic.
 * We restrict the search to ACC_FMIN..ACC_FMAX.
 */
static float pick_fundamental_hps(const float *mag, int nbins, float df)
{
    static float hps[ACC_FFT_SIZE / 2];
    int min_bin = (int)(ACC_FMIN_HZ / df);
    int max_bin = (int)(ACC_FMAX_HZ / df);
    if (max_bin >= nbins) max_bin = nbins - 1;
    if (min_bin < 1) min_bin = 1;

    /* HPS = mag * mag(2x) * mag(3x) * mag(4x) * mag(5x) */
    int hmax = nbins / 5;
    for (int i = min_bin; i < hmax && i < max_bin; i++) {
        float p = mag[i];
        for (int h = 2; h <= 5; h++) {
            int idx = i * h;
            if (idx < nbins) p *= mag[idx];
        }
        hps[i] = p;
    }
    /* find peak in the restricted band */
    int peak = min_bin;
    float peakval = 0.0f;
    for (int i = min_bin; i < hmax && i < max_bin; i++) {
        if (hps[i] > peakval) {
            peakval = hps[i];
            peak = i;
        }
    }
    /* parabolic interpolation around the peak for sub-bin accuracy */
    float y0 = (peak > 0) ? hps[peak - 1] : 0.0f;
    float y1 = hps[peak];
    float y2 = (peak + 1 < nbins) ? hps[peak + 1] : 0.0f;
    float denom = (y0 - 2.0f * y1 + y2);
    float delta = 0.0f;
    if (fabsf(denom) > 1e-9f) {
        delta = 0.5f * (y0 - y2) / denom;
        if (delta > 0.5f) delta = 0.5f;
        if (delta < -0.5f) delta = -0.5f;
    }
    return ((float)peak + delta) * df;
}

/* ---- SPI3 stubs; real firmware implements these against the SPI3 regs ---- */
uint8_t spi3_read_reg(uint8_t reg)
{
    (void)reg;
    return 0;
}

void spi3_write_reg(uint8_t reg, uint8_t val)
{
    (void)reg; (void)val;
}

void spi3_read_burst(uint8_t reg, uint8_t *buf, uint16_t n)
{
    (void)reg; (void)buf; (void)n;
}