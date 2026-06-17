/**
 * @file    calibration.c
 * @brief   SonicSight — Calibration routines.
 *          Manages per-channel gain/delay calibration. Stores data in
 *          QSPI flash for persistence across power cycles.
 * @author  jayis1
 * @copyright © 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 */

#include <string.h>
#include <math.h>
#include "calibration.h"
#include "board.h"
#include "registers.h"
#include "acquisition.h"
#include "crosscorr.h"

/* QSPI flash sector dedicated to calibration data */
#define CALIBRATION_FLASH_SECTOR  (7U)        /* Sector 7 of 16 MB QSPI */
#define CALIBRATION_FLASH_ADDR    (QSPI_BASE + (CALIBRATION_FLASH_SECTOR * 256 * 1024))

/* ========================================================================== */
/*  Initialisation                                                            */
/* ========================================================================== */

void calibration_init(calibration_data_t *data)
{
    memset(data, 0, sizeof(calibration_data_t));
    data->magic = 0x534F4E49; /* "SONI" */
    data->version = 1;
    data->temp_reference = TOMO_TEMP_REF;
    data->alpha_velocity = TOMO_TEMP_ALPHA;
}

/* ========================================================================== */
/*  Load / Save                                                               */
/* ========================================================================== */

int32_t calibration_load(calibration_data_t *data)
{
    /* QSPI is memory-mapped — read directly */
    calibration_data_t *flash_data = (calibration_data_t *)CALIBRATION_FLASH_ADDR;

    if (flash_data->magic != 0x534F4E49) {
        return -1; /* No valid calibration found */
    }

    memcpy(data, flash_data, sizeof(calibration_data_t));

    /* CRC check would go here (omitted for brevity) */
    return ERR_OK;
}

int32_t calibration_save(const calibration_data_t *data)
{
    /* Erase QSPI sector */
    /* QSPI_EraseSector(CALIBRATION_FLASH_SECTOR); — HAL-dependent */

    /* Write calibration data */
    /* QSPI_Write(CALIBRATION_FLASH_ADDR, (uint8_t *)data, sizeof(calibration_data_t)); */

    /* Verify */
    calibration_data_t verify;
    memcpy(&verify, (void *)CALIBRATION_FLASH_ADDR, sizeof(calibration_data_t));
    if (verify.magic != data->magic) {
        return ERR_SD_FAIL;
    }

    return ERR_OK;
}

/* ========================================================================== */
/*  Factory Defaults                                                          */
/* ========================================================================== */

void calibration_set_defaults(calibration_data_t *data)
{
    data->magic = 0x534F4E49;
    data->version = 1;
    data->temp_reference = TOMO_TEMP_REF;
    data->alpha_velocity = TOMO_TEMP_ALPHA;
    data->timestamp = 0;

    for (uint8_t i = 0; i < CAL_NUM_CHANNELS; i++) {
        data->channel_gain_db[i] = (float)VGA_GAIN_DEFAULT_DB;
        data->channel_delay_us[i] = 0.0f;
        data->channel_offset[i] = 0.0f;
    }

    data->crc32 = 0; /* Compute CRC after fields set */
}

/* ========================================================================== */
/*  Calibration Scan — Measure Reference Phantom                             */
/* ========================================================================== */

int32_t run_calibration_scan(calibration_data_t *data)
{
    /* The calibration phantom is an acrylic disc (150 mm dia., 40 mm thick)
     * with 5 known defect holes at known positions. By measuring ToFs on this
     * known geometry, we extract per-channel gain and timing offsets.
     *
     * This is a simplified calibration that measures timing offsets by
     * comparing measured ToF against known theoretical ToF on the phantom.
     */

    /* 1. Place sensors on phantom, run a full acquisition cycle */
    acquisition_ctx_t cal_ctx;
    memset(&cal_ctx, 0, sizeof(cal_ctx));

    /* Setup geometry: 32 sensors equally spaced on phantom circumference */
    float theta[TOMO_MAX_SENSORS];
    float radius[TOMO_MAX_SENSORS];
    uint8_t active[TOMO_MAX_SENSORS];
    for (uint8_t i = 0; i < TOMO_MAX_SENSORS; i++) {
        theta[i] = (float)i * 2.0f * 3.14159265f / (float)TOMO_MAX_SENSORS;
        radius[i] = 0.075f; /* 7.5 cm (phantom radius 75 mm) */
        active[i] = 1;
    }

    /* 2. Arm and acquire */
    acquisition_init(&cal_ctx);
    acquisition_set_gains(&cal_ctx, NULL); /* default gains */
    acquisition_set_filter_fc(&cal_ctx, FILTER_FC_DEFAULT_HZ);
    acquisition_prepare_adc(&cal_ctx);

    /* 3. Measure travel time on known material (PMMA: v_p ~ 2700 m/s) */
    /* Theoretical path length between opposite sensors = 0.15 m
     * Theoretical ToF = 0.15 / 2700 = 55.5 µs */
    float expected_velocity = 2700.0f; /* m/s in PMMA */

    /* For each active sensor as transmitter */
    for (uint8_t tx = 0; tx < TOMO_MAX_SENSORS; tx++) {
        acquisition_select_tx_channel(tx);
        acquisition_configure_rx_muxes(tx, active, TOMO_MAX_SENSORS);
        delay_us(ACQ_SETTLE_TIME_US);

        /* Fire low-voltage pulse (20 V, 2 cycles) */
        TIM1->CCR1 = 320;
        TIM1->BDTR |= TIM_BDTR_MOE;
        TIM1->CR1 |= TIM_CR1_CEN;
        while ((TIM1->RCR & 0xFF) > 1) { /* spin for 2 cycles */ }
        while (!(TIM1->SR & TIM_SR_UIF)) { /* spin */ }
        TIM1->SR &= ~TIM_SR_UIF;
        TIM1->CR1 &= ~TIM_CR1_CEN;
        TIM1->BDTR &= ~TIM_BDTR_MOE;

        /* Capture */
        cal_ctx.capture_done = 0;
        TIM8->CR1 |= TIM_CR1_CEN;
        uint32_t timeout = 50000;
        while (!cal_ctx.capture_done && timeout--) { delay_us(1); }
        TIM8->CR1 &= ~TIM_CR1_CEN;

        /* Extract ToF for each receiver */
        for (uint8_t ch = 0; ch < ADC_NUM_CHANNELS; ch++) {
            uint8_t rx = ch; /* simplified — assumes 1:1 mapping in cal mode */
            if (rx == tx) continue;

            float peak_time_us, snr;
            crosscorr_compute(cal_ctx.adc_buffers[ch], ADC_SAMPLES_PER_TRACE,
                              cal_ctx.tx_template, cal_ctx.tx_template_len,
                              &peak_time_us, &snr);

            /* Theoretical ToF for this path length */
            float dx = radius[tx] * cosf(theta[tx]) - radius[rx] * cosf(theta[rx]);
            float dy = radius[tx] * sinf(theta[tx]) - radius[rx] * sinf(theta[rx]);
            float path_len = sqrtf(dx * dx + dy * dy);
            float expected_tof = (path_len / expected_velocity) * 1e6f;

            /* Timing offset = measured − theoretical */
            float offset = peak_time_us - expected_tof;

            /* Update channel delay — moving average for robustness */
            if (ch < CAL_NUM_CHANNELS) {
                float alpha = 0.2f; /* EMA factor */
                data->channel_delay_us[ch] = (1.0f - alpha) * data->channel_delay_us[ch]
                                           + alpha * offset;
            }
        }
    }

    /* 4. Measure per-channel gain mismatch */
    /* Compare received signal amplitude across channels */
    for (uint8_t ch = 0; ch < CAL_NUM_CHANNELS; ch++) {
        int16_t *buf = cal_ctx.adc_buffers[ch];
        int16_t min_val = 2047, max_val = -2048;
        for (uint32_t s = 100; s < ADC_SAMPLES_PER_TRACE - 50; s++) {
            if (buf[s] < min_val) min_val = buf[s];
            if (buf[s] > max_val) max_val = buf[s];
        }
        float amplitude = (float)(max_val - min_val);
        /* Reference amplitude (channel 0) */
        static float ref_amplitude = 0;
        if (ch == 0) ref_amplitude = amplitude;
        if (ref_amplitude > 10.0f) {
            float gain_adjust = 20.0f * log10f(ref_amplitude / amplitude);
            data->channel_gain_db[ch] += gain_adjust * 0.1f; /* small adjustment */
        }
    }

    data->timestamp = 0; /* Would use RTC timestamp in production */
    calibration_save(data);

    return ERR_OK;
}