/*
 * ad5933.c — AD5933 Impedance Analyzer Driver
 *
 * Drives the Analog Devices AD5933 impedance-to-digital converter via I2C
 * to perform electrical impedance spectroscopy (EIS) on the fermentation
 * liquid. Uses an ADG715 8:1 analog switch to configure 4-wire (Kelvin)
 * measurement with separate excitation and sensing electrode pairs.
 *
 * The driver runs a frequency sweep from 1 kHz to 100 kHz in 50 steps,
 * collects the real and imaginary impedance components, computes magnitude
 * and phase, and extracts 8 features (Cole-Cole parameters) for the
 * TinyML biomass estimation model.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: GPL-3.0
 */

#include "ad5933.h"
#include "../board.h"
#include "../registers.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>

static const char *TAG = "ad5933";

/* ------------------------------------------------------------------- */
/* I2C helpers                                                         */
/* ------------------------------------------------------------------- */

static esp_err_t ad5933_write(uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = { reg, val };
    return i2c_master_write_to_device(I2C_PORT_NUM, AD5933_I2C_ADDR,
                                      buf, 2, pdMS_TO_TICKS(100));
}

static esp_err_t ad5933_write_block(const uint8_t *data, size_t len)
{
    return i2c_master_write_to_device(I2C_PORT_NUM, AD5933_I2C_ADDR,
                                      data, len, pdMS_TO_TICKS(100));
}

static esp_err_t ad5933_read(uint8_t reg, uint8_t *buf, size_t len)
{
    esp_err_t err = i2c_master_write_read_device(I2C_PORT_NUM, AD5933_I2C_ADDR,
                                                  &reg, 1, buf, len,
                                                  pdMS_TO_TICKS(100));
    return err;
}

static esp_err_t ad5933_read_reg16(uint8_t reg, uint16_t *val)
{
    uint8_t buf[2];
    esp_err_t err = ad5933_read(reg, buf, 2);
    if (err == ESP_OK)
        *val = ((uint16_t)buf[0] << 8) | buf[1];
    return err;
}

/* ------------------------------------------------------------------- */
/* ADG715 analog switch control                                        */
/* ------------------------------------------------------------------- */

static esp_err_t adg715_set_channels(uint8_t mask)
{
    return i2c_master_write_to_device(I2C_PORT_NUM, ADG715_ADDR,
                                      &mask, 1, pdMS_TO_TICKS(50));
}

/* ------------------------------------------------------------------- */
/* Frequency coding (AD5933 uses a 24-bit code from MCLK)             */
/* ------------------------------------------------------------------- */

static void ad5933_code_freq(uint32_t freq_hz, uint8_t *h, uint8_t *m, uint8_t *l)
{
    /* code = (freq / (MCLK / 2^27)) rounded */
    uint32_t code = (uint32_t)(((uint64_t)freq_hz << 27) / AD5933_MCLK_HZ + 0.5);
    *h = (code >> 16) & 0xFF;
    *m = (code >> 8)  & 0xFF;
    *l = (code)       & 0xFF;
}

/* ------------------------------------------------------------------- */
/* Wait for data valid / sweep complete                               */
/* ------------------------------------------------------------------- */

static int ad5933_wait_status(uint8_t mask, int timeout_ms)
{
    uint8_t status = 0;
    int elapsed = 0;
    while (elapsed < timeout_ms) {
        ad5933_read(AD5933_REG_STATUS, &status, 1);
        if (status & mask)
            return 0;
        vTaskDelay(pdMS_TO_TICKS(2));
        elapsed += 2;
    }
    return -1;
}

/* ------------------------------------------------------------------- */
/* Initialization                                                      */
/* ------------------------------------------------------------------- */

int ad5933_init(void)
{
    /* Reset the AD5933 */
    esp_err_t err = ad5933_write(AD5933_REG_CTRL_H, AD5933_CTRL_RESET);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "AD5933 not responding on I2C (addr 0x%02X)", AD5933_I2C_ADDR);
        return -1;
    }
    vTaskDelay(pdMS_TO_TICKS(5));

    /* Configure: standby mode, 200mVpp output range (range 2), PGA x1 */
    uint8_t ctrl_h = AD5933_CTRL_MODE_STANDBY;
    uint8_t ctrl_l = AD5933_CTRL_RANGE_2V | AD5933_CTRL_PGA_1X;
    ad5933_write(AD5933_REG_CTRL_H, ctrl_h);
    ad5933_write(AD5933_REG_CTRL_L, ctrl_l);

    /* Set settling time cycles (15 cycles + 1 multiplier = 0x00 | 15) */
    ad5933_write(AD5933_REG_NUM_SETTLE_H, 0x00);
    ad5933_write(AD5933_REG_NUM_SETTLE_L, AD5933_SETTLE_CYCLES);

    /* Set number of increment points */
    ad5933_write(AD5933_REG_NUM_INCR_H, (AD5933_NUM_INCR >> 8) & 0xFF);
    ad5933_write(AD5933_REG_NUM_INCR_L, AD5933_NUM_INCR & 0xFF);

    /* Program start frequency */
    uint8_t fh, fm, fl;
    ad5933_code_freq(AD5933_START_FREQ_HZ, &fh, &fm, &fl);
    ad5933_write(AD5933_REG_START_FREQ_H, fh);
    ad5933_write(AD5933_REG_START_FREQ_M, fm);
    ad5933_write(AD5933_REG_START_FREQ_L, fl);

    /* Program frequency increment */
    ad5933_code_freq(AD5933_DELTA_FREQ_HZ, &fh, &fm, &fl);
    ad5933_write(AD5933_REG_FREQ_INCR_H, fh);
    ad5933_write(AD5933_REG_FREQ_INCR_M, fm);
    ad5933_write(AD5933_REG_FREQ_INCR_L, fl);

    /* Initialize analog switch: all off initially */
    adg715_set_channels(ADG715_CHAN_ALL_OFF);

    ESP_LOGI(TAG, "AD5933 initialized: sweep %lu Hz, %d pts, %lu Hz step",
             (unsigned long)AD5933_START_FREQ_HZ, AD5933_NUM_INCR,
             (unsigned long)AD5933_DELTA_FREQ_HZ);
    return 0;
}

/* ------------------------------------------------------------------- */
/* Calibration: measure a known reference resistor                     */
/* ------------------------------------------------------------------- */

float ad5933_calibrate(float reference_resistor)
{
    /* Switch to calibration channel */
    adg715_set_channels(ADG715_CHAN_CALIBRATION);
    vTaskDelay(pdMS_TO_TICKS(10));

    /* Run a single-point measurement at 10 kHz */
    ad5933_sweep_result_t sweep;
    memset(&sweep, 0, sizeof(sweep));

    /* Set start freq to 10 kHz, 1 point */
    uint8_t fh, fm, fl;
    ad5933_code_freq(10000UL, &fh, &fm, &fl);
    ad5933_write(AD5933_REG_START_FREQ_H, fh);
    ad5933_write(AD5933_REG_START_FREQ_M, fm);
    ad5933_write(AD5933_REG_START_FREQ_L, fl);
    ad5933_write(AD5933_REG_NUM_INCR_H, 0);
    ad5933_write(AD5933_REG_NUM_INCR_L, 1);

    /* Initialize with start frequency */
    ad5933_write(AD5933_REG_CTRL_H, AD5933_CTRL_MODE_INIT_FREQ |
                 AD5933_CTRL_RANGE_2V);
    vTaskDelay(pdMS_TO_TICKS(20));

    /* Start sweep */
    ad5933_write(AD5933_REG_CTRL_H, AD5933_CTRL_MODE_START_SWEEP |
                 AD5933_CTRL_RANGE_2V);

    if (ad5933_wait_status(AD5933_STATUS_SWEEP_DONE, 500) != 0) {
        ESP_LOGW(TAG, "Calibration sweep timeout");
        return 1.0f;  /* default unity gain factor */
    }

    /* Read real + imaginary */
    uint16_t real_raw, imag_raw;
    ad5933_read_reg16(AD5933_REG_REAL_H, &real_raw);
    ad5933_read_reg16(AD5933_REG_IMAG_H, &imag_raw);

    /* Convert from signed 16-bit */
    int16_t real_s = (int16_t)real_raw;
    int16_t imag_s = (int16_t)imag_raw;
    float mag = sqrtf((float)real_s * real_s + (float)imag_s * imag_s);

    /* Gain factor = 1 / (magnitude × reference_resistor) */
    float gain_factor = (mag > 0.1f) ? (1.0f / (mag * reference_resistor)) : 1.0f;

    /* Restore full sweep parameters */
    ad5933_code_freq(AD5933_START_FREQ_HZ, &fh, &fm, &fl);
    ad5933_write(AD5933_REG_START_FREQ_H, fh);
    ad5933_write(AD5933_REG_START_FREQ_M, fm);
    ad5933_write(AD5933_REG_START_FREQ_L, fl);
    ad5933_write(AD5933_REG_NUM_INCR_H, (AD5933_NUM_INCR >> 8) & 0xFF);
    ad5933_write(AD5933_REG_NUM_INCR_L, AD5933_NUM_INCR & 0xFF);

    /* Switch back to measurement electrodes (4-wire) */
    adg715_set_channels(ADG715_CHAN_I_PLUS | ADG715_CHAN_I_MINUS |
                        ADG715_CHAN_V_PLUS | ADG715_CHAN_V_MINUS);
    vTaskDelay(pdMS_TO_TICKS(10));

    ESP_LOGI(TAG, "Calibrated with %.0f ohm ref: gain=%.6f, raw_mag=%.1f",
             reference_resistor, gain_factor, mag);
    return gain_factor;
}

/* ------------------------------------------------------------------- */
/* Run a full frequency sweep                                          */
/* ------------------------------------------------------------------- */

int ad5933_run_sweep(ad5933_sweep_result_t *result)
{
    if (!result)
        return -1;

    memset(result, 0, sizeof(*result));

    /* Ensure 4-wire electrode configuration */
    adg715_set_channels(ADG715_CHAN_I_PLUS | ADG715_CHAN_I_MINUS |
                        ADG715_CHAN_V_PLUS | ADG715_CHAN_V_MINUS);
    vTaskDelay(pdMS_TO_TICKS(5));

    /* Initialize with start frequency */
    ad5933_write(AD5933_REG_CTRL_H, AD5933_CTRL_MODE_INIT_FREQ |
                 AD5933_CTRL_RANGE_2V);
    vTaskDelay(pdMS_TO_TICKS(25));

    /* Start sweep */
    ad5933_write(AD5933_REG_CTRL_H, AD5933_CTRL_MODE_START_SWEEP |
                 AD5933_CTRL_RANGE_2V);

    int point = 0;
    while (point < AD5933_NUM_INCR) {
        /* Wait for data valid */
        if (ad5933_wait_status(AD5933_STATUS_VALID_DATA, 200) != 0) {
            ESP_LOGW(TAG, "Sweep timeout at point %d", point);
            break;
        }

        /* Read real + imaginary */
        uint16_t real_raw, imag_raw;
        ad5933_read_reg16(AD5933_REG_REAL_H, &real_raw);
        ad5933_read_reg16(AD5933_REG_IMAG_H, &imag_raw);

        int16_t real_s = (int16_t)real_raw;
        int16_t imag_s = (int16_t)imag_raw;

        result->real[point] = (float)real_s;
        result->imag[point] = (float)imag_s;
        result->mag[point] = sqrtf((float)real_s * real_s +
                                   (float)imag_s * imag_s);
        result->phase_rad[point] = atan2f((float)imag_s, (float)real_s);

        /* Compute actual frequency for this point */
        result->freq_hz[point] = (float)(AD5933_START_FREQ_HZ +
                                         (uint32_t)point * AD5933_DELTA_FREQ_HZ);

        point++;

        /* Increment to next frequency */
        ad5933_write(AD5933_REG_CTRL_H, AD5933_CTRL_MODE_INC_FREQ |
                     AD5933_CTRL_RANGE_2V);
    }

    result->num_points = point;

    /* Put AD5933 in power-down mode */
    ad5933_write(AD5933_REG_CTRL_H, AD5933_CTRL_MODE_POWER_DOWN);

    /* Turn off analog switch to minimize electrode polarization */
    adg715_set_channels(ADG715_CHAN_ALL_OFF);

    return (point > 0) ? 0 : -1;
}

/* ------------------------------------------------------------------- */
/* Extract Cole-Cole features from sweep for TinyML model              */
/* ------------------------------------------------------------------- */

void ad5933_extract_features(const ad5933_sweep_result_t *sweep,
                             impedance_data_t *data, float cal_factor)
{
    if (!sweep || !data || sweep->num_points < 3)
        return;

    /* Apply calibration gain factor to magnitudes */
    /* Note: in a full implementation, we'd convert raw DFT output to
     * actual impedance using the gain factor + system phase. Here we
     * apply the factor directly to get approximate impedance. */

    /* Find indices for 1 kHz, 10 kHz, 100 kHz */
    int idx_1k = 0, idx_10k = 0, idx_100k = sweep->num_points - 1;
    for (int i = 0; i < sweep->num_points; i++) {
        if (sweep->freq_hz[i] <= 1000.0f)  idx_1k = i;
        if (sweep->freq_hz[i] <= 10000.0f) idx_10k = i;
        if (sweep->freq_hz[i] <= 100000.0f) idx_100k = i;
    }

    /* Magnitude features (convert to ohms via calibration factor) */
    data->z_mag_1k   = sweep->mag[idx_1k]   / (cal_factor + 1e-9f);
    data->z_mag_10k  = sweep->mag[idx_10k]  / (cal_factor + 1e-9f);
    data->z_mag_100k = sweep->mag[idx_100k] / (cal_factor + 1e-9f);

    /* Phase features (degrees) */
    data->z_phase_10k  = RAD_TO_DEG(sweep->phase_rad[idx_10k]);
    data->z_phase_100k = RAD_TO_DEG(sweep->phase_rad[idx_100k]);

    /* Cole-Cole parameter estimation via nonlinear fit (simplified).
     *
     * The Cole-Cole impedance model:
     *   Z(ω) = R∞ + (R0 - R∞) / (1 + (jωτ)^α)
     *
     * where:
     *   R0  = resistance at zero frequency (extracellular)
     *   R∞  = resistance at infinite frequency (total)
     *   τ   = characteristic relaxation time
     *   α   = distribution width (0 = single relaxation, 1 = pure capacitor)
     *
     * We estimate R0 and R∞ from the sweep endpoints, and α from the
     * phase peak width. This is a simplified estimation; a full Levenberg-
     * Marquardt fit would be more accurate but is too expensive for
     * real-time on the ESP32. */

    /* R0 ≈ |Z| at lowest frequency (extracellular resistance) */
    data->cole_r0 = data->z_mag_1k;

    /* R∞ ≈ |Z| at highest frequency */
    data->cole_rinf = data->z_mag_100k;

    /* α estimation from the phase angle at the midpoint.
     * For a Cole-Cole model, the maximum phase angle φ_max relates to α as:
     *   φ_max = (α × π/2) / 2  (simplified for symmetric dispersion)
     * We approximate α from the phase at 10 kHz (midpoint of our sweep). */
    float phase_max_rad = DEG_TO_RAD(fabsf(data->z_phase_10k));
    data->cole_alpha = CLAMP(phase_max_rad / (M_PI / 4.0f), 0.0f, 1.0f);
}

/* ------------------------------------------------------------------- */
/* Read AD5933 internal temperature sensor                             */
/* ------------------------------------------------------------------- */

int ad5933_read_temperature(float *temp_c)
{
    /* Command temperature measurement */
    ad5933_write(AD5933_REG_CTRL_H, AD5933_CTRL_MODE_TEMP |
                 AD5933_CTRL_RANGE_2V);

    if (ad5933_wait_status(AD5933_STATUS_VALID_TEMP, 100) != 0)
        return -1;

    uint16_t raw;
    ad5933_read_reg16(AD5933_REG_TEMP_H, &raw);

    /* Temperature = raw / 32 (per datasheet, 13-bit signed) */
    int16_t signed_raw = (int16_t)(raw >> 2);  /* 14-bit signed */
    *temp_c = (float)signed_raw / 32.0f;
    return 0;
}