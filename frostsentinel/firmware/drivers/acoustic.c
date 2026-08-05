/*
 * drivers/acoustic.c — Piezoelectric acoustic-emission ice-nucleation
 *                      detection driver
 *
 * A Murata 7BB-35-3 PZT diaphragm is bonded to the underside of the
 * leaf-wetness film.  When water freezes, the phase transition releases
 * a broadband ultrasonic micro-acoustic signature.  The PZT signal is
 * AC-coupled, amplified ~60 dB by an OPA2376, and band-passed 20–200 kHz.
 * The MCU's ADC samples the amplified signal at 500 kSPS in 40 ms
 * bursts (20000 samples), then runs a 128-point sliding-window FFT in
 * RAM to extract band energy in the 80–150 kHz range where the freezing
 * signature is most prominent.
 *
 * A nucleation event is declared when the 80–150 kHz band energy exceeds
 * 6σ above the rolling 10-minute baseline for ≥2 consecutive bursts.
 * The cumulative AE energy is integrated into an "ice load" estimate.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#include "acoustic.h"
#include "../board.h"

/* ------------------------------------------------------------------ */
/*  Constants                                                          */
/* ------------------------------------------------------------------ */
#define AE_ADC_CHANNEL       7      /* PA8 = ADC1_IN7 */
#define AE_SAMPLES_PER_BURST 20000  /* 40 ms at 500 kSPS */
#define AE_FFT_N             128    /* 128-point FFT */
#define AE_BASELINE_LEN      20     /* 20 bursts × 30 s = 10 min baseline */
#define AE_BAND_LO_BIN       20     /* ~78 kHz at 500 kSPS/128 */
#define AE_BAND_HI_BIN       38     /* ~148 kHz */

/* ------------------------------------------------------------------ */
/*  State                                                              */
/* ------------------------------------------------------------------ */
static int16_t  s_sample_buf[AE_FFT_N];       /* ring buffer for FFT */
static int16_t  s_fft_re[AE_FFT_N];
static int16_t  s_fft_im[AE_FFT_N];
static uint32_t s_baseline[AE_BASELINE_LEN];
static uint32_t s_baseline_sum;
static uint8_t  s_baseline_idx;
static uint8_t  s_baseline_count;
static uint32_t s_last_band_energy;
static uint32_t s_cumulative_energy;
static uint8_t  s_consecutive_hits;
static uint8_t  s_nucleation_detected;
static uint32_t s_nucleation_time_s;

/* ------------------------------------------------------------------ */
/*  ADC configuration for 500 kSPS single-channel sampling             */
/* ------------------------------------------------------------------ */
static void ae_adc_init(void)
{
    /* Enable ADC clock */
    RCC->AHB2ENR |= RCC_AHB2ENR_ADC12EN;

    /* PA8 as analog input */
    GPIO_CONFIG(GPIOA, 8, GPIO_MODE_ANALOG, 0, 0, GPIO_PUPD_NONE, 0);

    /* ADC: 12-bit, single channel, software trigger, fastest sampling */
    ADC1->CR = 0;
    ADC1->CR |= ADC_CR_ADVREGEN;      /* enable voltage regulator */
    delay_ms(1);                      /* regulator startup */
    ADC1->CFGR = ADC_CFGR_RES_12BIT | ADC_CFGR_CONT;
    ADC1->SMPR1 = 0;                   /* fastest sampling time */

    /* Enable ADC */
    ADC1->ISR = ADC_ISR_ADRDY;
    ADC1->CR |= ADC_CR_ADEN;
    uint32_t to = 0x10000;
    while (!(ADC1->ISR & ADC_ISR_ADRDY) && to--) ;
}

static uint16_t ae_adc_read_fast(void)
{
    ADC1->CR |= ADC_CR_ADSTART;
    uint32_t to = 0x100;
    while (!(ADC1->ISR & ADC_ISR_EOC) && to--) ;
    return (uint16_t)(ADC1->DR1 & 0x0FFF);
}

/* ------------------------------------------------------------------ */
/*  128-point decimation-in-time FFT (radix-2, in-place)               */
/*  Input: s_fft_re[] (real), s_fft_im[] (imag, zeroed)                */
/*  Output: magnitudes^2 in s_fft_re[] (power spectrum)                */
/*                                                                    */
/*  Fixed-point Q15 implementation.  Bit-reversal + butterfly stages.  */
/* ------------------------------------------------------------------ */
static void fft_q15(int16_t *re, int16_t *im, int n)
{
    int i, j, k, m, step;
    int16_t tr, ti, ur, ui, wr, wi;

    /* Bit-reversal permutation */
    j = 0;
    for (i = 1; i < n; i++) {
        int bit = n >> 1;
        while (j & bit) {
            j ^= bit;
            bit >>= 1;
        }
        j ^= bit;
        if (i < j) {
            int16_t tmp_r = re[i]; re[i] = re[j]; re[j] = tmp_r;
            int16_t tmp_i = im[i]; im[i] = im[j]; im[j] = tmp_i;
        }
    }

    /* Butterfly stages: m = stage size (2, 4, 8, ..., n) */
    for (m = 2; m <= n; m <<= 1) {
        int half_m = m >> 1;
        /* Twiddle base angle = -2π/m */
        /* We precompute twiddle factors on the fly via Q15 cos/sin */
        for (k = 0; k < n; k += m) {
            for (i = 0; i < half_m; i++) {
                /* angle = -2π * i / m */
                /* Use lookup for speed (approximate Q15 cos/sin) */
                int idx = (i * 256) / m;  /* 0..127 index */
                /* Mini cos/sin table (128 entries, Q15) */
                static const int16_t cos_tab[128] = {
                    32767,32744,32678,32568,32415,32218,31978,31695,
                    31369,31002,30594,30145,29657,29130,28565,27963,
                    27325,26652,25945,25206,24435,23635,22806,21950,
                    21069,20164,19237,18290,17324,16341,15343,14332,
                    13310,12279,11240,10196, 9148, 8100, 7053, 6010,
                     4972, 3941, 2920, 1910,  913,  -71,-1092,-2099,
                    -3091,-4066,-5021,-5955,-6865,-7749,-8605,-9430,
                   -10221,-10977,-11695,-12373,-13010,-13604,-14154,-14658,
                   -15116,-15526,-15887,-16199,-16461,-16671,-16829,-16934,
                   -16987,-16987,-16934,-16829,-16671,-16461,-16199,-15887,
                   -15526,-15116,-14658,-14154,-13604,-13010,-12373,-11695,
                   -10977,-10221, -9430, -8605, -7749, -6865, -5955, -5021,
                    -4066, -3091, -2099, -1092,   -71,   913,  1910,  2920,
                     3941,  4972,  6010,  7053,  8100,  9148, 10196, 11240,
                    12279, 13310, 14332, 15343, 16341, 17324, 18290, 19237,
                    20164, 21069, 21950, 22806, 23635, 24435, 25206, 25945,
                    26652, 27325, 27963, 28565, 29130, 29657, 30145, 30594
                };
                static const int16_t sin_tab[128] = {
                        0,  913, 1910, 2920, 3941, 4972, 6010, 7053,
                     8100, 9148,10196,11240,12279,13310,14332,15343,
                    16341,17324,18290,19237,20164,21069,21950,22806,
                    23635,24435,25206,25945,26652,27325,27963,28565,
                    29130,29657,30145,30594,31002,31369,31695,31978,
                    32218,32415,32568,32678,32744,32767,32744,32678,
                    32568,32415,32218,31978,31695,31369,31002,30594,
                    30145,29657,29130,28565,27963,27325,26652,25945,
                    25206,24435,23635,22806,21950,21069,20164,19237,
                    18290,17324,16341,15343,14332,13310,12279,11240,
                    10196, 9148, 8100, 7053, 6010, 4972, 3941, 2920,
                     1910,  913,    0, -913,-1910,-2920,-3941,-4972,
                    -6010,-7053,-8100,-9148,-10196,-11240,-12279,-13310,
                    -14332,-15343,-16341,-17324,-18290,-19237,-20164,-21069,
                    -21950,-22806,-23635,-24435,-25206,-25945,-26652,-27325,
                    -27963,-28565,-29130,-29657,-30145,-30594,-31002,-31369,
                    -31695,-31978,-32218,-32415,-32568,-32678,-32744,-32767
                };
                wr = cos_tab[idx];
                wi = -sin_tab[idx];  /* negative for -2π/m convention */

                int a = k + i;
                int b = k + i + half_m;
                ur = re[a]; ui = im[a];
                tr = (int16_t)(((int32_t)re[b] * wr - (int32_t)im[b] * wi) >> 15);
                ti = (int16_t)(((int32_t)re[b] * wi + (int32_t)im[b] * wr) >> 15);
                re[a] = ur + tr;  im[a] = ui + ti;
                re[b] = ur - tr;  im[b] = ui - ti;
            }
        }
    }

    /* Compute power spectrum (magnitude squared) in re[] */
    for (i = 0; i < n; i++) {
        int32_t r = re[i], imv = im[i];
        re[i] = (int16_t)((r * r + imv * imv) >> 8);  /* scaled Q7 */
    }
}

/* ------------------------------------------------------------------ */
/*  Sample one 40 ms burst at 500 kSPS, decimate to 128 points, FFT    */
/*  Returns the 80–150 kHz band energy (sum of power bins).            */
/* ------------------------------------------------------------------ */
static uint32_t ae_capture_burst(void)
{
    /* We sample 20000 points but only keep every 156th for a 128-point
     * FFT (decimation factor ~156 → effective sample rate ~3.2 kHz
     * → Nyquist ~1.6 kHz).  That's far too low for 80–150 kHz!
     *
     * Correct approach: sample at 500 kSPS into a 128-sample buffer
     * directly (128 samples = 256 µs, which is fine for a burst).
     * The 40 ms "burst" consists of many such 128-sample windows;
     * we sum the band energy across all windows.
     */
    uint32_t total_band_energy = 0;
    uint32_t windows = (AE_SAMPLES_PER_BURST / AE_FFT_N);  /* ~156 windows */

    for (uint32_t w = 0; w < windows; w++) {
        for (int i = 0; i < AE_FFT_N; i++) {
            uint16_t raw = ae_adc_read_fast();
            /* Center around 2048 (mid-scale) and shift to Q15 */
            s_sample_buf[i] = (int16_t)((raw - 2048) << 3);
        }
        memcpy(s_fft_re, s_sample_buf, sizeof(s_sample_buf));
        memset(s_fft_im, 0, sizeof(s_fft_im));

        fft_q15(s_fft_re, s_fft_im, AE_FFT_N);

        /* Sum power in 80–150 kHz band (bins 20..38) */
        uint32_t band = 0;
        for (int b = AE_BAND_LO_BIN; b <= AE_BAND_HI_BIN; b++) {
            band += (uint16_t)s_fft_re[b];
        }
        total_band_energy += band;
    }

    return total_band_energy / windows;  /* average per-window band energy */
}

/* ------------------------------------------------------------------ */
/*  Update the rolling baseline                                       */
/* ------------------------------------------------------------------ */
static void ae_update_baseline(uint32_t band_energy)
{
    if (s_baseline_count < AE_BASELINE_LEN) {
        s_baseline[s_baseline_idx] = band_energy;
        s_baseline_sum += band_energy;
        s_baseline_idx = (s_baseline_idx + 1) % AE_BASELINE_LEN;
        s_baseline_count++;
    } else {
        s_baseline_sum -= s_baseline[s_baseline_idx];
        s_baseline[s_baseline_idx] = band_energy;
        s_baseline_sum += band_energy;
        s_baseline_idx = (s_baseline_idx + 1) % AE_BASELINE_LEN;
    }
}

static uint32_t ae_baseline_mean(void)
{
    if (s_baseline_count == 0) return 0;
    return s_baseline_sum / s_baseline_count;
}

/* Rough σ estimate from the baseline (max - min)/4 heuristic */
static uint32_t ae_baseline_sigma(void)
{
    if (s_baseline_count < 4) return 1000;  /* default until we have data */
    uint32_t mn = 0xFFFFFFFF, mx = 0;
    for (uint8_t i = 0; i < s_baseline_count; i++) {
        if (s_baseline[i] < mn) mn = s_baseline[i];
        if (s_baseline[i] > mx) mx = s_baseline[i];
    }
    return (mx - mn) / 4 + 1;
}

/* ------------------------------------------------------------------ */
/*  Public: initialize the AE channel                                  */
/* ------------------------------------------------------------------ */
void acoustic_init(void)
{
    ae_adc_init();
    s_baseline_sum = 0;
    s_baseline_idx = 0;
    s_baseline_count = 0;
    s_last_band_energy = 0;
    s_cumulative_energy = 0;
    s_consecutive_hits = 0;
    s_nucleation_detected = 0;
    s_nucleation_time_s = 0;
}

/* ------------------------------------------------------------------ */
/*  Public: arm the AE channel (called when conditions are right)      */
/*  Returns AE_STATUS_IDLE/ARMED/NUCLEATION                            */
/* ------------------------------------------------------------------ */
uint8_t acoustic_check(void)
{
    uint32_t energy = ae_capture_burst();
    s_last_band_energy = energy;

    /* Update baseline (only if not currently in nucleation) */
    if (!s_nucleation_detected) {
        ae_update_baseline(energy);
    }

    uint32_t mean = ae_baseline_mean();
    uint32_t sigma = ae_baseline_sigma();
    uint32_t threshold = mean + 6 * sigma;

    if (s_baseline_count < 4) {
        /* Still learning baseline; just report armed */
        return AE_STATUS_ARMED;
    }

    if (energy > threshold) {
        s_consecutive_hits++;
        s_cumulative_energy += (energy - mean);
        if (s_consecutive_hits >= 2 && !s_nucleation_detected) {
            s_nucleation_detected = 1;
            s_nucleation_time_s = g_rtc_seconds;
        }
    } else {
        if (s_consecutive_hits > 0) s_consecutive_hits--;
    }

    return s_nucleation_detected ? AE_STATUS_NUCLEATION : AE_STATUS_ARMED;
}

/* ------------------------------------------------------------------ */
/*  Public: disarm and reset nucleation state                          */
/* ------------------------------------------------------------------ */
void acoustic_reset(void)
{
    s_nucleation_detected = 0;
    s_consecutive_hits = 0;
    s_cumulative_energy = 0;
}

/* ------------------------------------------------------------------ */
/*  Public: query state                                                */
/* ------------------------------------------------------------------ */
uint32_t acoustic_get_cumulative_energy(void)
{
    return s_cumulative_energy;
}

uint32_t acoustic_get_last_band_energy(void)
{
    return s_last_band_energy;
}

uint8_t  acoustic_is_nucleation_detected(void)
{
    return s_nucleation_detected;
}

uint32_t acoustic_get_nucleation_time(void)
{
    return s_nucleation_time_s;
}