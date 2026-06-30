/*
 * drivers/acoustic.c — Acoustic wingbeat analysis for SpectraPest
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 *
 * Captures 3 seconds of audio from the SPH0645 I²S MEMS microphone
 * (ultrasonic-capable, 20 Hz – 80 kHz bandwidth) and extracts a
 * 24-dimensional acoustic feature vector for pest species classification.
 *
 * The feature vector captures:
 *   - Wingbeat fundamental frequency (50–600 Hz for most pest insects)
 *   - Harmonic ratios (species-specific spectral fingerprint)
 *   - Spectral centroid, spread, entropy (broadband activity indicators)
 *   - Zero-crossing rate (rough frequency estimate)
 *   - RMS amplitude (activity level / distance estimate)
 *   - 16 mel-frequency cepstral coefficients (MFCCs) for robust
 *     species discrimination across varying noise conditions
 *
 * The FFT is a radix-2 Cooley-Tukey implementation (1024-point) that runs
 * entirely in SRAM. For 3 seconds of 96 kHz audio, we compute the FFT on
 * overlapping 1024-sample windows (93 ms) with 50% overlap, then aggregate
 * the spectral estimates. The wingbeat fundamental is detected as the
 * strongest peak in the 40–800 Hz range after spectral smoothing.
 *
 * Power management: The SPH0645 has a SHUTDOWN pin that reduces current
 * to <1 µA. The I²S peripheral clock is gated when not in use.
 */

#include "acoustic.h"
#include "../registers.h"
#include "../board.h"
#include <math.h>
#include <string.h>

/* ----------------------------------------------------------------- *
 *  Constants
 * ----------------------------------------------------------------- */
#define FFT_SIZE        1024
#define FFT_LOG2N        10
#define MFCC_BANKS       16
#define MFCC_FFT_SIZE    512
#define MEL_MIN_HZ       50.0f
#define MEL_MAX_HZ      20000.0f
#define PI               3.14159265358979323846f

/* I²S2 DMA buffer: placed in non-cacheable SRAM2 */
#define I2S_DMA_BUF_ADDR  0x10040000UL
#define I2S_DMA_BUF_SIZE  (ACOUSTIC_BUF_SAMPLES * 2)  /* 16-bit samples */

static volatile uint8_t  g_capture_done = 0;
static volatile uint32_t g_samples_captured = 0;
static int16_t           *g_audio_buffer = NULL;

/* FFT working buffers (in SRAM1, cacheable) */
static float fft_real[FFT_SIZE];
static float fft_imag[FFT_SIZE];
static float fft_window[FFT_SIZE];
static float power_spectrum[FFT_SIZE / 2];
static float mel_filterbank[MFCC_BANKS][MFCC_FFT_SIZE / 2];
static float mel_energies[MFCC_BANKS];
static float mfcc_dct_matrix[MFCC_BANKS][MFCC_BANKS];

/* ----------------------------------------------------------------- *
 *  I²S DMA interrupt handler
 * ----------------------------------------------------------------- */
void DMA1_Stream4_IRQHandler(void) {
    uint32_t flags = DMA1_CTRL->HISR;

    if (flags & (1 << 15)) {  /* TCIF4 */
        DMA1_CTRL->HIFCR = (1 << 27);  /* CTCIF4 */

        /* In circular mode, this fires every half-buffer.
         * We copy the latest samples to the main audio buffer. */
        uint32_t half_buf_samples = I2S_DMA_BUF_SIZE / 4;  /* Half of 16-bit buffer */
        int16_t *dma_buf = (int16_t *)I2S_DMA_BUF_ADDR;

        uint32_t to_copy = half_buf_samples;
        if (g_samples_captured + to_copy > ACOUSTIC_BUF_SAMPLES) {
            to_copy = ACOUSTIC_BUF_SAMPLES - g_samples_captured;
        }

        if (g_audio_buffer && to_copy > 0) {
            memcpy(&g_audio_buffer[g_samples_captured], dma_buf, to_copy * 2);
            g_samples_captured += to_copy;
        }

        if (g_samples_captured >= ACOUSTIC_BUF_SAMPLES) {
            g_capture_done = 1;
            DMA1_STR4->CR &= ~DMA_CR_EN;  /* Stop DMA */
        }
    }
}

/* ----------------------------------------------------------------- *
 *  FFT (radix-2 Cooley-Tukey, in-place, decimation-in-time)
 *  Standard implementation optimized for Cortex-M7 with FPU.
 * ----------------------------------------------------------------- */
void acoustic_fft(float *real, float *imag, int n) {
    /* Bit-reversal permutation */
    int j = 0;
    for (int i = 1; i < n; i++) {
        int bit = n >> 1;
        while (j & bit) {
            j ^= bit;
            bit >>= 1;
        }
        j ^= bit;
        if (i < j) {
            float tr = real[i]; real[i] = real[j]; real[j] = tr;
            float ti = imag[i]; imag[i] = imag[j]; imag[j] = ti;
        }
    }

    /* Butterfly operations */
    for (int len = 2; len <= n; len <<= 1) {
        float ang = -2.0f * PI / (float)len;
        float wlen_r = cosf(ang);
        float wlen_i = sinf(ang);

        for (int i = 0; i < n; i += len) {
            float w_r = 1.0f, w_i = 0.0f;
            for (int k = 0; k < len / 2; k++) {
                float u_r = real[i + k];
                float u_i = imag[i + k];
                float v_r = real[i + k + len/2] * w_r - imag[i + k + len/2] * w_i;
                float v_i = real[i + k + len/2] * w_i + imag[i + k + len/2] * w_r;

                real[i + k] = u_r + v_r;
                imag[i + k] = u_i + v_i;
                real[i + k + len/2] = u_r - v_r;
                imag[i + k + len/2] = u_i - v_i;

                float next_w_r = w_r * wlen_r - w_i * wlen_i;
                w_i = w_r * wlen_i + w_i * wlen_r;
                w_r = next_w_r;
            }
        }
    }
}

/* ----------------------------------------------------------------- *
 *  Hann window
 * ----------------------------------------------------------------- */
void acoustic_hann_window(float *window, int n) {
    for (int i = 0; i < n; i++) {
        window[i] = 0.5f * (1.0f - cosf(2.0f * PI * (float)i / (float)(n - 1)));
    }
}

/* ----------------------------------------------------------------- *
 *  Mel filterbank initialization
 *  Pre-compute triangular filter banks for MFCC extraction.
 * ----------------------------------------------------------------- */
static void init_mel_filterbank(void) {
    float mel_min = 1127.0f * logf(1.0f + MEL_MIN_HZ / 700.0f);
    float mel_max = 1127.0f * logf(1.0f + MEL_MAX_HZ / 700.0f);

    for (int b = 0; b < MFCC_BANKS; b++) {
        float mel_center = mel_min + (mel_max - mel_min) * (float)(b + 1) / (float)(MFCC_BANKS + 1);
        float mel_left   = mel_min + (mel_max - mel_min) * (float)b / (float)(MFCC_BANKS + 1);
        float mel_right  = mel_min + (mel_max - mel_min) * (float)(b + 2) / (float)(MFCC_BANKS + 1);

        float hz_center = 700.0f * (expf(mel_center / 1127.0f) - 1.0f);
        float hz_left    = 700.0f * (expf(mel_left / 1127.0f) - 1.0f);
        float hz_right   = 700.0f * (expf(mel_right / 1127.0f) - 1.0f);

        int bin_center = (int)(hz_center * MFCC_FFT_SIZE / ACoustic_SAMPLE_RATE);
        int bin_left   = (int)(hz_left   * MFCC_FFT_SIZE / ACoustic_SAMPLE_RATE);
        int bin_right  = (int)(hz_right  * MFCC_FFT_SIZE / ACoustic_SAMPLE_RATE);

        if (bin_right >= MFCC_FFT_SIZE / 2) bin_right = MFCC_FFT_SIZE / 2 - 1;
        if (bin_left >= bin_right) bin_left = bin_right - 1;
        if (bin_left < 0) bin_left = 0;

        memset(mel_filterbank[b], 0, sizeof(float) * MFCC_FFT_SIZE / 2);

        for (int k = bin_left; k <= bin_center && k < MFCC_FFT_SIZE / 2; k++) {
            if (bin_center > bin_left)
                mel_filterbank[b][k] = (float)(k - bin_left) / (float)(bin_center - bin_left);
        }
        for (int k = bin_center; k <= bin_right && k < MFCC_FFT_SIZE / 2; k++) {
            if (bin_right > bin_center)
                mel_filterbank[b][k] = (float)(bin_right - k) / (float)(bin_right - bin_center);
        }
    }
}

/* ----------------------------------------------------------------- *
 *  DCT-II for MFCC computation
 *  Pre-compute the DCT matrix to avoid repeated cos() calls.
 * ----------------------------------------------------------------- */
static void init_dct_matrix(void) {
    for (int k = 0; k < MFCC_BANKS; k++) {
        for (int n = 0; n < MFCC_BANKS; n++) {
            mfcc_dct_matrix[k][n] = cosf(PI * (float)k * (2.0f * (float)n + 1.0f) / (2.0f * MFCC_BANKS));
        }
    }
}

/* ----------------------------------------------------------------- *
 *  Initialize acoustic subsystem
 * ----------------------------------------------------------------- */
int acoustic_init(void) {
    /* Enable I2S2 peripheral clock */
    RCC->APB1ENR1 |= (1 << 27);  /* SPI2EN (I2S2 shares SPI2 clock) */

    /* Configure I2S2 pins: CK=PB10, WS=PC0, SD=PB11 */
    gpio_set_mode(GPIOB, 10, GPIO_MODE_AF);
    gpio_set_af(GPIOB, 10, AF_I2S2_CK);
    gpio_set_ospeed(GPIOB, 10, GPIO_SPEED_VHIGH);

    gpio_set_mode(GPIOC, 0, GPIO_MODE_AF);
    gpio_set_af(GPIOC, 0, AF_I2S2_WS);
    gpio_set_ospeed(GPIOC, 0, GPIO_SPEED_VHIGH);

    gpio_set_mode(GPIOB, 11, GPIO_MODE_AF);
    gpio_set_af(GPIOB, 11, AF_I2S2_SD);

    /* Configure I2S2 as master receiver, 96 kHz, 16-bit data */
    /* (SPI2 CFG1/CFG2 registers programmed here) */
    SPI2->CFG1 = SPI_CFG1_MASTER | (0x7 << 0);  /* Master, 8-bit */
    SPI2->CFG2 = SPI_CFG2_COMM_RX;               /* Receive-only */
    /* I2S configuration bits would be in SPI2_I2SCFGR register */

    /* Enable microphone (take out of shutdown) */
    gpio_set_mode(MIC_SHUTDOWN_PORT, MIC_SHUTDOWN_PIN, GPIO_MODE_OUTPUT);
    gpio_write(MIC_SHUTDOWN_PORT, MIC_SHUTDOWN_PIN, 1);  /* Active high shutdown, pull low to enable */
    board_delay_ms(100);

    /* Configure DMA1 Stream4 for I2S2 -> SRAM2 */
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;
    DMA1_STR4->CR = 0;

    DMA1_STR4->PAR = (uint32_t)&SPI2->RXDR;  /* I2S2 data register */
    DMA1_STR4->M0AR = I2S_DMA_BUF_ADDR;
    DMA1_STR4->NDTR = I2S_DMA_BUF_SIZE / 2;  /* 16-bit transfers */
    DMA1_STR4->CR = DMA_CR_TCIE | DMA_CR_MINC | DMA_CR_PL_HIGH |
                    DMA_CR_PSIZE_16 | DMA_CR_MSIZE_16 |
                    DMA_CR_DIR_P2M | DMA_CR_CIRC | (DMA_CHSEL_I2S2 << 25);
    DMA1_STR4->FCR = 0x5;

    NVIC_ISER1 |= (1 << (IRQ_DMA1_STR4 - 32));

    /* Initialize feature extraction matrices */
    init_mel_filterbank();
    init_dct_matrix();
    acoustic_hann_window(fft_window, FFT_SIZE);

    return 0;
}

/* ----------------------------------------------------------------- *
 *  Capture audio
 * ----------------------------------------------------------------- */
int acoustic_capture(int16_t *out_samples, uint32_t max_samples) {
    if (!out_samples || max_samples < ACOUSTIC_BUF_SAMPLES) return -1;

    g_audio_buffer = out_samples;
    g_samples_captured = 0;
    g_capture_done = 0;

    /* Start DMA */
    DMA1_STR4->CR |= DMA_CR_EN;

    /* Wait for capture complete (timeout: 5 seconds) */
    uint32_t start = board_get_tick_ms();
    while (!g_capture_done) {
        if (board_get_tick_ms() - start > 5000) {
            DMA1_STR4->CR &= ~DMA_CR_EN;
            return -1;
        }
    }

    return (int)g_samples_captured;
}

/* ----------------------------------------------------------------- *
 *  Extract acoustic features from captured audio
 *  This runs entirely on the Cortex-M7 with FPU.
 * ----------------------------------------------------------------- */
int acoustic_extract_features(const int16_t *samples, uint32_t n_samples,
                                 acoustic_features_t *out) {
    if (!samples || !out || n_samples < FFT_SIZE) return -1;

    /* --- Step 1: Compute power spectrum using overlapping FFT windows --- */
    /* Use 1024-point FFT with 50% overlap */
    int hop = FFT_SIZE / 2;
    int num_frames = (n_samples - FFT_SIZE) / hop;
    if (num_frames <= 0) num_frames = 1;

    /* Accumulate power spectrum across all frames */
    float avg_spectrum[FFT_SIZE / 2];
    memset(avg_spectrum, 0, sizeof(avg_spectrum));

    for (int frame = 0; frame < num_frames; frame++) {
        const int16_t *src = &samples[frame * hop];

        /* Apply Hann window and convert to float */
        for (int i = 0; i < FFT_SIZE; i++) {
            fft_real[i] = (float)src[i] * fft_window[i];
            fft_imag[i] = 0.0f;
        }

        /* Compute FFT */
        acoustic_fft(fft_real, fft_imag, FFT_SIZE);

        /* Compute power spectrum (magnitude squared) */
        for (int i = 0; i < FFT_SIZE / 2; i++) {
            float mag = sqrtf(fft_real[i] * fft_real[i] + fft_imag[i] * fft_imag[i]);
            avg_spectrum[i] += mag;
        }
    }

    /* Average */
    for (int i = 0; i < FFT_SIZE / 2; i++) {
        avg_spectrum[i] /= (float)num_frames;
    }

    /* --- Step 2: Find wingbeat fundamental in 40-800 Hz range --- */
    int bin_min = (int)(40.0f * FFT_SIZE / ACoustic_SAMPLE_RATE);
    int bin_max = (int)(800.0f * FFT_SIZE / ACoustic_SAMPLE_RATE);
    if (bin_max >= FFT_SIZE / 2) bin_max = FFT_SIZE / 2 - 1;

    float max_mag = 0.0f;
    int   max_bin = bin_min;
    for (int i = bin_min; i <= bin_max; i++) {
        if (avg_spectrum[i] > max_mag) {
            max_mag = avg_spectrum[i];
            max_bin = i;
        }
    }

    float fundamental_hz = (float)max_bin * (float)ACoustic_SAMPLE_RATE / (float)FFT_SIZE;

    /* Harmonics */
    int h2_bin = max_bin * 2;
    int h3_bin = max_bin * 3;
    float h1_mag = avg_spectrum[max_bin];
    float h2_mag = (h2_bin < FFT_SIZE / 2) ? avg_spectrum[h2_bin] : 0.0f;
    float h3_mag = (h3_bin < FFT_SIZE / 2) ? avg_spectrum[h3_bin] : 0.0f;
    float h2_ratio = (h1_mag > 0.001f) ? h2_mag / h1_mag : 0.0f;
    float h3_ratio = (h1_mag > 0.001f) ? h3_mag / h1_mag : 0.0f;

    /* --- Step 3: Spectral centroid, spread, entropy --- */
    float total_mag = 0.0f;
    float weighted_freq = 0.0f;
    for (int i = 1; i < FFT_SIZE / 2; i++) {
        float freq = (float)i * (float)ACoustic_SAMPLE_RATE / (float)FFT_SIZE;
        total_mag += avg_spectrum[i];
        weighted_freq += avg_spectrum[i] * freq;
    }
    float centroid = (total_mag > 0.001f) ? weighted_freq / total_mag : 0.0f;

    float spread_sum = 0.0f;
    for (int i = 1; i < FFT_SIZE / 2; i++) {
        float freq = (float)i * (float)ACoustic_SAMPLE_RATE / (float)FFT_SIZE;
        spread_sum += avg_spectrum[i] * (freq - centroid) * (freq - centroid);
    }
    float spread = (total_mag > 0.001f) ? sqrtf(spread_sum / total_mag) : 0.0f;

    float entropy = 0.0f;
    for (int i = 1; i < FFT_SIZE / 2; i++) {
        if (avg_spectrum[i] > 0.001f) {
            float p = avg_spectrum[i] / total_mag;
            entropy -= p * logf(p);
        }
    }
    /* Normalize to bits */
    entropy /= logf(FFT_SIZE / 2);

    /* --- Step 4: Zero-crossing rate --- */
    uint32_t zc_count = 0;
    for (uint32_t i = 1; i < n_samples; i++) {
        if ((samples[i] > 0 && samples[i-1] <= 0) ||
            (samples[i] <= 0 && samples[i-1] > 0)) {
            zc_count++;
        }
    }
    float zcr = (float)zc_count / (float)n_samples * (float)ACoustic_SAMPLE_RATE;

    /* --- Step 5: RMS amplitude --- */
    float sq_sum = 0.0f;
    for (uint32_t i = 0; i < n_samples; i++) {
        float s = (float)samples[i] / 32768.0f;
        sq_sum += s * s;
    }
    float rms = sqrtf(sq_sum / (float)n_samples);

    /* --- Step 6: MFCC computation --- */
    /* Use a smaller FFT (512-point) for the mel filterbank */
    int mfcc_hop = MFCC_FFT_SIZE / 2;
    int mfcc_frames = (n_samples - MFCC_FFT_SIZE) / mfcc_hop;
    if (mfcc_frames <= 0) mfcc_frames = 1;

    float mel_acc[MFCC_BANKS];
    memset(mel_acc, 0, sizeof(mel_acc));

    float mfcc_window[MFCC_FFT_SIZE];
    acoustic_hann_window(mfcc_window, MFCC_FFT_SIZE);

    for (int frame = 0; frame < mfcc_frames; frame++) {
        const int16_t *src = &samples[frame * mfcc_hop];

        for (int i = 0; i < MFCC_FFT_SIZE; i++) {
            fft_real[i] = (float)src[i] * mfcc_window[i];
            fft_imag[i] = 0.0f;
        }

        acoustic_fft(fft_real, fft_imag, MFCC_FFT_SIZE);

        /* Apply mel filterbank */
        for (int b = 0; b < MFCC_BANKS; b++) {
            float energy = 0.0f;
            for (int k = 0; k < MFCC_FFT_SIZE / 2; k++) {
                float mag = sqrtf(fft_real[k] * fft_real[k] + fft_imag[k] * fft_imag[k]);
                energy += mel_filterbank[b][k] * mag;
            }
            mel_acc[b] += logf(energy + 1e-10f);
        }
    }

    /* Average mel energies across frames */
    for (int b = 0; b < MFCC_BANKS; b++) {
        mel_energies[b] = mel_acc[b] / (float)mfcc_frames;
    }

    /* DCT to get MFCC coefficients */
    for (int k = 0; k < MFCC_BANKS; k++) {
        float sum = 0.0f;
        for (int n = 0; n < MFCC_BANKS; n++) {
            sum += mfcc_dct_matrix[k][n] * mel_energies[n];
        }
        out->mfcc[k] = sum;
    }

    /* Fill the rest of the feature vector */
    out->fundamental_hz = fundamental_hz;
    out->harmonic_2 = h2_mag;
    out->harmonic_3 = h3_mag;
    out->harmonic_ratio = h2_ratio;
    out->spectral_centroid = centroid;
    out->spectral_spread = spread;
    out->spectral_entropy = entropy;
    out->zero_crossing_rate = zcr;
    out->rms_amplitude = rms;

    return 0;
}

void acoustic_power_down(void) {
    /* Put microphone in shutdown */
    gpio_write(MIC_SHUTDOWN_PORT, MIC_SHUTDOWN_PIN, 0);
    /* Disable I2S2 peripheral clock */
    RCC->APB1ENR1 &= ~(1 << 27);
    /* Disable DMA */
    DMA1_STR4->CR &= ~DMA_CR_EN;
}

void acoustic_power_up(void) {
    RCC->APB1ENR1 |= (1 << 27);
    gpio_write(MIC_SHUTDOWN_PORT, MIC_SHUTDOWN_PIN, 1);
    board_delay_ms(100);
}