/*
 * main.c — TremorSense firmware main application
 * TremorSense — Wearable Tremor Characterization Band
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 *
 * This is the main firmware entry point for TremorSense.  It initializes
 * all hardware peripherals, runs the tremor detection DSP + edge ML pipeline,
 * drives the OLED display, logs episodes to SPI flash, and exposes
 * real-time measurements over BLE.
 *
 * The firmware uses a cooperative super-loop with interrupt-driven IMU
 * sampling.  The main loop runs the following state machine:
 *
 *   IDLE → SAMPLE_IMU → PREPROCESS → FEATURE_EXTRACT → ML_INFER →
 *     LOG_EPISODE → UPDATE_DISPLAY → BLE_NOTIFY → IDLE (repeat)
 *
 * Long-press button: enters calibration mode.
 * Short-press button: cycles display view.
 * NFC tap: logs medication dose timestamp.
 *
 * Build: make TARGET=tremorsense
 * Target MCU: nRF5340 (app core), Cortex-M33 @ 128 MHz
 */

#include "board.h"
#include "registers.h"
#include "drivers/imu_drv.h"
#include "drivers/dsp.h"
#include "drivers/ml_model.h"
#include "drivers/flash_drv.h"
#include "drivers/ble_drv.h"
#include "drivers/display_drv.h"
#include "drivers/power_drv.h"
#include "drivers/env_drv.h"

/* ---- Global state ---- */

static volatile device_state_t g_state = STATE_IDLE;
static board_config_t g_config;

/* IMU data ring buffer: 4 windows × 600 bytes */
#define RING_BUF_WINDOWS 4
static uint8_t imu_ring_buf[RING_BUF_WINDOWS * IMU_WINDOW_BYTES];
static volatile uint16_t ring_write_idx = 0;
static volatile uint16_t ring_read_idx = 0;
static volatile uint8_t  ring_count = 0;
static volatile uint8_t  imu_data_ready = 0;

/* DSP working buffers */
static float accel_mag[FFT_SIZE];       /* Acceleration magnitude */
static float gyro_mag[FFT_SIZE];         /* Gyroscope magnitude */
static float fft_real[FFT_SIZE * 2];     /* Real part of FFT output */
static float fft_imag[FFT_SIZE * 2];
static float psd[FFT_SIZE / 2];           /* Power spectral density */
static float features[ML_FEATURE_COUNT]; /* Feature vector for ML */

/* Butterworth bandpass state (4th order, 3.5–12 Hz at 1 kHz) */
static butter_state_t bp_state;

/* Gravity filter state (low-pass at 0.3 Hz) */
static float gravity[3] = {0.0f, 0.0f, 0.0f};
static float lp_alpha;  /* Gravity LPF coefficient */

/* Tremor episode tracking */
typedef struct {
    uint8_t  active;          /* Currently in episode? */
    uint32_t start_time_ms;   /* Episode start timestamp */
    uint32_t duration_ms;     /* Total duration */
    float    dom_freq_hz;     /* Dominant frequency */
    float    rms_amplitude;   /* RMS amplitude (g) */
    uint8_t  tremor_class;     /* ML class index */
    uint8_t  context;          /* REST/POSTURAL/ACTION */
    float    confidence;       /* ML confidence */
    uint16_t sample_count;     /* Raw samples to store */
} episode_t;

static episode_t current_episode;
static uint32_t last_episode_time_ms = 0;

/* Daily tremor score accumulator */
static uint16_t daily_score = 0;
static uint32_t daily_tremor_ms = 0;
static uint32_t daily_reset_time = 0;

/* Display state */
typedef enum {
    DISP_VIEW_IDLE = 0,
    DISP_VIEW_WAVEFORM,
    DISP_VIEW_SPECTRUM,
    DISP_VIEW_SCORE,
    DISP_VIEW_BATTERY,
    DISP_VIEW_COUNT
} display_view_t;

static display_view_t display_view = DISP_VIEW_IDLE;
static uint32_t last_display_update = 0;
static uint32_t last_ble_notify = 0;

/* Calibration mode */
static uint8_t calibration_mode = 0;
static float calibration_acc_baseline[3] = {0, 0, 0};

/* Button state */
static volatile uint8_t button_pressed = 0;
static volatile uint32_t button_press_time = 0;
static volatile uint8_t button_held = 0;

/* ---- Forward declarations ---- */
static void imu_irq_handler(void);
static void button_irq_handler(void);
static void process_imu_window(const uint8_t *raw_buf);
static void run_dsp_pipeline(const int16_t *accel, const int16_t *gyro,
                             int n_samples);
static void extract_features(void);
static void run_ml_inference(void);
static void check_episode_state(float confidence, uint8_t tremor_class,
                                float dom_freq, float rms_amp);
static void log_episode(void);
static void update_display(uint32_t now_ms);
static void ble_notify_data(uint32_t now_ms);
static void enter_calibration(void);
static void handle_button(uint32_t now_ms);
static void update_daily_score(uint32_t duration_ms, float amplitude,
                                uint8_t tremor_class);
static void daily_score_reset_check(uint32_t now_ms);
static uint8_t classify_context(const float *accel, int n);
static uint16_t compute_crc16(const uint8_t *data, size_t len);

/* ---- Main entry point ---- */

int main(void)
{
    uint32_t now_ms;

    /* ---- Hardware initialization ---- */
    board_init();

    /* Initialize drivers */
    power_init();
    imu_init();
    flash_init();
    display_init();
    env_init();
    ble_init();

    /* Load configuration (defaults if flash is empty) */
    g_config = DEFAULT_CONFIG;

    /* Initialize DSP state */
    lp_alpha = 0.002f;  /* ~0.3 Hz cutoff at 1 kHz: alpha = dt/(RC+dt) */
    butter_bandpass_init(&bp_state, TREMOR_BAND_LOW_HZ, TREMOR_BAND_HIGH_HZ,
                         SAMPLE_RATE_HZ, 4);

    /* Clear buffers */
    memset(accel_mag, 0, sizeof(accel_mag));
    memset(gyro_mag, 0, sizeof(gyro_mag));
    memset(psd, 0, sizeof(psd));
    memset(features, 0, sizeof(features));
    memset(&current_episode, 0, sizeof(current_episode));

    /* Register interrupt handlers */
    imu_register_irq_callback(imu_irq_handler);
    button_register_irq_callback(button_irq_handler);

    /* Start IMU sampling at 1 kHz */
    imu_start_sampling(IMU_ODR_HZ, IMU_ACC_FS_16G, IMU_GYRO_FS_2000DPS,
                        IMU_FIFO_WATERMARK);

    /* Show splash screen */
    display_show_splash("TremorSense v" FIRMWARE_VERSION, "by " AUTHOR);
    board_delay_ms(2000);
    display_clear();

    /* Enable BLE advertising */
    if (g_config.ble_enabled) {
        ble_start_advertising(BLE_ADV_INTERVAL_MS, BLE_TX_POWER_DBM);
    }

    daily_reset_time = board_millis();
    g_state = STATE_IDLE;

    /* ---- Main super-loop ---- */
    while (1) {
        now_ms = board_millis();

        /* Feed watchdog */
        watchdog_feed();

        /* Handle button presses */
        if (button_pressed) {
            handle_button(now_ms);
            button_pressed = 0;
        }

        /* Check battery — critical shutdown */
        if (board_battery_percent() < 5 && !board_is_charging()) {
            g_state = STATE_LOW_BATTERY;
            display_show_low_battery();
            ble_disconnect();
            board_enter_sleep();
            continue;
        }

        /* Process IMU data when ready */
        if (imu_data_ready && ring_count > 0) {
            g_state = STATE_SAMPLE_IMU;

            /* Get next window from ring buffer */
            uint8_t *window = &imu_ring_buf[ring_read_idx * IMU_WINDOW_BYTES];
            process_imu_window(window);

            /* Advance ring buffer */
            CRITICAL_ENTER();
            ring_read_idx = (ring_read_idx + 1) % RING_BUF_WINDOWS;
            ring_count--;
            if (ring_count == 0) {
                imu_data_ready = 0;
            }
            CRITICAL_EXIT();

            /* DSP pipeline */
            g_state = STATE_PREPROCESS;
            /* (done inside process_imu_window above via run_dsp_pipeline) */

            /* Feature extraction */
            g_state = STATE_FEATURE_EXTRACT;
            extract_features();

            /* ML inference */
            g_state = STATE_ML_INFER;
            run_ml_inference();

            /* Episode logic */
            float conf = ml_get_last_confidence();
            uint8_t cls = ml_get_last_class();
            float dom_freq = features[0];   /* Feature 0 = dominant frequency */
            float rms_amp = features[1];    /* Feature 1 = RMS amplitude */
            check_episode_state(conf, cls, dom_freq, rms_amp);

            /* Log completed episodes */
            if (current_episode.active == 0 && current_episode.duration_ms > 0) {
                g_state = STATE_LOG_EPISODE;
                log_episode();
                update_daily_score(current_episode.duration_ms,
                                   current_episode.rms_amplitude,
                                   current_episode.tremor_class);
                memset(&current_episode, 0, sizeof(current_episode));
            }

            /* Update display */
            g_state = STATE_UPDATE_DISPLAY;
            update_display(now_ms);

            /* BLE notification */
            g_state = STATE_BLE_NOTIFY;
            ble_notify_data(now_ms);

            g_state = STATE_IDLE;
        }

        /* Daily score reset at midnight (every 24h) */
        daily_score_reset_check(now_ms);

        /* Yield to lower-priority tasks when no IMU data */
        if (!imu_data_ready) {
            /* Power optimization: wait for interrupt */
            __WFI();
        }
    }

    return 0;  /* Never reached */
}

/* ---- IMU interrupt handler ----
 * Called when ICM-42688-P FIFO watermark (50 samples) is reached.
 * Reads 50×6 int16 values via SPI DMA into the ring buffer.
 */
static void imu_irq_handler(void)
{
    uint8_t *dest = &imu_ring_buf[ring_write_idx * IMU_WINDOW_BYTES];

    /* Read FIFO data via SPI (DMA handles the transfer) */
    uint16_t bytes_read = imu_read_fifo(dest, IMU_WINDOW_BYTES);
    if (bytes_read == 0) {
        return;  /* Spurious interrupt */
    }

    /* Advance write index */
    ring_write_idx = (ring_write_idx + 1) % RING_BUF_WINDOWS;

    CRITICAL_ENTER();
    ring_count++;
    if (ring_count > RING_BUF_WINDOWS) {
        /* Overflow: overwrite oldest window */
        ring_count = RING_BUF_WINDOWS;
        ring_read_idx = ring_write_idx;
    }
    imu_data_ready = 1;
    CRITICAL_EXIT();
}

/* ---- Process one IMU window (50 samples = 50 ms at 1 kHz) ---- */
static void process_imu_window(const uint8_t *raw_buf)
{
    int16_t accel[IMU_WINDOW_SAMPLES * 3];
    int16_t gyro[IMU_WINDOW_SAMPLES * 3];

    /* Parse raw FIFO data: ICM-42688-P FIFO format is
     * [acc_h][acc_l][gyro_h][gyro_l] repeated for each axis set.
     * Actually, FIFO packet format: 1 header byte + 6×2 bytes = 14 bytes.
     * We parse the accel and gyro fields out of each packet.
     */
    for (int i = 0; i < IMU_WINDOW_SAMPLES; i++) {
        const uint8_t *pkt = raw_buf + i * 14;  /* 14 bytes per FIFO packet */
        /* Skip header byte (pkt[0]) */
        accel[i * 3 + 0] = (int16_t)((pkt[1] << 8) | pkt[2]);  /* X */
        accel[i * 3 + 1] = (int16_t)((pkt[3] << 8) | pkt[4]);  /* Y */
        accel[i * 3 + 2] = (int16_t)((pkt[5] << 8) | pkt[6]);  /* Z */
        gyro[i * 3 + 0]  = (int16_t)((pkt[7] << 8) | pkt[8]);  /* X */
        gyro[i * 3 + 1]  = (int16_t)((pkt[9] << 8) | pkt[10]);  /* Y */
        gyro[i * 3 + 2]  = (int16_t)((pkt[11] << 8) | pkt[12]); /* Z */
    }

    /* In calibration mode, accumulate baseline */
    if (calibration_mode) {
        for (int i = 0; i < IMU_WINDOW_SAMPLES; i++) {
            calibration_acc_baseline[0] += accel[i * 3 + 0];
            calibration_acc_baseline[1] += accel[i * 3 + 1];
            calibration_acc_baseline[2] += accel[i * 3 + 2];
        }
        return;
    }

    /* Run DSP pipeline on this window */
    run_dsp_pipeline(accel, gyro, IMU_WINDOW_SAMPLES);
}

/* ---- DSP pipeline: gravity removal, bandpass, FFT ---- */
static void run_dsp_pipeline(const int16_t *accel, const int16_t *gyro,
                             int n_samples)
{
    /* Step 1: Convert raw to physical units and compute magnitude */
    /* We accumulate into a sliding buffer of FFT_SIZE samples.
     * Each call adds n_samples=50 new samples, shifting old ones out.
     * For efficiency, we use a circular index, but here we shift
     * for simplicity (FFT_SIZE=256, shift by 50 each call).
     */

    /* Shift existing samples left by n_samples */
    memmove(accel_mag, &accel_mag[n_samples],
            (FFT_SIZE - n_samples) * sizeof(float));
    memmove(gyro_mag, &gyro_mag[n_samples],
            (FFT_SIZE - n_samples) * sizeof(float));

    /* Add new samples: convert to g and dps, then compute magnitude */
    for (int i = 0; i < n_samples; i++) {
        float ax = (float)accel[i * 3 + 0] / IMU_ACC_SENS_16G_LSB;
        float ay = (float)accel[i * 3 + 1] / IMU_ACC_SENS_16G_LSB;
        float az = (float)accel[i * 3 + 2] / IMU_ACC_SENS_16G_LSB;

        float gx = (float)gyro[i * 3 + 0] / IMU_GYRO_SENS_2000_LSB;
        float gy = (float)gyro[i * 3 + 1] / IMU_GYRO_SENS_2000_LSB;
        float gz = (float)gyro[i * 3 + 2] / IMU_GYRO_SENS_2000_LSB;

        /* Gravity separation: exponential moving average low-pass at ~0.3 Hz */
        gravity[0] += lp_alpha * (ax - gravity[0]);
        gravity[1] += lp_alpha * (ay - gravity[1]);
        gravity[2] += lp_alpha * (az - gravity[2]);

        /* Linear acceleration = total - gravity */
        float lx = ax - gravity[0];
        float ly = ay - gravity[1];
        float lz = az - gravity[2];

        /* Acceleration magnitude (linear) */
        accel_mag[FFT_SIZE - n_samples + i] = sqrtf(lx * lx + ly * ly + lz * lz);

        /* Gyroscope magnitude */
        gyro_mag[FFT_SIZE - n_samples + i] = sqrtf(gx * gx + gy * gy + gz * gz);
    }

    /* Step 2: Apply 4th-order Butterworth bandpass (3.5–12 Hz) to accel magnitude */
    float filtered[FFT_SIZE];
    butter_bandpass_apply(&bp_state, accel_mag, filtered, FFT_SIZE);

    /* Step 3: Compute 256-point FFT on filtered signal */
    /* Apply Hanning window */
    float windowed[FFT_SIZE];
    for (int i = 0; i < FFT_SIZE; i++) {
        float w = 0.5f * (1.0f - cosf(2.0f * 3.14159265f * (float)i / (float)(FFT_SIZE - 1)));
        windowed[i] = filtered[i] * w;
    }

    /* Run FFT (radix-2 Cooley-Tukey, in-place on real/imag arrays) */
    fft_compute(windowed, fft_real, fft_imag, FFT_SIZE);

    /* Step 4: Compute one-sided power spectral density */
    for (int i = 0; i < FFT_SIZE / 2; i++) {
        float re = fft_real[i];
        float im = fft_imag[i];
        psd[i] = (re * re + im * im) / (float)FFT_SIZE;
    }

    /* Scale by window power */
    float window_power = 0.0f;
    for (int i = 0; i < FFT_SIZE; i++) {
        float w = 0.5f * (1.0f - cosf(2.0f * 3.14159265f * (float)i / (float)(FFT_SIZE - 1)));
        window_power += w * w;
    }
    if (window_power > 0) {
        for (int i = 0; i < FFT_SIZE / 2; i++) {
            psd[i] /= window_power;
        }
    }
}

/* ---- Feature extraction from PSD and time-domain ---- */
static void extract_features(void)
{
    /* Find dominant frequency in tremor band (3.5–12 Hz) */
    int bin_low = (int)(TREMOR_BAND_LOW_HZ / FREQ_RESOLUTION_HZ);
    int bin_high = (int)(TREMOR_BAND_HIGH_HZ / FREQ_RESOLUTION_HZ);
    if (bin_high >= FFT_SIZE / 2) bin_high = FFT_SIZE / 2 - 1;

    float max_psd = 0.0f;
    int max_bin = bin_low;
    for (int i = bin_low; i <= bin_high; i++) {
        if (psd[i] > max_psd) {
            max_psd = psd[i];
            max_bin = i;
        }
    }

    float dom_freq = (float)max_bin * FREQ_RESOLUTION_HZ;
    features[0] = dom_freq;

    /* RMS amplitude in tremor band */
    float band_power = 0.0f;
    for (int i = bin_low; i <= bin_high; i++) {
        band_power += psd[i];
    }
    float rms_amp = sqrtf(band_power);
    features[1] = rms_amp;

    /* Harmonic ratio: fundamental / 2nd harmonic */
    int harm2_bin = max_bin * 2;
    float harm2_psd = (harm2_bin < FFT_SIZE / 2) ? psd[harm2_bin] : 0.0f;
    features[2] = (harm2_psd > 0.001f) ? (max_psd / harm2_psd) : 100.0f;

    /* Harmonic ratio 3rd */
    int harm3_bin = max_bin * 3;
    float harm3_psd = (harm3_bin < FFT_SIZE / 2) ? psd[harm3_bin] : 0.0f;
    features[3] = (harm3_psd > 0.001f) ? (max_psd / harm3_psd) : 100.0f;

    /* Spectral centroid in tremor band */
    float weighted_sum = 0.0f, total_psd = 0.0f;
    for (int i = bin_low; i <= bin_high; i++) {
        weighted_sum += (float)i * psd[i];
        total_psd += psd[i];
    }
    features[4] = (total_psd > 0.001f) ?
        (weighted_sum / total_psd * FREQ_RESOLUTION_HZ) : 0.0f;

    /* Spectral entropy (normalized) */
    float entropy = 0.0f;
    for (int i = bin_low; i <= bin_high; i++) {
        if (psd[i] > 0.0001f && total_psd > 0.0001f) {
            float p = psd[i] / total_psd;
            entropy -= p * logf(p);
        }
    }
    /* Normalize by max possible entropy (log(N)) */
    int n_bins = bin_high - bin_low + 1;
    features[5] = (n_bins > 1) ? (entropy / logf((float)n_bins)) : 0.0f;

    /* Time-domain: kurtosis of filtered acceleration magnitude */
    float mean = 0.0f;
    for (int i = 0; i < FFT_SIZE; i++) mean += accel_mag[i];
    mean /= FFT_SIZE;
    float variance = 0.0f, m4 = 0.0f;
    for (int i = 0; i < FFT_SIZE; i++) {
        float d = accel_mag[i] - mean;
        variance += d * d;
        m4 += d * d * d * d;
    }
    variance /= FFT_SIZE;
    m4 /= FFT_SIZE;
    features[6] = (variance > 0.0001f) ? (m4 / (variance * variance)) : 0.0f;

    /* Hjorth activity (variance) */
    features[7] = variance;

    /* Hjorth mobility: sqrt(var(diff) / var(signal)) */
    float diff_var = 0.0f;
    for (int i = 1; i < FFT_SIZE; i++) {
        float d = accel_mag[i] - accel_mag[i - 1];
        diff_var += d * d;
    }
    diff_var /= (FFT_SIZE - 1);
    features[8] = (variance > 0.0001f) ? sqrtf(diff_var / variance) : 0.0f;

    /* Hjorth complexity: mobility(diff) / mobility(signal) */
    float diff2_var = 0.0f;
    for (int i = 2; i < FFT_SIZE; i++) {
        float d = accel_mag[i] - 2.0f * accel_mag[i - 1] + accel_mag[i - 2];
        diff2_var += d * d;
    }
    diff2_var /= (FFT_SIZE - 2);
    float mobility_diff = (diff_var > 0.0001f) ? sqrtf(diff2_var / diff_var) : 0.0f;
    features[9] = (features[8] > 0.0001f) ? (mobility_diff / features[8]) : 0.0f;

    /* Gyroscope-accelerometer coherence proxy:
     * ratio of gyro band power to accel band power
     */
    float gyro_band_power = 0.0f;
    /* Compute gyro PSD on the fly (reuse FFT arrays) */
    memset(fft_real, 0, sizeof(fft_real));
    memset(fft_imag, 0, sizeof(fft_imag));
    float gyro_windowed[FFT_SIZE];
    for (int i = 0; i < FFT_SIZE; i++) {
        float w = 0.5f * (1.0f - cosf(2.0f * 3.14159265f * (float)i / (float)(FFT_SIZE - 1)));
        gyro_windowed[i] = gyro_mag[i] * w;
    }
    fft_compute(gyro_windowed, fft_real, fft_imag, FFT_SIZE);
    for (int i = bin_low; i <= bin_high; i++) {
        gyro_band_power += (fft_real[i] * fft_real[i] + fft_imag[i] * fft_imag[i]);
    }
    features[10] = (band_power > 0.0001f) ? (gyro_band_power / band_power) : 0.0f;

    /* Activity context (low-band energy) */
    float low_band_power = 0.0f;
    int low_bin_high = (int)(2.0f / FREQ_RESOLUTION_HZ);
    for (int i = 1; i <= low_bin_high && i < FFT_SIZE / 2; i++) {
        low_band_power += psd[i];
    }
    features[11] = sqrtf(low_band_power);  /* Low-freq activity level */

    /* Cross-axis asymmetry: ratio of dominant freq power in X vs Y */
    /* (Simplified: use accel magnitude variance ratio)
     * For a full implementation, we'd compute per-axis FFTs.
     * Here we approximate with dominant-peak sharpness.
     */
    float peak_sharpness = 0.0f;
    if (max_bin > bin_low && max_bin < bin_high) {
        float neighbor_avg = (psd[max_bin - 1] + psd[max_bin + 1]) * 0.5f;
        peak_sharpness = (neighbor_avg > 0.001f) ? (max_psd / neighbor_avg) : 1.0f;
    }
    features[12] = peak_sharpness;

    /* Total signal energy */
    float total_energy = 0.0f;
    for (int i = 0; i < FFT_SIZE / 2; i++) {
        total_energy += psd[i];
    }
    features[13] = total_energy;

    /* Frequency stability: how much dominant freq changed over last
     * few windows (we track this with a simple buffer). */
    static float freq_history[8];
    static int freq_hist_idx = 0;
    float freq_mean = 0.0f;
    for (int i = 0; i < 8; i++) freq_mean += freq_history[i];
    freq_mean /= 8.0f;
    float freq_var = 0.0f;
    for (int i = 0; i < 8; i++) {
        float d = freq_history[i] - freq_mean;
        freq_var += d * d;
    }
    freq_var /= 8.0f;
    features[14] = sqrtf(freq_var);  /* Frequency stability (lower=more stable) */
    freq_history[freq_hist_idx] = dom_freq;
    freq_hist_idx = (freq_hist_idx + 1) % 8;

    /* Tremor percentage: fraction of last 8 windows with significant tremor */
    static uint8_t tremor_flags[8];
    static int tremor_flag_idx = 0;
    tremor_flags[tremor_flag_idx] = (rms_amp > 0.01f) ? 1 : 0;
    tremor_flag_idx = (tremor_flag_idx + 1) % 8;
    float tremor_pct = 0.0f;
    for (int i = 0; i < 8; i++) tremor_pct += tremor_flags[i];
    features[15] = tremor_pct / 8.0f;
}

/* ---- Run ML inference ---- */
static void run_ml_inference(void)
{
    /* Normalize features and run int8 CNN */
    ml_infer(features, ML_FEATURE_COUNT);
}

/* ---- Episode state machine ---- */
static void check_episode_state(float confidence, uint8_t tremor_class,
                                float dom_freq, float rms_amp)
{
    uint32_t now_ms = board_millis();

    if (confidence >= g_config.confidence_thresh && rms_amp > 0.008f) {
        /* Tremor detected */
        if (!current_episode.active) {
            /* Start new episode */
            current_episode.active = 1;
            current_episode.start_time_ms = now_ms;
            current_episode.duration_ms = 0;
            current_episode.dom_freq_hz = dom_freq;
            current_episode.rms_amplitude = rms_amp;
            current_episode.tremor_class = tremor_class;
            current_episode.context = classify_context(accel_mag, FFT_SIZE);
            current_episode.confidence = confidence;
            current_episode.sample_count = 0;
        } else {
            /* Update existing episode */
            current_episode.duration_ms = now_ms - current_episode.start_time_ms;
            /* Running average of frequency and amplitude */
            current_episode.dom_freq_hz = 0.7f * current_episode.dom_freq_hz
                                        + 0.3f * dom_freq;
            current_episode.rms_amplitude = 0.7f * current_episode.rms_amplitude
                                           + 0.3f * rms_amp;
            current_episode.confidence = 0.7f * current_episode.confidence
                                        + 0.3f * confidence;
            current_episode.sample_count += IMU_WINDOW_SAMPLES;
        }
    } else {
        /* No tremor detected */
        if (current_episode.active) {
            current_episode.duration_ms = now_ms - current_episode.start_time_ms;
            /* End episode if it was long enough */
            if (current_episode.duration_ms >=
                (uint32_t)(ML_MIN_EPISODE_SEC * 1000.0f)) {
                current_episode.active = 0;  /* Will be logged in main loop */
                last_episode_time_ms = now_ms;
            } else {
                /* Too short, discard */
                memset(&current_episode, 0, sizeof(current_episode));
            }
        }
    }
}

/* ---- Log episode to SPI flash ---- */
static void log_episode(void)
{
    if (current_episode.duration_ms == 0) return;

    /* Build episode record */
    uint8_t record[64];  /* Header portion */
    memset(record, 0, sizeof(record));

    uint32_t timestamp = current_episode.start_time_ms / 1000;
    uint16_t duration = (uint16_t)(current_episode.duration_ms & 0xFFFF);
    uint16_t sample_count = current_episode.sample_count;

    /* Pack header */
    uint32_t magic = EPISODE_MAGIC;
    memcpy(&record[0], &magic, 4);
    memcpy(&record[4], &timestamp, 4);
    memcpy(&record[8], &duration, 2);
    memcpy(&record[10], &sample_count, 2);
    memcpy(&record[12], &current_episode.dom_freq_hz, 4);
    memcpy(&record[16], &current_episode.rms_amplitude, 4);
    record[20] = current_episode.tremor_class;
    record[21] = current_episode.context;
    record[22] = (uint8_t)(current_episode.confidence * 100.0f);
    record[23] = 0;  /* reserved */

    /* CRC-16 over header bytes 0–23 */
    uint16_t crc = compute_crc16(record, 24);
    memcpy(&record[24], &crc, 2);

    /* Write header to flash (append mode) */
    flash_append_record(record, 26);

    /* Optionally write compressed raw IMU samples after header.
     * For storage efficiency, we store 1-in-10 samples (100 Hz effective).
     * 10-second episode × 100 samples/s × 12 bytes = 12 KB.
     * For simplicity, we write a subset here.
     */
    if (current_episode.sample_count > 0 && current_episode.sample_count < 5000) {
        /* Would write compressed samples here in production */
    }
}

/* ---- Update OLED display ---- */
static void update_display(uint32_t now_ms)
{
    if (!g_config.display_on) return;

    uint32_t refresh_interval = (current_episode.active) ?
        DISPLAY_REFRESH_ACTIVE_MS : DISPLAY_REFRESH_IDLE_MS;

    if ((now_ms - last_display_update) < refresh_interval) return;
    last_display_update = now_ms;

    display_view_t view = display_view;

    switch (view) {
    case DISP_VIEW_IDLE:
        display_show_idle(daily_score, board_battery_percent(),
                         current_episode.active);
        break;
    case DISP_VIEW_WAVEFORM:
        display_show_waveform(accel_mag, FFT_SIZE,
                             current_episode.active,
                             current_episode.dom_freq_hz);
        break;
    case DISP_VIEW_SPECTRUM:
        display_show_spectrum(psd, FFT_SIZE / 2,
                             current_episode.dom_freq_hz);
        break;
    case DISP_VIEW_SCORE:
        display_show_score(daily_score, daily_tremor_ms / 1000,
                          (uint8_t)current_episode.tremor_class,
                          current_episode.confidence);
        break;
    case DISP_VIEW_BATTERY:
        display_show_battery(board_battery_percent(),
                            board_is_charging(),
                            power_get_estimated_runtime_hours());
        break;
    default:
        display_view = DISP_VIEW_IDLE;
        break;
    }
}

/* ---- BLE notification (1 Hz real-time data) ---- */
static void ble_notify_data(uint32_t now_ms)
{
    if (!g_config.ble_enabled) return;
    if (!ble_is_connected()) return;

    if ((now_ms - last_ble_notify) < (1000 / BLE_NOTIFY_RATE_HZ)) return;
    last_ble_notify = now_ms;

    /* Pack status notification: freq(4) + amp(4) + class(1) + conf(1) + ctx(1) + score(2) + batt(1) = 14 bytes */
    uint8_t notify[14];
    float freq = current_episode.active ? current_episode.dom_freq_hz : 0.0f;
    float amp = current_episode.active ? current_episode.rms_amplitude : 0.0f;
    memcpy(&notify[0], &freq, 4);
    memcpy(&notify[4], &amp, 4);
    notify[8] = current_episode.active ? current_episode.tremor_class : 0xFF;
    notify[9] = current_episode.active ?
        (uint8_t)(current_episode.confidence * 100.0f) : 0;
    notify[10] = current_episode.active ? current_episode.context : 0;
    notify[11] = (uint8_t)(daily_score & 0xFF);
    notify[12] = (uint8_t)((daily_score >> 8) & 0xFF);
    notify[13] = board_battery_percent();

    ble_notify_tremor_status(notify, sizeof(notify));
}

/* ---- Classify activity context: REST / POSTURAL / ACTION ---- */
static uint8_t classify_context(const float *accel, int n)
{
    /* Compute low-frequency energy (< 2 Hz) as proxy for voluntary motion */
    /* If very low → REST, moderate → POSTURAL, high → ACTION */
    float low_energy = features[11];  /* Already extracted */

    if (low_energy < 0.02f) return 0;  /* REST */
    if (low_energy < 0.15f) return 1;  /* POSTURAL */
    return 2;                           /* ACTION */
}

/* ---- Enter calibration mode ---- */
static void enter_calibration(void)
{
    calibration_mode = 1;
    memset(calibration_acc_baseline, 0, sizeof(calibration_acc_baseline));
    display_show_message("Calibrating...", "Keep still for 5s");
    board_delay_ms(5000);

    /* Average baseline */
    calibration_acc_baseline[0] /= (IMU_WINDOW_SAMPLES * 100);  /* ~100 windows in 5s */
    calibration_acc_baseline[1] /= (IMU_WINDOW_SAMPLES * 100);
    calibration_acc_baseline[2] /= (IMU_WINDOW_SAMPLES * 100);

    calibration_mode = 0;
    display_show_message("Calibration", "Complete!");
    board_delay_ms(1500);
}

/* ---- Handle button press ---- */
static void handle_button(uint32_t now_ms)
{
    uint32_t press_duration = now_ms - button_press_time;

    if (press_duration > 2000) {
        /* Long press: enter calibration mode */
        enter_calibration();
    } else if (press_duration > 50) {
        /* Short press: cycle display view */
        display_view = (display_view + 1) % DISP_VIEW_COUNT;
    }
}

/* ---- Button interrupt handler ---- */
static void button_irq_handler(void)
{
    uint32_t now = board_millis();

    if (gpio_read(BUTTON_PIN) == 0) {
        /* Button pressed */
        button_press_time = now;
        button_held = 1;
    } else {
        /* Button released */
        if (button_held) {
            button_pressed = 1;
            button_held = 0;
        }
    }
}

/* ---- Update daily tremor score ---- */
static void update_daily_score(uint32_t duration_ms, float amplitude,
                                uint8_t tremor_class)
{
    /* Score = sum over episodes of (duration_factor × amplitude_factor × class_factor)
     * Duration: normalized to hours (max 8 hours = 100%)
     * Amplitude: normalized (1g = 100%)
     * Class: Parkinsonian=1.0, Essential=0.8, Cerebellar=0.7, Physiol=0.3, Drug=0.6
     */
    float dur_hrs = (float)duration_ms / 3600000.0f;
    float dur_factor = (dur_hrs / 8.0f);
    if (dur_factor > 1.0f) dur_factor = 1.0f;

    float amp_factor = (amplitude / 1.0f);
    if (amp_factor > 1.0f) amp_factor = 1.0f;

    float class_mult[] = {1.0f, 0.8f, 0.7f, 0.3f, 0.6f};
    float class_factor = (tremor_class < ML_CLASS_COUNT) ?
        class_mult[tremor_class] : 0.5f;

    uint16_t episode_score = (uint16_t)(dur_factor * amp_factor *
                                         class_factor * 100.0f);
    daily_score += episode_score;
    if (daily_score > 100) daily_score = 100;

    daily_tremor_ms += duration_ms;
}

/* ---- Reset daily score at midnight (or every 24h) ---- */
static void daily_score_reset_check(uint32_t now_ms)
{
    if ((now_ms - daily_reset_time) >= 86400000UL) {  /* 24 hours */
        daily_score = 0;
        daily_tremor_ms = 0;
        daily_reset_time = now_ms;
    }
}

/* ---- CRC-16 CCITT computation ---- */
static uint16_t compute_crc16(const uint8_t *data, size_t len)
{
    uint16_t crc = CRC16_INIT_VAL;
    for (size_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (int j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ CRC16_POLY_CCITT;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

/* ---- Weak stubs for platform functions (overridden by BSP) ---- */
__attribute__((weak)) void board_init(void) { }
__attribute__((weak)) void board_deinit(void) { }
__attribute__((weak)) uint32_t board_millis(void) { return 0; }
__attribute__((weak)) void board_delay_ms(uint32_t ms) { (void)ms; }
__attribute__((weak)) void board_enter_sleep(void) { }
__attribute__((weak)) void board_wakeup(void) { }
__attribute__((weak)) uint16_t board_read_battery_mv(void) { return 3700; }
__attribute__((weak)) uint8_t board_battery_percent(void) { return 80; }
__attribute__((weak)) uint8_t board_is_charging(void) { return 0; }
__attribute__((weak)) uint32_t board_get_device_id(void) { return 0; }

__attribute__((weak)) void gpio_set(uint8_t pin, uint8_t val) { (void)pin; (void)val; }
__attribute__((weak)) uint8_t gpio_read(uint8_t pin) { (void)pin; return 1; }
__attribute__((weak)) void gpio_toggle(uint8_t pin) { (void)pin; }
__attribute__((weak)) void gpio_config_output(uint8_t pin) { (void)pin; }
__attribute__((weak)) void gpio_config_input(uint8_t pin, uint8_t pullup) { (void)pin; (void)pullup; }

__attribute__((weak)) uint32_t irq_save(void) { return 0; }
__attribute__((weak)) void irq_restore(uint32_t primask) { (void)primask; }

/* ---- End of main.c ---- */