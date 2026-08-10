/**
 * @file    doppler.h
 * @brief   TideBand — Doppler velocimeter driver API.
 * @author  jayis1
 * @copyright © 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 */

#ifndef TIDEBAND_DOPPLER_H
#define TIDEBAND_DOPPLER_H

#include <stdint.h>

/* ---- Calibration structure (stored in flash) ---- */
typedef struct {
    uint32_t magic;              /* CAL_MAGIC = 0x54424341 */
    float    deproj_matrix[3][3]; /* Body-frame deprojection matrix */
    float    phase_offset[3];     /* Per-channel phase offsets (rad) */
    float    tx_power_comp;       /* TX power compensation factor */
    float    sound_speed;         /* Calibrated sound speed (m/s) */
    float    scale_factor[3];     /* Per-channel velocity scale */
    uint16_t cal_date;            /* Calibration date (days since epoch) */
    uint16_t reserved;
    uint32_t crc;                 /* CRC32 of above fields */
} doppler_calibration_t;

/* ---- Measurement result ---- */
typedef struct {
    float vx;           /* Body-frame X velocity (m/s), along TX axis */
    float vy;           /* Body-frame Y velocity (m/s) */
    float vz;           /* Body-frame Z velocity (m/s) */
    float speed;        /* Magnitude (m/s) */
    float doppler_hz[3]; /* Raw Doppler shifts per channel (Hz) */
    float snr[3];       /* Per-channel SNR (dB) */
    uint8_t valid;      /* 1 if measurement passed quality checks */
    uint8_t quality;    /* 0=poor, 1=fair, 2=good, 3=excellent */
} doppler_result_t;

/* ---- Public API ---- */

/** Initialize Doppler subsystem: timers, ADC, DMA, SPI. */
void doppler_init(void);

/** Start a single Doppler measurement cycle (TX burst + ADC capture). */
void doppler_trigger(void);

/** Check if measurement data is ready (DMA complete + FFT done). */
uint8_t doppler_data_ready(void);

/** Process captured ADC data: FFT, Doppler extraction, 3D solution.
 *  Fills the result structure. Called after doppler_data_ready() returns 1. */
void doppler_process(doppler_result_t *result);

/** Load calibration from flash. Returns 0 on success, -1 if invalid. */
int doppler_load_calibration(doppler_calibration_t *cal);

/** Save calibration to flash. Returns 0 on success. */
int doppler_save_calibration(const doppler_calibration_t *cal);

/** Apply calibration to raw Doppler shifts to produce velocity. */
void doppler_apply_calibration(const doppler_calibration_t *cal,
                                const float doppler_hz[3],
                                float vel_body[3]);

/** Shutdown Doppler subsystem to save power (between samples). */
void doppler_sleep(void);

/** Wake Doppler subsystem from sleep. */
void doppler_wake(void);

#endif /* TIDEBAND_DOPPLER_H */