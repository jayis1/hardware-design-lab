/*
 * acoustic.c — Acoustic Bubble Detection via I2S MEMS Microphone
 *
 * Captures audio from a SPH-0645LM4H-B I2S MEMS microphone, computes a
 * 2048-point FFT every 500ms, and detects CO2 bubble events by matching
 * a spectral template (broadband impulse in the 1-8 kHz range with
 * 20-80ms duration). The bubble rate (bubbles/min) is a secondary
 * fermentation-vigor indicator.
 *
 * The SPH-0645LM4H-B outputs 24-bit left-justified data on the I2S
 * interface at a configurable sample rate. We use 16 kHz which gives
 * good frequency resolution for the 1-8 kHz bubble band while keeping
 * the FFT size manageable for the ESP32-S3.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: GPL-3.0
 */

#include "acoustic.h"
#include "../board.h"
#include "driver/i2s.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>
#include <string.h>

static const char *TAG = "acoustic";

#define FFT_SIZE            2048
#define FFT_LOG2            11
#define HALF_FFT            (FFT_SIZE / 2)
#define BUBBLE_FREQ_LO_BIN  4       /* ~1 kHz (bin = freq / (fs/N)) */
#define BUBBLE_FREQ_HI_BIN  32      /* ~8 kHz */
#define BUBBLE_ENERGY_THRESH 0.015f
#define BUBBLE_MIN_DURATION_MS 20
#define BUBBLE_MAX_DURATION_MS 80

/* Ring buffer for bubble events (last 60 seconds) */
#define BUBBLE_RING_LEN   120

/* FFT twiddle factors (precomputed) */
static float s_cos_table[FFT_SIZE];
static float s_sin_table[FFT_SIZE];

/* Audio buffers */
static int32_t s_audio_raw[FFT_SIZE];
static float   s_fft_real[FFT_SIZE];
static float   s_fft_imag[FFT_SIZE];

/* Bubble detection state */
static struct {
    uint64_t timestamps[BUBBLE_RING_LEN];
    int head;
    int count;
} s_bubble_ring;

static uint64_t millis(void)
{
    return esp_timer_get_time() / 1000;
}

/* ------------------------------------------------------------------- */
/* Precompute FFT twiddle factors                                     */
/* ------------------------------------------------------------------- */
static void fft_init_tables(void)
{
    for (int i = 0; i < FFT_SIZE; i++) {
        float angle = -2.0f * M_PI * (float)i / (float)FFT_SIZE;
        s_cos_table[i] = cosf(angle);
        s_sin_table[i] = sinf(angle);
    }
}

/* ------------------------------------------------------------------- */
/* In-place radix-2 Cooley-Tukey FFT (decimation-in-time)             */
/* ------------------------------------------------------------------- */
static void fft_compute(float *real, float *imag, int n)
{
    /* Bit-reversal permutation */
    int j = 0;
    for (int i = 1; i < n; i++) {
        int bit = n >> 1;
        for (; j & bit; bit >>= 1)
            j ^= bit;
        j ^= bit;
        if (i < j) {
            float tr = real[i]; real[i] = real[j]; real[j] = tr;
            float ti = imag[i]; imag[i] = imag[j]; imag[j] = ti;
        }
    }

    /* Butterfly operations */
    for (int len = 2; len <= n; len <<= 1) {
        int half = len >> 1;
        int step = FFT_SIZE / len;
        for (int i = 0; i < n; i += len) {
            int k = 0;
            for (int k1 = i; k1 < i + half; k1++) {
                int k2 = k1 + half;
                float tr = real[k2] * s_cos_table[k] - imag[k2] * s_sin_table[k];
                float ti = real[k2] * s_sin_table[k] + imag[k2] * s_cos_table[k];
                real[k2] = real[k1] - tr;
                imag[k2] = imag[k1] - ti;
                real[k1] += tr;
                imag[k1] += ti;
                k += step;
            }
        }
    }
}

/* ------------------------------------------------------------------- */
/* I2S initialization                                                  */
/* ------------------------------------------------------------------- */
int acoustic_init(void)
{
    i2s_config_t i2s_cfg = {
        .mode = I2S_MODE_MASTER | I2S_MODE_RX,
        .sample_rate = I2S_SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = I2S_DMA_BUF_COUNT,
        .dma_buf_len = I2S_DMA_BUF_LEN,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0,
    };

    i2s_pin_config_t pin_cfg = {
        .ws_io_num = I2S_WS,
        .bck_io_num = I2S_SCK,
        .data_in_io_num = I2S_SD,
        .data_out_io_num = I2S_PIN_NO_CHANGE,
    };

    esp_err_t err = i2s_driver_install(I2S_PORT, &i2s_cfg, 0, NULL);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "I2S install failed: %s", esp_err_to_name(err));
        return -1;
    }
    err = i2s_set_pin(I2S_PORT, &pin_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2S set pin failed: %s", esp_err_to_name(err));
        return -1;
    }

    fft_init_tables();
    memset(&s_bubble_ring, 0, sizeof(s_bubble_ring));

    ESP_LOGI(TAG, "Acoustic initialized: I2S %d Hz, FFT %d points",
             I2S_SAMPLE_RATE, FFT_SIZE);
    return 0;
}

/* ------------------------------------------------------------------- */
/* Read one FFT block from I2S                                         */
/* ------------------------------------------------------------------- */
static int read_audio_block(void)
{
    size_t bytes_read = 0;
    /* Read 32-bit samples (SPH-0645 outputs 24-bit in 32-bit frame) */
    int32_t raw32[FFT_SIZE];
    esp_err_t err = i2s_read(I2S_PORT, raw32, sizeof(raw32),
                              &bytes_read, pdMS_TO_TICKS(200));
    if (err != ESP_OK || bytes_read < FFT_SIZE * 4) {
        ESP_LOGW(TAG, "I2S read short: %d bytes", (int)bytes_read);
        return -1;
    }

    /* Convert to float, normalize (shift right 8 bits since 24-bit in 32) */
    for (int i = 0; i < FFT_SIZE; i++) {
        /* The SPH-0645 outputs data in the upper 24 bits of a 32-bit word.
         * Shift right by 8 to get the 24-bit value, then normalize to [-1, 1]. */
        int32_t sample = raw32[i] >> 8;
        s_audio_raw[i] = sample;
        s_fft_real[i] = (float)sample / 8388608.0f;  /* 2^23 */
        s_fft_imag[i] = 0.0f;
    }

    return 0;
}

/* ------------------------------------------------------------------- */
/* Detect bubble events from spectral energy                           */
/* ------------------------------------------------------------------- */
static void detect_bubble_event(uint64_t now, float bubble_band_energy,
                                float total_energy)
{
    /* A bubble produces a broadband impulse concentrated in the 1-8 kHz
     * band. We detect it when the band energy exceeds a threshold AND
     * the band contains a disproportionate fraction of the total energy. */

    float band_ratio = (total_energy > 1e-9f) ?
                        bubble_band_energy / total_energy : 0.0f;

    if (bubble_band_energy > BUBBLE_ENERGY_THRESH && band_ratio > 0.3f) {
        /* Record the event */
        s_bubble_ring.timestamps[s_bubble_ring.head] = now;
        s_bubble_ring.head = (s_bubble_ring.head + 1) % BUBBLE_RING_LEN;
        if (s_bubble_ring.count < BUBBLE_RING_LEN)
            s_bubble_ring.count++;
    }
}

/* ------------------------------------------------------------------- */
/* Compute bubble rate from ring buffer (events in last 60 seconds)   */
/* ------------------------------------------------------------------- */
static float compute_bubble_rate(uint64_t now)
{
    uint64_t cutoff = now - 60000;  /* 60 seconds ago */
    int count = 0;

    for (int i = 0; i < s_bubble_ring.count; i++) {
        int idx = (s_bubble_ring.head - 1 - i + BUBBLE_RING_LEN) %
                  BUBBLE_RING_LEN;
        if (s_bubble_ring.timestamps[idx] > cutoff)
            count++;
        else
            break;
    }

    return (float)count;  /* bubbles per minute (60-second window) */
}

/* ------------------------------------------------------------------- */
/* Main processing: read audio, FFT, detect bubbles, compute features */
/* ------------------------------------------------------------------- */
int acoustic_process(float *bubble_rate, float *centroid, float *rms)
{
    if (read_audio_block() != 0)
        return -1;

    /* Compute RMS level */
    float sum_sq = 0.0f;
    for (int i = 0; i < FFT_SIZE; i++) {
        float v = s_fft_real[i];
        sum_sq += v * v;
    }
    float rms_val = sqrtf(sum_sq / FFT_SIZE);
    *rms = rms_val;

    /* Apply Hann window to reduce spectral leakage */
    for (int i = 0; i < FFT_SIZE; i++) {
        float w = 0.5f * (1.0f - cosf(2.0f * M_PI * i / (FFT_SIZE - 1)));
        s_fft_real[i] *= w;
    }

    /* Compute FFT */
    fft_compute(s_fft_real, s_fft_imag, FFT_SIZE);

    /* Compute power spectrum for first half (Nyquist) */
    float power[HALF_FFT];
    float total_power = 0.0f;
    float bubble_band_power = 0.0f;
    float weighted_freq_sum = 0.0f;

    for (int i = 0; i < HALF_FFT; i++) {
        power[i] = s_fft_real[i] * s_fft_real[i] +
                   s_fft_imag[i] * s_fft_imag[i];
        total_power += power[i];

        float freq = (float)i * I2S_SAMPLE_RATE / FFT_SIZE;
        weighted_freq_sum += freq * power[i];

        /* Accumulate bubble band energy (1-8 kHz) */
        if (i >= BUBBLE_FREQ_LO_BIN && i <= BUBBLE_FREQ_HI_BIN)
            bubble_band_power += power[i];
    }

    /* Spectral centroid (perceptual "brightness" of the sound) */
    *centroid = (total_power > 1e-9f) ?
                weighted_freq_sum / total_power : 0.0f;

    /* Bubble detection */
    uint64_t now = millis();
    detect_bubble_event(now, bubble_band_power, total_power);
    *bubble_rate = compute_bubble_rate(now);

    return 0;
}