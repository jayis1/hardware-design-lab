/*
 * flicker.c — Photodiode flicker sampling & FFT analysis
 * LumiCast — Open-Source Circadian Light & SPD Analyzer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Implements high-speed photodiode sampling via ADC1 at 4 kHz with DMA,
 * then runs a 512-point radix-2 FFT to extract the dominant flicker
 * frequency and depth.  Computes percent flicker and flicker index
 * per IEEE 1789-2015, plus a safety rating.
 */

#include "flicker.h"
#include "timer_drv.h"
#include <math.h>

static uint16_t adc_buf[FLICKER_BUF_LEN];
static volatile bool capture_done = false;

/* FFT working buffers (512-point radix-2). */
static float fft_re[FLICKER_FFT_LEN];
static float fft_im[FLICKER_FFT_LEN];

/* ------------------------------------------------------------------ */

static void adc_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;

    /* Configure PA0 (ADC1_IN1) as analog. */
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    GPIOA->MODER |= (GPIO_MODE_ANALOG << (0U*2U));

    /* Enable ADC voltage regulator. */
    ADC1->CR = 0;
    ADC1->CR |= (1U << 28);   /* ADVREGEN */
    timer_delay_ms(1);

    /* Calibrate. */
    ADC1->CR |= ADC_CR_ADCAL;
    while (ADC1->CR & ADC_CR_ADCAL);

    /* Enable ADC. */
    ADC1->ISR = ADC_ISR_ADRDY;
    ADC1->CR |= ADC_CR_ADEN;
    while (!(ADC1->ISR & ADC_ISR_ADRDY));

    /* Configure: 12-bit, continuous, DMA, circular. */
    ADC1->CFGR = ADC_CFGR_CONT | ADC_CFGR_DMAEN | ADC_CFGR_DMACFG;
    ADC1->SMPR1 = (7U << (3U*3));  /* channel 1: 640.5 cycles sampling */

    /* Select channel 1. */
    ADC1->CFGR &= ~(0xFU << 6);   /* clear EXTSEL */
}

static void dma_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;

    /* Configure DMA1 Channel 1 for ADC1. */
    DMA1_Channel1->CCR = 0;
    DMA1_Channel1->CPAR = (uint32_t)&ADC1->DR;
    DMA1_Channel1->CMAR = (uint32_t)adc_buf;
    DMA1_Channel1->CNDTR = FLICKER_BUF_LEN;
    /* P2M, 16-bit, circular, minc. */
    DMA1_Channel1->CCR = DMA_CCR_DIR_P2M | DMA_CCR_CIRC | DMA_CCR_MINC
                       | DMA_CCR_PSIZE_16 | DMA_CCR_MSIZE_16
                       | DMA_CCR_PR_HI | DMA_CCR_TCIE;
    DMA1_Channel1->CCR |= DMA_CCR_EN;

    /* Select DMA1 Channel 1 = ADC1 (request mapping). */
    DMA1_CSELR = 0;
}

void flicker_init(void)
{
    adc_init();
    dma_init();
}

void flicker_start_capture(void)
{
    capture_done = false;

    /* Reset DMA. */
    DMA1_Channel1->CCR &= ~DMA_CCR_EN;
    DMA1_Channel1->CNDTR = FLICKER_BUF_LEN;
    DMA1_Channel1->CCR |= DMA_CCR_EN;

    /* Start ADC conversion. */
    ADC1->CR |= (1U << 2);  /* ADSTART */
}

int flicker_is_ready(void)
{
    /* Check if DMA has filled buffer (TCIF flag). */
    if (DMA1_Channel1->CNDTR == 0) {
        capture_done = true;
        return 1;
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/*  Radix-2 FFT implementation                                         */
/* ------------------------------------------------------------------ */

static void fft_bit_reverse(float *re, float *im, int n)
{
    int i, j = 0, k;
    float tr, ti;
    for (i = 1; i < n; i++) {
        k = n >> 1;
        while (j & k) {
            j ^= k;
            k >>= 1;
        }
        j |= k;
        if (i < j) {
            tr = re[i]; re[i] = re[j]; re[j] = tr;
            ti = im[i]; im[i] = im[j]; im[j] = ti;
        }
    }
}

static void fft_radix2(float *re, float *im, int n)
{
    int s, m, mh, i, j, k;
    float wr, wi, xr, xi, theta;

    fft_bit_reverse(re, im, n);

    for (s = 1; (1 << s) <= n; s++) {
        m = 1 << s;
        mh = m >> 1;
        for (i = 0; i < n; i += m) {
            for (j = i, k = 0; j < i + mh; j++, k++) {
                theta = -2.0f * 3.14159265359f * (float)k / (float)m;
                wr = cosf(theta);
                wi = sinf(theta);
                xr = re[j + mh] * wr - im[j + mh] * wi;
                xi = re[j + mh] * wi + im[j + mh] * wr;
                re[j + mh] = re[j] - xr;
                im[j + mh] = im[j] - xi;
                re[j] += xr;
                im[j] += xi;
            }
        }
    }
}

int flicker_get_result(flicker_result_t *out)
{
    float dc = 0.0f, max_val = 0.0f, min_val = 65535.0f;
    float max_mag = 0.0f, freq_bin = 0.0f;
    float sum_abs = 0.0f, sum_total = 0.0f;
    int i;
    float bin_hz = (float)FLICKER_SAMPLE_HZ / (float)FLICKER_FFT_LEN;

    /* Compute DC and AC peak/valley. */
    for (i = 0; i < FLICKER_BUF_LEN; i++) {
        float v = (float)adc_buf[i];
        dc += v;
        if (v > max_val) max_val = v;
        if (v < min_val) min_val = v;
        sum_total += v;
    }
    dc /= (float)FLICKER_BUF_LEN;

    /* Percent flicker = (max - min) / (max + min) * 100. */
    out->dc_level = (uint16_t)dc;
    out->ac_peak = (uint16_t)((max_val - min_val) / 2.0f);
    out->percent_flicker = (max_val - min_val) / (max_val + min_val) * 100.0f;

    /* Flicker index = area above DC / total area. */
    for (i = 0; i < FLICKER_BUF_LEN; i++) {
        float v = (float)adc_buf[i];
        float dev = v - dc;
        sum_abs += fabsf(dev);
    }
    out->flicker_index = sum_abs / (2.0f * sum_total);

    /* Flicker percent = AC peak / DC * 100. */
    out->flicker_percent = (dc > 1.0f) ? (out->ac_peak / dc * 100.0f) : 0.0f;

    /* Copy to FFT buffer, apply Hann window. */
    for (i = 0; i < FLICKER_FFT_LEN; i++) {
        float w = 0.5f * (1.0f - cosf(2.0f * 3.14159265359f * (float)i / (float)(FLICKER_FFT_LEN - 1)));
        fft_re[i] = (float)adc_buf[i] * w;
        fft_im[i] = 0.0f;
    }

    fft_radix2(fft_re, fft_im, FLICKER_FFT_LEN);

    /* Find peak magnitude in 1-2000 Hz range. */
    for (i = 1; i < FLICKER_FFT_LEN / 2; i++) {
        float freq = (float)i * bin_hz;
        if (freq < 1.0f || freq > 2000.0f) continue;
        float mag = sqrtf(fft_re[i]*fft_re[i] + fft_im[i]*fft_im[i]);
        if (mag > max_mag) {
            max_mag = mag;
            freq_bin = freq;
        }
    }

    out->fundamental_hz = freq_bin;
    out->fundamental_depth = (dc > 1.0f) ? (max_mag / dc * 100.0f) : 0.0f;

    /* Safety rating per IEEE 1789 "low risk" boundary:                */
    /*   percent_flicker < 0.08 * frequency (low-risk)                 */
    /*   percent_flicker < 0.0333 * frequency (no observable effect)    */
    if (freq_bin > 0.1f) {
        float low_risk_limit = 0.08f * freq_bin;
        float no_effect_limit = 0.0333f * freq_bin;
        if (out->percent_flicker < no_effect_limit) {
            out->safety_rating = 100.0f;
        } else if (out->percent_flicker < low_risk_limit) {
            /* Scale between no-effect and low-risk. */
            out->safety_rating = 50.0f + 50.0f * (1.0f -
                (out->percent_flicker - no_effect_limit) /
                (low_risk_limit - no_effect_limit));
        } else {
            /* Above low-risk. */
            out->safety_rating = 0.0f;
        }
    } else {
        /* DC — no flicker. */
        out->safety_rating = 100.0f;
        out->fundamental_hz = 0.0f;
        out->fundamental_depth = 0.0f;
    }

    /* Stop ADC. */
    ADC1->CR &= ~(1U << 2);  /* ADSTART off */
    ADC1->CR |= ADC_CR_DISON;

    return 0;
}