/*
 * ph_isfet.c — ISFET pH Sensor Driver
 *
 * Drives the LMP91200 ISFET/pH analog front-end and reads the conditioned
 * pH voltage from the ESP32-S3 ADC. The pH is computed from the Nernst
 * equation using a two-point calibration (pH 4.00 and pH 7.00 buffers)
 * stored in NVS.
 *
 * The LMP91200 provides:
 *  - High-impedance buffer for the ISFET/pH probe (>1 TOhm input)
 *  - Temperature compensation input
 *  - Programmable bias voltage
 *  - Ground-reference or VDD/2 reference output
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: GPL-3.0
 */

#include "ph_isfet.h"
#include "../board.h"
#include "../registers.h"
#include "driver/i2c.h"
#include "driver/adc.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include <math.h>

static const char *TAG = "ph_isfet";

/* Calibration parameters (loaded from NVS):
 * pH = offset + slope * (adc_voltage_mv / PH_NERNST_MV)
 * where slope and offset are determined by two-point calibration.
 */
static float s_ph_offset = 7.0f;   /* pH at zero mV (isopotential) */
static float s_ph_slope  = 1.0f;   /* slope factor (ideally 1.0)   */
static bool  s_calibrated = false;

/* ------------------------------------------------------------------- */
/* LMP91200 I2C write helper                                           */
/* ------------------------------------------------------------------- */

static esp_err_t lmp91200_write_config(uint8_t config)
{
    uint8_t buf[2] = { LMP91200_REG_CONFIG, config };
    return i2c_master_write_to_device(I2C_PORT_NUM, LMP91200_I2C_ADDR,
                                      buf, 2, pdMS_TO_TICKS(50));
}

/* ------------------------------------------------------------------- */
/* Load calibration from NVS                                          */
/* ------------------------------------------------------------------- */

static void load_calibration(void)
{
    nvs_handle_t h;
    if (nvs_open("fermentiq", NVS_READONLY, &h) != ESP_OK) {
        ESP_LOGW(TAG, "No pH calibration in NVS, using defaults");
        return;
    }

    float offset = 0.0f, slope = 0.0f;
    if (nvs_get_f32(h, "ph_offset", &offset) == ESP_OK &&
        nvs_get_f32(h, "ph_slope", &slope) == ESP_OK) {
        s_ph_offset = offset;
        s_ph_slope = slope;
        s_calibrated = true;
        ESP_LOGI(TAG, "pH calibration loaded: offset=%.3f slope=%.4f",
                 s_ph_offset, s_ph_slope);
    }
    nvs_close(h);
}

/* ------------------------------------------------------------------- */
/* Save calibration to NVS                                            */
/* ------------------------------------------------------------------- */

static void save_calibration(void)
{
    nvs_handle_t h;
    if (nvs_open("fermentiq", NVS_READWRITE, &h) != ESP_OK)
        return;
    nvs_set_f32(h, "ph_offset", s_ph_offset);
    nvs_set_f32(h, "ph_slope", s_ph_slope);
    nvs_commit(h);
    nvs_close(h);
    ESP_LOGI(TAG, "pH calibration saved: offset=%.3f slope=%.4f",
             s_ph_offset, s_ph_slope);
}

/* ------------------------------------------------------------------- */
/* Initialization                                                      */
/* ------------------------------------------------------------------- */

int ph_isfet_init(void)
{
    /* Configure LMP91200 for pH measurement mode */
    uint8_t config = LMP91200_CFG_PH_MODE | LMP91200_CFG_TEMP_EN |
                     LMP91200_CFG_VBIAS_EN | LMP91200_CFG_HIGH_Z;
    esp_err_t err = lmp91200_write_config(config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "LMP91200 config failed: %s", esp_err_to_name(err));
        /* Continue anyway — we can still try to read the ADC */
    }

    load_calibration();
    ESP_LOGI(TAG, "pH ISFET driver initialized (ADC CH%d)", PH_ADC_CHANNEL);
    return 0;
}

/* ------------------------------------------------------------------- */
/* Read raw ADC and convert to pH                                      */
/* ------------------------------------------------------------------- */

int ph_isfet_read(float *ph, float *raw_mv)
{
    /* Oversample: take 16 readings and average for noise reduction */
    uint32_t sum = 0;
    int valid = 0;

    for (int i = 0; i < 16; i++) {
        int raw = adc1_get_raw(PH_ADC_CHANNEL);
        if (raw >= 0) {
            sum += (uint32_t)raw;
            valid++;
        }
    }

    if (valid < 12) {
        ESP_LOGW(TAG, "ADC read failed (%d/16 valid)", valid);
        return -1;
    }

    float avg = (float)sum / (float)valid;
    float voltage_mv = (avg / PH_ADC_MAX) * PH_ADC_VREF_MV;

    /* The LMP91200 outputs a voltage centered at VDD/2 (= 1650 mV) for
     * pH 7.0 (neutral). The Nernst slope is ~59.16 mV/pH at 25°C.
     * pH = 7.0 + (1650 - voltage_mv) / 59.16 * slope_factor
     * (inverted because higher pH = lower voltage) */

    float delta_mv = 1650.0f - voltage_mv;
    float ph_val = s_ph_offset + (delta_mv / PH_NERNST_MV) * s_ph_slope;

    /* Temperature compensation (simplified — Nernst slope is temp-dependent):
     * slope(T) = slope(25°C) * (T + 273.15) / 298.15
     * This correction is applied if we have a valid temperature reading.
     * For now, we use the default 25°C. The fusion task can post-correct. */

    *ph = ph_val;
    *raw_mv = voltage_mv;

    return 0;
}

/* ------------------------------------------------------------------- */
/* Two-point calibration                                               */
/* ------------------------------------------------------------------- */

int ph_isfet_calibrate(float ph_buffer_1, float ph_buffer_2)
{
    ESP_LOGI(TAG, "Starting 2-point pH calibration: pH %.2f and pH %.2f",
             ph_buffer_1, ph_buffer_2);

    /* Read raw voltage for buffer 1 */
    float mv1 = 0.0f, ph_dummy = 0.0f;
    vTaskDelay(pdMS_TO_TICKS(2000));  /* let probe stabilize */

    for (int i = 0; i < 8; i++) {
        ph_isfet_read(&ph_dummy, &mv1);
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    ESP_LOGI(TAG, "Buffer 1 (pH %.2f): %.1f mV", ph_buffer_1, mv1);

    /* Read raw voltage for buffer 2 */
    ESP_LOGI(TAG, ">> Rinse probe, place in pH %.2f buffer, wait...", ph_buffer_2);
    vTaskDelay(pdMS_TO_TICKS(10000));  /* user swaps buffers */

    float mv2 = 0.0f;
    for (int i = 0; i < 8; i++) {
        ph_isfet_read(&ph_dummy, &mv2);
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    ESP_LOGI(TAG, "Buffer 2 (pH %.2f): %.1f mV", ph_buffer_2, mv2);

    /* Compute slope and offset:
     * ph = offset + slope * (1650 - mv) / 59.16
     * For two points:
     *   slope = (ph1 - ph2) / ((1650-mv1)/59.16 - (1650-mv2)/59.16)
     *   offset = ph1 - slope * (1650-mv1)/59.16 */

    float d1 = (1650.0f - mv1) / PH_NERNST_MV;
    float d2 = (1650.0f - mv2) / PH_NERNST_MV;

    if (fabsf(d1 - d2) < 1e-6f) {
        ESP_LOGE(TAG, "Calibration failed: readings too close");
        return -1;
    }

    s_ph_slope = (ph_buffer_1 - ph_buffer_2) / (d1 - d2);
    s_ph_offset = ph_buffer_1 - s_ph_slope * d1;
    s_calibrated = true;

    save_calibration();

    ESP_LOGI(TAG, "Calibration complete: offset=%.3f slope=%.4f",
             s_ph_offset, s_ph_slope);
    return 0;
}

/* ------------------------------------------------------------------- */
/* Get current calibration parameters                                  */
/* ------------------------------------------------------------------- */

int ph_isfet_get_calibration(float *offset, float *slope)
{
    *offset = s_ph_offset;
    *slope = s_ph_slope;
    return s_calibrated ? 0 : 1;
}