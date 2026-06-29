/*
 * drivers/acoustic.c — PDM microphone capture + FFT feature extraction
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 *
 * Captures 512-sample blocks from the SPH0645LU4H-1 PDM MEMS microphone via
 * I2S2, decimates to 16 kHz PCM, computes a 512-point radix-2 FFT, and
 * extracts the features consumed by colony_classify() in main.c.
 *
 * The I2S peripheral is clocked from a timer-derived PDM bit clock; the
 * DMA writes PDM samples to a buffer and we apply a simple CIC decimator
 * (divide by 64) to get 16 kHz PCM. For firmware brevity and to avoid the
 * full PDM→PCM filter chain, this driver reads pre-decimated 16-bit PCM
 * samples directly from the I2S data register (the SPH0645 in PCM mode
 * outputs decimated samples at the configured sample rate).
 */
#include "acoustic.h"
#include "../registers.h"
#include "../board.h"
#include <string.h>

/* ---- DWT cycle counter (for µs delays) ----------------------------- */
#define DWT_BASE        0xE0001000UL
#define DWT_CTRL        (*(volatile uint32_t *)(DWT_BASE + 0x00))
#define DWT_CYCCNT      (*(volatile uint32_t *)(DWT_BASE + 0x04))
#define DEMCR           (*(volatile uint32_t *)(0xE000EDFCUL))
#define DEMCR_TRCENA    (1U << 24)

volatile uint32_t g_dwt_cycles_per_us = 80;  /* 80 MHz core */

volatile uint32_t dwt_cycle_count(void) { return DWT_CYCCNT; }

static int16_t  g_pcm[FFT_N];          /* time-domain samples */
static int16_t  g_fft_re[FFT_N];       /* real part (also input) */
static int16_t  g_fft_im[FFT_N];       /* imag part */
static uint16_t g_mag[FFT_N/2];        /* magnitude spectrum */

/* ---- I2S2 / PDM microphone setup ----------------------------------- */
/*
 * We configure I2S2 in master receive mode, 16-bit data, at a sample rate
 * that the SPH0645 supports. The SPH0645 outputs PDM on the I2S data line
 * and uses WS (LRCLK) and SCK (bit clock). We set SCK = 1.024 MHz and
 * WS = 16 kHz (SCK/64), which gives us 16 kHz PCM after the mic's internal
 * decimation by 64.
 */

static void i2s2_init(void)
{
    /* Enable I2S2 via SPI2 clock (I2S2 shares SPI2 clock enable) */
    RCC_APB1ENR1 |= RCC_APB1ENR1_SPI2EN;

    /* PA10 (MIC_EN) as GPIO output, high to power the mic */
    GPIOA->MODER &= ~(3U << (10*2));
    GPIOA->MODER |=  (GPIO_MODE_OUTPUT << (10*2));
    GPIOA->BSRR = (1U << 10);  /* MIC_EN high */

    /* PA11 (I2S2_SD), PA12 (I2S2_WS), PB3 (I2S2_CK) as AF5 */
    GPIOA->MODER &= ~((3U << (11*2)) | (3U << (12*2)));
    GPIOA->MODER |=  (GPIO_MODE_AF << (11*2)) | (GPIO_MODE_AF << (12*2));
    GPIOA->AFRL &= ~((0xFU << (11*4)) | (0xFU << (12*4)));
    GPIOA->AFRL |=  (5U << (11*4)) | (5U << (12*4));   /* AF5 = I2S2 */

    GPIOB->MODER &= ~(3U << (3*2));
    GPIOB->MODER |=  (GPIO_MODE_AF << (3*2));
    GPIOB->AFRL &= ~(0xFU << (3*4));
    GPIOB->AFRL |=  (5U << (3*4));   /* AF5 = I2S2_CK */
    GPIOB->OSPEEDR |= (3U << (3*2));  /* high speed */

    /* Configure I2S2: master Rx, 16-bit, standard Phillips, 16 kHz */
    /* I2SCFGR: I2SMOD=1, I2SCFG=11 (master Rx), I2SSTD=00, PCMSYNC=0,
       DATLEN=00 (16b), CHLEN=0 (16b), CKPOL=0 */
    SPI2->I2SCFGR = (1U << 0)          /* I2SMOD */
                  | (3U << 8)          /* I2SCFG = master Rx */
                  | (0U << 4);        /* DATLEN 16-bit, CHLEN 16-bit */

    /* I2SPR: prescaler for 16 kHz with 32 MHz peripheral clock.
     * I2S clock = PCLK / (2 * prescaler). For 16 kHz WS with 32-bit frame:
     *   SCK = WS * 32 = 512 kHz; I2S clock = 2*SCK = 1.024 MHz.
     *   prescaler = PCLK / (2 * 1.024M) = 32M / 2.048M ≈ 15.625 → 16
     * MCKOE=0 (no MCK output). */
    SPI2->I2SPR = 16U | (0U << 8);  /* prescaler 16, MCKOE off */

    /* Enable I2S */
    SPI2->I2SCFGR |= (1U << 10);  /* I2SE */
}

/* Capture one block of FFT_N samples via polling (simple, robust). */
static void capture_block(void)
{
    for (int i = 0; i < FFT_N; i++) {
        uint32_t timeout = 1000000;
        /* Wait for RXNE */
        while (!(SPI2->SR & SPI_SR_RXNE) && timeout--) { }
        int16_t sample = (int16_t)SPI2->DR;
        g_pcm[i] = sample;
    }
}

/* ---- Radix-2 decimation-in-time FFT (16-bit fixed-point) ----------- */
/* Bit-reversal permutation. */
static void fft_bit_reverse(int16_t *re, int16_t *im, int n)
{
    int j = 0;
    for (int i = 1; i < n; i++) {
        int bit = n >> 1;
        while (j & bit) { j ^= bit; bit >>= 1; }
        j ^= bit;
        if (i < j) {
            int16_t tr = re[i]; re[i] = re[j]; re[j] = tr;
            int16_t ti = im[i]; im[i] = im[j]; im[j] = ti;
        }
    }
}

/*
 * In-place radix-2 DIT FFT on 16-bit fixed-point arrays.
 * Uses Q15 twiddle factors precomputed at runtime.
 * n must be a power of 2.
 */
static void fft_radix2(int16_t *re, int16_t *im, int n)
{
    fft_bit_reverse(re, im, n);

    for (int len = 2; len <= n; len <<= 1) {
        int half = len >> 1;
        /* Twiddle factor step */
        for (int k = 0; k < half; k++) {
            /* angle = -2*pi*k/len */
            int32_t angle = -2 * 314159 * k / len;  /* scaled by 1000 */
            /* Q15 cosine/sine: use int16 range [-32768,32767] */
            int16_t wr = (int16_t)(32767 * cos_q15(angle));
            int16_t wi = (int16_t)(32767 * sin_q15(angle));

            for (int i = 0; i < n; i += len) {
                int idx = i + k;
                /* t = w * re[i+k] + j*w * im[i+k] (complex mult) */
                int32_t tr = (int32_t)re[idx + half] * wr - (int32_t)im[idx + half] * wi;
                int32_t ti = (int32_t)re[idx + half] * wi + (int32_t)im[idx + half] * wr;
                tr >>= 15; ti >>= 15;  /* normalize Q15 */

                re[idx + half] = re[idx] - (int16_t)tr;
                im[idx + half] = im[idx] - (int16_t)ti;
                re[idx] = re[idx] + (int16_t)tr;
                im[idx] = im[idx] + (int16_t)ti;
            }
        }
    }
}

/* Simple fixed-point trig (avoids float lib for determinism). */
int32_t cos_q15(int32_t angle_milli)
{
    /* angle in milli-radians; reduce to [-π, π] → [-3142, 3142] */
    int32_t a = angle_milli % 6283;
    if (a > 3141) a -= 6283;
    if (a < -3141) a += 6283;
    /* Taylor: cos(x) ≈ 1 - x²/2 + x⁴/24 (x in rad) */
    int32_t x = a;          /* milli-rad */
    int64_t x2 = (int64_t)x * x / 1000000;   /* x² in milli² → rad² */
    int64_t x4 = x2 * x2;
    int64_t cosv = 32767 - 32767 * x2 / 2 + 32767 * x4 / 24;
    if (cosv > 32767) cosv = 32767;
    if (cosv < -32768) cosv = -32768;
    return (int32_t)cosv;
}

int32_t sin_q15(int32_t angle_milli)
{
    /* sin(x) ≈ x - x³/6 + x⁵/120 */
    int32_t a = angle_milli % 6283;
    if (a > 3141) a -= 6283;
    if (a < -3141) a += 6283;
    int32_t x = a;
    int64_t x3 = (int64_t)x * x * x / 1000000000;  /* milli³ → rad³ */
    int64_t x5 = x3 * x * x / 1000000;
    int64_t sinv = 32767 * x / 1000 - 32767 * x3 / 6 + 32767 * x5 / 120;
    if (sinv > 32767) sinv = 32767;
    if (sinv < -32768) sinv = -32768;
    return (int32_t)sinv;
}

/* Compute magnitude spectrum from re/im. */
static void compute_magnitude(int n)
{
    for (int i = 0; i < n/2; i++) {
        int32_t r = g_fft_re[i];
        int32_t im = g_fft_im[i];
        int32_t mag = (r < 0 ? -r : r) + (im < 0 ? -im : im);  /* L1 approx */
        g_mag[i] = (uint16_t)(mag > 65535 ? 65535 : mag);
    }
}

/* ---- Feature extraction -------------------------------------------- */

/*
 * Extract the three features used by the colony classifier:
 *   - dominant frequency f0 (Hz)
 *   - high-band energy ratio r_hi (energy >400Hz / total)
 *   - loudness coefficient of variation cv_loud (across MIC_FRAMES_PER_CYCLE)
 */
void acoustic_capture_features(acoustic_features_t *out, uint8_t frames)
{
    int32_t total_energy = 0;
    int32_t hi_energy = 0;
    int32_t loudness[8];
    int32_t dominant_freq_accum = 0;
    int32_t dominant_count = 0;
    uint32_t hi_bin = (400UL * FFT_N) / (2 * MIC_SAMPLE_RATE_HZ);  /* ~ bin 12 */

    for (uint8_t f = 0; f < frames; f++) {
        capture_block();

        /* Apply Hann window to reduce spectral leakage */
        for (int i = 0; i < FFT_N; i++) {
            int32_t hann = 32767 * (1 - cos_q15(2 * 3141 * i / FFT_N)) / 2;
            g_fft_re[i] = (int16_t)((int32_t)g_pcm[i] * hann >> 15);
            g_fft_im[i] = 0;
        }

        fft_radix2(g_fft_re, g_fft_im, FFT_N);
        compute_magnitude(FFT_N);

        /* Find dominant frequency bin */
        uint16_t max_mag = 0;
        int max_bin = 0;
        for (int i = 1; i < FFT_N/2; i++) {
            if (g_mag[i] > max_mag) { max_mag = g_mag[i]; max_bin = i; }
        }
        dominant_freq_accum += max_bin * (MIC_SAMPLE_RATE_HZ / FFT_N);
        dominant_count++;

        /* Band energies */
        int32_t frame_total = 0, frame_hi = 0;
        for (int i = 1; i < FFT_N/2; i++) {
            frame_total += g_mag[i];
            if (i >= (int)hi_bin) frame_hi += g_mag[i];
        }
        total_energy += frame_total;
        hi_energy += frame_hi;
        loudness[f] = frame_total;
    }

    /* Aggregate features */
    out->dominant_hz = dominant_count ? (uint16_t)(dominant_freq_accum / dominant_count) : 0;
    out->r_hi = total_energy ? (uint8_t)((hi_energy * 255) / total_energy) : 0;

    /* CV of loudness = stddev / mean */
    int32_t mean = 0;
    for (uint8_t f = 0; f < frames; f++) mean += loudness[f];
    mean /= frames;
    int32_t var = 0;
    for (uint8_t f = 0; f < frames; f++) {
        int32_t d = loudness[f] - mean;
        var += d * d;
    }
    var /= frames;
    /* cv = sqrt(var)/mean; approximate sqrt with integer */
    int32_t stddev = isqrt32(var);
    out->cv_loud = mean ? (uint8_t)((stddev * 255) / mean) : 255;
    out->total_energy = total_energy;
}

/* Integer square root (Newton's method). */
int32_t isqrt32(int32_t n)
{
    if (n <= 0) return 0;
    int32_t x = n;
    int32_t y = (x + 1) / 2;
    while (y < x) { x = y; y = (x + n / x) / 2; }
    return x;
}

void mic_enable(void)
{
    GPIOA->BSRR = (1U << 10);  /* MIC_EN high */
    for (volatile int i = 0; i < MIC_ENABLE_WARMUP_MS * 8000; i++) { }
}

void mic_disable(void)
{
    GPIOA->BRR = (1U << 10);
}

void acoustic_init(void)
{
    /* Enable DWT cycle counter for µs delays */
    DEMCR |= DEMCR_TRCENA;
    DWT_CTRL |= 1;  /* CYCCNT enable */
    DWT_CYCCNT = 0;

    i2s2_init();
}