/*
 * main.c — FermenTiq Main Firmware Entry Point
 *
 * Initializes all hardware peripherals, creates FreeRTOS tasks for each
 * sensing modality, and manages the shared sensor state. The fusion task
 * combines all sensor data and runs on-device TinyML inference to produce
 * fermentation phase, ABV estimate, and spoilage risk.
 *
 * Hardware: ESP32-S3-WROOM-1-N16R8
 * Framework: ESP-IDF v5.1 + FreeRTOS
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: GPL-3.0
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/spi_master.h"
#include "driver/uart.h"
#include "driver/adc.h"

#include "board.h"
#include "registers.h"
#include "drivers/ad5933.h"
#include "drivers/co2_ndir.h"
#include "drivers/ph_isfet.h"
#include "drivers/temp_rtd.h"
#include "drivers/acoustic.h"
#include "drivers/fusion.h"
#include "drivers/ble.h"
#include "drivers/wifi_mqtt.h"
#include "drivers/power.h"
#include "drivers/storage.h"

static const char *TAG = FERMENTIQ_TAG;

/* ========================================================================
 * Global State
 * ======================================================================== */
fermentiq_state_t g_state;
void *g_state_mutex = NULL;  /* SemaphoreHandle_t cast to void* */

/* Task handles */
static TaskHandle_t h_impedance = NULL;
static TaskHandle_t h_co2       = NULL;
static TaskHandle_t h_ph        = NULL;
static TaskHandle_t h_temp      = NULL;
static TaskHandle_t h_acoustic  = NULL;
static TaskHandle_t h_fusion    = NULL;
static TaskHandle_t h_ble       = NULL;
static TaskHandle_t h_wifi      = NULL;
static TaskHandle_t h_logger    = NULL;

/* ========================================================================
 * State Lock Helpers
 * ======================================================================== */
static inline void state_lock(void)
{
    if (g_state_mutex)
        xSemaphoreTake((SemaphoreHandle_t)g_state_mutex, portMAX_DELAY);
}

static inline void state_unlock(void)
{
    if (g_state_mutex)
        xSemaphoreGive((SemaphoreHandle_t)g_state_mutex);
}

/* ========================================================================
 * I2C Master Initialization
 * ======================================================================== */
static esp_err_t init_i2c(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA,
        .scl_io_num = I2C_MASTER_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ,
    };
    esp_err_t err = i2c_param_config(I2C_PORT_NUM, &conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C param config failed: %s", esp_err_to_name(err));
        return err;
    }
    err = i2c_driver_install(I2C_PORT_NUM, I2C_MODE_MASTER, 0, 0, 0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "I2C driver install failed: %s", esp_err_to_name(err));
        return err;
    }
    ESP_LOGI(TAG, "I2C bus initialized (SDA=%d, SCL=%d, %d Hz)",
             I2C_MASTER_SDA, I2C_MASTER_SCL, I2C_MASTER_FREQ);
    return ESP_OK;
}

/* ========================================================================
 * SPI Master Initialization (for MAX31865 RTD)
 * ======================================================================== */
static esp_err_t init_spi(void)
{
    spi_bus_config_t buscfg = {
        .mosi_io_num = SPI_MOSI,
        .miso_io_num = SPI_MISO,
        .sclk_io_num = SPI_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 32,
    };
    esp_err_t err = spi_bus_initialize(SPI2_HOST, &buscfg, 1);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "SPI bus init failed: %s", esp_err_to_name(err));
        return err;
    }
    ESP_LOGI(TAG, "SPI bus initialized (MOSI=%d, MISO=%d, SCLK=%d)",
             SPI_MOSI, SPI_MISO, SPI_SCLK);
    return ESP_OK;
}

/* ========================================================================
 * UART Initialization (for Senseair S8 CO2 sensor)
 * ======================================================================== */
static esp_err_t init_co2_uart(void)
{
    uart_config_t cfg = {
        .baud_rate  = CO2_UART_BAUD,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    esp_err_t err = uart_driver_install(UART_NUM_2, 256, 256, 0, NULL, 0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "CO2 UART install failed: %s", esp_err_to_name(err));
        return err;
    }
    err = uart_param_config(UART_NUM_2, &cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "CO2 UART config failed: %s", esp_err_to_name(err));
        return err;
    }
    uart_set_pin(UART_NUM_2, CO2_UART_TX, CO2_UART_RX,
                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    ESP_LOGI(TAG, "CO2 UART initialized (TX=%d, RX=%d, %d baud)",
             CO2_UART_TX, CO2_UART_RX, CO2_UART_BAUD);
    return ESP_OK;
}

/* ========================================================================
 * ADC Initialization (for ISFET pH)
 * ======================================================================== */
static esp_err_t init_adc(void)
{
    adc1_config_width(PH_ADC_WIDTH);
    adc1_config_channel_atten(PH_ADC_CHANNEL, PH_ADC_ATTEN);
    ESP_LOGI(TAG, "ADC initialized for pH channel");
    return ESP_OK;
}

/* ========================================================================
 * GPIO Initialization
 * ======================================================================== */
static esp_err_t init_gpio(void)
{
    gpio_config_t io_conf = {0};

    /* Status LED */
    io_conf.pin_bit_mask = (1ULL << STATUS_LED_PIN);
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io_conf);
    gpio_set_level(STATUS_LED_PIN, STATUS_LED_ON);

    /* Charger status + USB VBUS detect (inputs) */
    io_conf.pin_bit_mask = (1ULL << CHARGER_STAT_PIN) | (1ULL << USB_VBUS_DETECT_PIN);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);

    ESP_LOGI(TAG, "GPIO initialized (LED=%d, CHG_STAT=%d, VBUS=%d)",
             STATUS_LED_PIN, CHARGER_STAT_PIN, USB_VBUS_DETECT_PIN);
    return ESP_OK;
}

/* ========================================================================
 * Default State Setup
 * ======================================================================== */
static void init_default_state(void)
{
    memset(&g_state, 0, sizeof(g_state));

    g_state.type = FERM_BEER;
    strncpy(g_state.batch_name, "Batch #1", sizeof(g_state.batch_name) - 1);
    g_state.vessel_volume_l = DEFAULT_VESSEL_VOL_L;
    g_state.temp_min_c = DEFAULT_TEMP_MIN_C;
    g_state.temp_max_c = DEFAULT_TEMP_MAX_C;
    g_state.ph_min = DEFAULT_PH_MIN;
    g_state.ph_max = DEFAULT_PH_MAX;
    g_state.co2_max_ppm = DEFAULT_CO2_MAX_PPM;
    g_state.spoilage_threshold = DEFAULT_SPOILAGE_THRESH;
    g_state.active = false;

    g_state.fusion.phase = PHASE_IDLE;
    g_state.fusion.batch_start_ms = 0;
}

/* ========================================================================
 * NVS: Load persisted batch configuration
 * ======================================================================== */
static void load_config_from_nvs(void)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open("fermentiq", NVS_READONLY, &h);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "No saved config, using defaults");
        return;
    }

    uint8_t type;
    if (nvs_get_u8(h, "ferm_type", &type) == ESP_OK)
        g_state.type = (fermentation_type_t)type;

    size_t len = sizeof(g_state.batch_name);
    nvs_get_str(h, "batch_name", g_state.batch_name, &len);

    nvs_get_f32(h, "vessel_vol", &g_state.vessel_volume_l);
    nvs_get_f32(h, "temp_min", &g_state.temp_min_c);
    nvs_get_f32(h, "temp_max", &g_state.temp_max_c);
    nvs_get_f32(h, "ph_min", &g_state.ph_min);
    nvs_get_f32(h, "ph_max", &g_state.ph_max);

    uint8_t active;
    if (nvs_get_u8(h, "active", &active) == ESP_OK)
        g_state.active = active ? true : false;

    nvs_close(h);
    ESP_LOGI(TAG, "Config loaded from NVS: type=%s, batch=%s, active=%d",
             ferm_type_names[g_state.type], g_state.batch_name, g_state.active);
}

/* ========================================================================
 * NVS: Save batch configuration
 * ======================================================================== */
void save_config_to_nvs(void)
{
    nvs_handle_t h;
    if (nvs_open("fermentiq", NVS_READWRITE, &h) != ESP_OK)
        return;

    nvs_set_u8(h, "ferm_type", (uint8_t)g_state.type);
    nvs_set_str(h, "batch_name", g_state.batch_name);
    nvs_set_f32(h, "vessel_vol", g_state.vessel_volume_l);
    nvs_set_f32(h, "temp_min", g_state.temp_min_c);
    nvs_set_f32(h, "temp_max", g_state.temp_max_c);
    nvs_set_f32(h, "ph_min", g_state.ph_min);
    nvs_set_f32(h, "ph_max", g_state.ph_max);
    nvs_set_u8(h, "active", g_state.active ? 1 : 0);
    nvs_commit(h);
    nvs_close(h);
    ESP_LOGI(TAG, "Config saved to NVS");
}

/* ========================================================================
 * Sensor Task: Impedance (AD5933)
 * Runs a frequency sweep every 60s and extracts biomass features.
 * ======================================================================== */
static void task_impedance(void *arg)
{
    (void)arg;
    ESP_LOGI(TAG, "[impedance] task started on core %d", xPortGetCoreID());

    /* Initialize AD5933 + analog switch for 4-wire Kelvin */
    if (ad5933_init() != 0) {
        ESP_LOGE(TAG, "[impedance] AD5933 init failed — task suspended");
        vTaskSuspend(NULL);
    }

    /* Calibration: measure known reference resistor (1000 ohm) */
    float cal_factor = ad5933_calibrate(1000.0f);
    ESP_LOGI(TAG, "[impedance] calibration factor: %.4f", cal_factor);

    ad5933_sweep_result_t sweep;

    while (1) {
        uint64_t now = esp_timer_get_time() / 1000;

        if (g_state.active) {
            int rc = ad5933_run_sweep(&sweep);
            if (rc == 0) {
                impedance_data_t data;
                memset(&data, 0, sizeof(data));

                /* Extract 8 features from the sweep */
                ad5933_extract_features(&sweep, &data, cal_factor);

                /* Run biomass estimation model (TinyML) */
                data.cell_density_log = fusion_predict_biomass(&data);
                data.cell_density = powf(10.0f, data.cell_density_log);
                data.timestamp_ms = now;
                data.valid = true;

                state_lock();
                memcpy(&g_state.impedance, &data, sizeof(data));
                state_unlock();

                ESP_LOGI(TAG, "[impedance] |Z|1k=%.0f |Z|10k=%.0f |Z|100k=%.0f "
                         "cells/mL=%.2e (log=%.2f)",
                         data.z_mag_1k, data.z_mag_10k, data.z_mag_100k,
                         data.cell_density, data.cell_density_log);
            } else {
                ESP_LOGW(TAG, "[impedance] sweep failed (rc=%d)", rc);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(SAMPLE_INTERVAL_IMPEDANCE));
    }
}

/* ========================================================================
 * Sensor Task: CO2 (Senseair S8)
 * Polls NDIR sensor every 15s, computes CO2 evolution rate.
 * ======================================================================== */
static void task_co2(void *arg)
{
    (void)arg;
    ESP_LOGI(TAG, "[co2] task started on core %d", xPortGetCoreID());

    if (co2_ndir_init() != 0) {
        ESP_LOGE(TAG, "[co2] S8 init failed — task suspended");
        vTaskSuspend(NULL);
    }

    /* CO2 history for rate computation (ring buffer, 8 samples = 2 min) */
    #define CO2_HIST_LEN 8
    static uint16_t co2_hist[CO2_HIST_LEN];
    static uint64_t time_hist[CO2_HIST_LEN];
    static int hist_idx = 0;
    static int hist_count = 0;

    while (1) {
        uint64_t now = esp_timer_get_time() / 1000;
        uint16_t ppm = 0;

        if (co2_ndir_read_ppm(&ppm) == 0) {
            co2_hist[hist_idx] = ppm;
            time_hist[hist_idx] = now;
            hist_idx = (hist_idx + 1) % CO2_HIST_LEN;
            if (hist_count < CO2_HIST_LEN) hist_count++;

            /* Compute CER (CO2 Evolution Rate) via linear regression slope */
            float cer = 0.0f;
            if (hist_count >= 4) {
                float sum_t = 0, sum_c = 0, sum_tt = 0, sum_tc = 0;
                int start = (hist_idx - hist_count + CO2_HIST_LEN) % CO2_HIST_LEN;
                for (int i = 0; i < hist_count; i++) {
                    int idx = (start + i) % CO2_HIST_LEN;
                    float t_h = (float)time_hist[idx] / 3600000.0f; /* hours */
                    float c = (float)co2_hist[idx];
                    sum_t += t_h; sum_c += c;
                    sum_tt += t_h * t_h; sum_tc += t_h * c;
                }
                float n = (float)hist_count;
                float denom = n * sum_tt - sum_t * sum_t;
                if (fabsf(denom) > 1e-9f) {
                    float slope = (n * sum_tc - sum_t * sum_c) / denom;
                    /* Convert ppm/hour to mmol/L/hour (ideal gas at STP) */
                    /* 1 ppm = 1e-6 atm; 1 mol gas = 24.465 L at 25C */
                    cer = slope * 1e-6f * 1000.0f / 24.465f *
                          g_state.vessel_volume_l;
                }
            }

            /* Estimate dissolved CO2 via Henry's Law */
            float co2_partial_atm = (float)ppm * 1e-6f;
            float dissolved = co2_partial_atm * HENRY_CO2_25C *
                              44.01f * 1000.0f; /* g/L */

            state_lock();
            g_state.co2.co2_ppm = ppm;
            g_state.co2.cer_mmol_lh = cer;
            g_state.co2.co2_dissolved = dissolved;
            g_state.co2.timestamp_ms = now;
            g_state.co2.valid = true;
            state_unlock();

            ESP_LOGI(TAG, "[co2] ppm=%u CER=%.2f mmol/L/h dissolved=%.2f g/L",
                     ppm, cer, dissolved);
        } else {
            ESP_LOGW(TAG, "[co2] read failed");
        }

        vTaskDelay(pdMS_TO_TICKS(SAMPLE_INTERVAL_CO2));
    }
}

/* ========================================================================
 * Sensor Task: pH (ISFET + LMP91200)
 * Reads pH every 30s, computes pH rate of change.
 * ======================================================================== */
static void task_ph(void *arg)
{
    (void)arg;
    ESP_LOGI(TAG, "[ph] task started on core %d", xPortGetCoreID());

    if (ph_isfet_init() != 0) {
        ESP_LOGE(TAG, "[ph] ISFET init failed — task suspended");
        vTaskSuspend(NULL);
    }

    /* pH history for rate computation */
    #define PH_HIST_LEN 6
    static float ph_hist[PH_HIST_LEN];
    static uint64_t ph_time[PH_HIST_LEN];
    static int ph_idx = 0, ph_count = 0;

    while (1) {
        uint64_t now = esp_timer_get_time() / 1000;
        float ph_val = 0.0f, raw_mv = 0.0f;

        if (ph_isfet_read(&ph_val, &raw_mv) == 0) {
            ph_hist[ph_idx] = ph_val;
            ph_time[ph_idx] = now;
            ph_idx = (ph_idx + 1) % PH_HIST_LEN;
            if (ph_count < PH_HIST_LEN) ph_count++;

            /* Compute pH rate (pH/hour) via linear regression */
            float rate = 0.0f;
            if (ph_count >= 3) {
                float sum_t = 0, sum_p = 0, sum_tt = 0, sum_tp = 0;
                int start = (ph_idx - ph_count + PH_HIST_LEN) % PH_HIST_LEN;
                for (int i = 0; i < ph_count; i++) {
                    int idx = (start + i) % PH_HIST_LEN;
                    float t_h = (float)ph_time[idx] / 3600000.0f;
                    float p = ph_hist[idx];
                    sum_t += t_h; sum_p += p;
                    sum_tt += t_h * t_h; sum_tp += t_h * p;
                }
                float n = (float)ph_count;
                float denom = n * sum_tt - sum_t * sum_t;
                if (fabsf(denom) > 1e-12f)
                    rate = (n * sum_tp - sum_t * sum_p) / denom;
            }

            state_lock();
            g_state.ph.ph = ph_val;
            g_state.ph.ph_raw_mv = raw_mv;
            g_state.ph.ph_rate = rate;
            g_state.ph.timestamp_ms = now;
            g_state.ph.valid = true;
            state_unlock();

            ESP_LOGI(TAG, "[ph] pH=%.2f raw=%.1f mV rate=%.3f pH/h",
                     ph_val, raw_mv, rate);
        } else {
            ESP_LOGW(TAG, "[ph] read failed");
        }

        vTaskDelay(pdMS_TO_TICKS(SAMPLE_INTERVAL_PH));
    }
}

/* ========================================================================
 * Sensor Task: Temperature (MAX31865 RTD) + Ambient (SHT41)
 * ======================================================================== */
static void task_temp(void *arg)
{
    (void)arg;
    ESP_LOGI(TAG, "[temp] task started on core %d", xPortGetCoreID());

    if (temp_rtd_init() != 0) {
        ESP_LOGE(TAG, "[temp] MAX31865 init failed — task suspended");
        vTaskSuspend(NULL);
    }

    while (1) {
        uint64_t now = esp_timer_get_time() / 1000;
        float liquid_c = 0.0f;

        if (temp_rtd_read(&liquid_c) == 0) {
            float amb_t = 0.0f, amb_rh = 0.0f;
            temp_ambient_read(&amb_t, &amb_rh);

            /* Compute dew point (Magnus formula) */
            float a = 17.27f, b = 237.7f;
            float alpha = (a * amb_t) / (b + amb_t) +
                          logf(amb_rh / 100.0f + 1e-6f);
            float dew = (b * alpha) / (a - alpha);

            state_lock();
            g_state.temp.temp_c = liquid_c;
            g_state.temp.ambient_temp_c = amb_t;
            g_state.temp.ambient_rh = amb_rh;
            g_state.temp.dew_point_c = dew;
            g_state.temp.timestamp_ms = now;
            g_state.temp.valid = true;
            state_unlock();

            ESP_LOGI(TAG, "[temp] liquid=%.2f°C amb=%.1f°C/%.1f%%RH dew=%.1f°C",
                     liquid_c, amb_t, amb_rh, dew);
        } else {
            ESP_LOGW(TAG, "[temp] RTD read failed");
        }

        vTaskDelay(pdMS_TO_TICKS(SAMPLE_INTERVAL_TEMP));
    }
}

/* ========================================================================
 * Sensor Task: Acoustic Bubble Detection (I2S MEMS Mic + FFT)
 * ======================================================================== */
static void task_acoustic(void *arg)
{
    (void)arg;
    ESP_LOGI(TAG, "[acoustic] task started on core %d", xPortGetCoreID());

    if (acoustic_init() != 0) {
        ESP_LOGE(TAG, "[acoustic] init failed — task suspended");
        vTaskSuspend(NULL);
    }

    float bubble_rate = 0.0f;
    float centroid = 0.0f;
    float rms = 0.0f;

    while (1) {
        uint64_t now = esp_timer_get_time() / 1000;

        if (acoustic_process(&bubble_rate, &centroid, &rms) == 0) {
            state_lock();
            g_state.acoustic.bubble_rate = bubble_rate;
            g_state.acoustic.spectral_centroid = centroid;
            g_state.acoustic.rms_level = rms;
            g_state.acoustic.timestamp_ms = now;
            g_state.acoustic.valid = true;
            state_unlock();

            if (bubble_rate > 0.1f) {
                ESP_LOGI(TAG, "[acoustic] bubbles=%.1f/min centroid=%.0f Hz "
                         "rms=%.4f", bubble_rate, centroid, rms);
            }
        }

        /* Process audio every 500ms (FFT window) */
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/* ========================================================================
 * Fusion Task: Sensor Fusion + TinyML Inference
 * Runs every 60s, combines all sensor data into phase/ABV/spoilage.
 * ======================================================================== */
static void task_fusion(void *arg)
{
    (void)arg;
    ESP_LOGI(TAG, "[fusion] task started on core %d", xPortGetCoreID());

    /* Initialize fusion model */
    if (fusion_init() != 0) {
        ESP_LOGE(TAG, "[fusion] init failed — task suspended");
        vTaskSuspend(NULL);
    }

    /* Integrated CO2 for ABV estimation */
    static float co2_total_mol = 0.0f;
    static uint64_t last_fusion_ms = 0;

    while (1) {
        uint64_t now = esp_timer_get_time() / 1000;

        if (g_state.active) {
            state_lock();
            /* Snapshot all sensor data under lock */
            impedance_data_t imp = g_state.impedance;
            co2_data_t co2 = g_state.co2;
            ph_data_t ph = g_state.ph;
            temp_data_t temp = g_state.temp;
            acoustic_data_t ac = g_state.acoustic;
            fusion_data_t prev_fusion = g_state.fusion;
            state_unlock();

            /* Integrate CO2 evolution rate for ABV estimation */
            if (last_fusion_ms > 0 && co2.valid) {
                float dt_h = (float)(now - last_fusion_ms) / 3600000.0f;
                co2_total_mol += co2.cer_mmol_lh * dt_h *
                                 g_state.vessel_volume_l / 1000.0f;
            }
            last_fusion_ms = now;

            /* Compute batch age */
            uint32_t age_h = 0;
            if (prev_fusion.batch_start_ms > 0)
                age_h = (uint32_t)((now - prev_fusion.batch_start_ms) / 3600000);

            /* Run fusion inference */
            fusion_result_t result;
            fusion_infer(&imp, &co2, &ph, &temp, &ac, &prev_fusion,
                         co2_total_mol, age_h, &result);

            state_lock();
            g_state.fusion.phase = result.phase;
            g_state.fusion.abv_estimate = result.abv_estimate;
            g_state.fusion.attenuation = result.attenuation;
            g_state.fusion.spoilage_risk = result.spoilage_risk;
            g_state.fusion.health_score = result.health_score;
            if (g_state.fusion.batch_start_ms == 0)
                g_state.fusion.batch_start_ms = now;
            g_state.fusion.batch_age_hours = age_h;
            g_state.fusion.timestamp_ms = now;
            state_unlock();

            ESP_LOGI(TAG, "[fusion] phase=%s ABV=%.1f%% att=%.1f%% "
                     "spoil=%d health=%d age=%luh",
                     phase_names[result.phase], result.abv_estimate,
                     result.attenuation, result.spoilage_risk,
                     result.health_score, (unsigned long)age_h);

            /* Check alarm conditions */
            bool alarm = false;
            const char *alarm_msg = NULL;

            if (temp.valid && (temp.temp_c < g_state.temp_min_c ||
                               temp.temp_c > g_state.temp_max_c)) {
                alarm = true;
                alarm_msg = "Temperature out of range";
            }
            if (ph.valid && (ph.ph < g_state.ph_min ||
                             ph.ph > g_state.ph_max)) {
                alarm = true;
                alarm_msg = "pH out of range";
            }
            if (co2.valid && co2.co2_ppm > g_state.co2_max_ppm) {
                alarm = true;
                alarm_msg = "CO2 concentration too high";
            }
            if (result.spoilage_risk >= g_state.spoilage_threshold) {
                alarm = true;
                alarm_msg = "Spoilage risk threshold exceeded";
            }
            if (result.phase == PHASE_STUCK) {
                alarm = true;
                alarm_msg = "Fermentation appears stuck";
            }

            if (alarm) {
                ESP_LOGW(TAG, "[fusion] ALARM: %s", alarm_msg);
                ble_send_alert(alarm_msg, result.spoilage_risk);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(FUSION_INTERVAL));
    }
}

/* ========================================================================
 * BLE Task
 * ======================================================================== */
static void task_ble(void *arg)
{
    (void)arg;
    ESP_LOGI(TAG, "[ble] task started on core %d", xPortGetCoreID());

    if (ble_init() != 0) {
        ESP_LOGE(TAG, "[ble] init failed — task suspended");
        vTaskSuspend(NULL);
    }

    while (1) {
        ble_process();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/* ========================================================================
 * WiFi + MQTT Task
 * ======================================================================== */
static void task_wifi(void *arg)
{
    (void)arg;
    ESP_LOGI(TAG, "[wifi] task started on core %d", xPortGetCoreID());

    if (wifi_mqtt_init() != 0) {
        ESP_LOGE(TAG, "[wifi] init failed — task suspended");
        vTaskSuspend(NULL);
    }

    while (1) {
        if (g_state.active) {
            state_lock();
            fermentiq_state_t snapshot = g_state;
            state_unlock();
            wifi_mqtt_publish(&snapshot);
        }
        vTaskDelay(pdMS_TO_TICKS(MQTT_PUBLISH_INTERVAL));
    }
}

/* ========================================================================
 * Logger Task
 * ======================================================================== */
static void task_logger(void *arg)
{
    (void)arg;
    ESP_LOGI(TAG, "[logger] task started on core %d", xPortGetCoreID());

    if (storage_init() != 0) {
        ESP_LOGW(TAG, "[logger] storage init failed — logging to flash only");
    }

    while (1) {
        if (g_state.active) {
            state_lock();
            fermentiq_state_t snapshot = g_state;
            state_unlock();
            storage_log_sample(&snapshot);
        }
        vTaskDelay(pdMS_TO_TICKS(LOG_FLUSH_INTERVAL));
    }
}

/* ========================================================================
 * Power Monitor (called periodically from main loop)
 * ======================================================================== */
static void update_power_status(void)
{
    float soc = 0.0f, mv = 0.0f;
    bool usb = (gpio_get_level(USB_VBUS_DETECT_PIN) == 1);

    if (power_read_battery(&soc, &mv) == 0) {
        state_lock();
        g_state.battery_soc = soc;
        g_state.battery_mv = mv;
        g_state.usb_connected = usb;
        state_unlock();
    }

    /* Low battery warning */
    if (!usb && mv < BATTERY_LOW_MV && mv > 0) {
        ESP_LOGW(TAG, "Low battery: %.0f mV (%.0f%%)", mv, soc);
    }
}

/* ========================================================================
 * Main Entry Point
 * ======================================================================== */
void app_main(void)
{
    ESP_LOGI(TAG, "=== FermenTiq v%s ===", FERMENTIQ_VERSION);
    ESP_LOGI(TAG, "Author: %s", FERMENTIQ_AUTHOR);
    ESP_LOGI(TAG, "Build: %s", FERMENTIQ_BUILD_DATE);
    ESP_LOGI(TAG, "Free heap: %lu bytes", (unsigned long)esp_get_free_heap_size());

    /* NVS */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }
    ESP_LOGI(TAG, "NVS initialized");

    /* State mutex */
    g_state_mutex = (void *)xSemaphoreCreateMutex();
    if (!g_state_mutex) {
        ESP_LOGE(TAG, "Failed to create state mutex");
        return;
    }

    /* Default state + load config */
    init_default_state();
    load_config_from_nvs();

    /* Hardware init */
    init_gpio();
    init_i2c();
    init_spi();
    init_co2_uart();
    init_adc();
    power_init();

    ESP_LOGI(TAG, "All hardware initialized. Starting tasks...");

    /* Create sensor tasks.
     * Pin high-priority / continuous tasks to core 0, periodic tasks to core 1.
     */
    xTaskCreatePinnedToCore(task_acoustic,  "acoustic",  8192, NULL, 6, &h_acoustic,  0);
    xTaskCreatePinnedToCore(task_fusion,    "fusion",    6144, NULL, 7, &h_fusion,    0);
    xTaskCreatePinnedToCore(task_ph,        "ph",        2048, NULL, 3, &h_ph,        0);
    xTaskCreatePinnedToCore(task_temp,      "temp",      2048, NULL, 3, &h_temp,      0);
    xTaskCreatePinnedToCore(task_ble,       "ble",       4096, NULL, 4, &h_ble,       0);
    xTaskCreatePinnedToCore(task_wifi,      "wifi",      4096, NULL, 3, &h_wifi,      0);
    xTaskCreatePinnedToCore(task_logger,    "logger",    3072, NULL, 2, &h_logger,    0);
    xTaskCreatePinnedToCore(task_impedance, "impedance", 4096, NULL, 5, &h_impedance, 1);
    xTaskCreatePinnedToCore(task_co2,       "co2",       2048, NULL, 4, &h_co2,       1);

    ESP_LOGI(TAG, "All tasks started. System running.");
    ESP_LOGI(TAG, "Batch: %s (%s) active=%s",
             g_state.batch_name, ferm_type_names[g_state.type],
             g_state.active ? "YES" : "NO");

    /* Main loop: periodic power monitoring + watchdog */
    while (1) {
        update_power_status();
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}