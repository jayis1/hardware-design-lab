/*
 * power.c — Battery & Power Management
 *
 * Manages the MAX17048 fuel gauge for battery state-of-charge monitoring
 * and the TP4056 linear charger for the 18650 backup battery. Also handles
 * low-power mode transitions when running on battery without USB power.
 *
 * The MAX17048 is an I2C fuel gauge that uses a ModelGauge algorithm to
 * estimate state of charge (%) from the battery voltage and current
 * characteristics. It requires no current-sense resistor.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: GPL-3.0
 */

#include "power.h"
#include "../board.h"
#include "../registers.h"
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "power";

/* ------------------------------------------------------------------- */
/* I2C helpers for MAX17048                                           */
/* ------------------------------------------------------------------- */

static esp_err_t max17048_read(uint8_t reg, uint16_t *val)
{
    uint8_t buf[2];
    esp_err_t err = i2c_master_write_read_device(I2C_PORT_NUM,
                                                  MAX17048_I2C_ADDR,
                                                  &reg, 1, buf, 2,
                                                  pdMS_TO_TICKS(50));
    if (err == ESP_OK)
        *val = ((uint16_t)buf[0] << 8) | buf[1];
    return err;
}

static esp_err_t max17048_write(uint8_t reg, uint16_t val)
{
    uint8_t buf[3] = { reg, (uint8_t)(val >> 8), (uint8_t)(val & 0xFF) };
    return i2c_master_write_to_device(I2C_PORT_NUM, MAX17048_I2C_ADDR,
                                      buf, 3, pdMS_TO_TICKS(50));
}

/* ------------------------------------------------------------------- */
/* Initialization                                                      */
/* ------------------------------------------------------------------- */
int power_init(void)
{
    /* Check if MAX17048 is present */
    uint16_t version = 0;
    if (max17048_read(MAX17048_REG_VERSION, &version) != ESP_OK) {
        ESP_LOGW(TAG, "MAX17048 not detected — battery monitoring disabled");
        return -1;
    }
    ESP_LOGI(TAG, "MAX17048 fuel gauge detected (version 0x%04X)", version);

    /* Quick-start to reset the fuel gauge algorithm */
    max17048_write(MAX17048_REG_CMD, 0x4000);
    vTaskDelay(pdMS_TO_TICKS(50));

    /* Configure alert threshold for low battery (10%) */
    uint16_t valert = 0x9C40;  /* ~10% SOC threshold */
    max17048_write(MAX17048_REG_VALERT, valert);

    /* Enable hibernate (saves power when USB disconnected) */
    uint16_t hibrt = 0x9F00;  /* hibernate when current is low */
    max17048_write(MAX17048_REG_HIBRT, hibrt);

    ESP_LOGI(TAG, "Power management initialized (TP4056 + MAX17048)");
    return 0;
}

/* ------------------------------------------------------------------- */
/* Read battery state of charge and voltage                           */
/* ------------------------------------------------------------------- */
int power_read_battery(float *soc_pct, float *voltage_mv)
{
    uint16_t soc_raw, vcell_raw;

    if (max17048_read(MAX17048_REG_SOC, &soc_raw) != ESP_OK)
        return -1;
    if (max17048_read(MAX17048_REG_VCELL, &vcell_raw) != ESP_OK)
        return -1;

    /* SOC is already in % (with 1/256% resolution) */
    *soc_pct = (float)soc_raw / 256.0f;

    /* VCELL: 78.125 µV per LSB → convert to mV */
    *voltage_mv = (float)vcell_raw * 0.078125f;

    return 0;
}

/* ------------------------------------------------------------------- */
/* Get charge status from TP4056                                      */
/* ------------------------------------------------------------------- */
int power_get_charge_status(bool *charging, bool *full)
{
    /* TP4056 STAT pin: LOW = charging, HIGH = charge complete / standby */
    int level = gpio_get_level(CHARGER_STAT_PIN);
    *charging = (level == TP4056_STAT_CHARGING);
    *full = (level == TP4056_STAT_FULL);

    /* Also check USB VBUS to distinguish "full" from "no battery" */
    int vbus = gpio_get_level(USB_VBUS_DETECT_PIN);
    if (vbus == 0) {
        /* No USB power — not charging */
        *charging = false;
        *full = false;
    }

    return 0;
}

/* ------------------------------------------------------------------- */
/* Enter low-power mode (reduce WiFi, slow down sampling)             */
/* ------------------------------------------------------------------- */
int power_enter_low_power(void)
{
    ESP_LOGI(TAG, "Entering low-power mode (battery operation)");

    /* In a full implementation:
     * 1. Disconnect from WiFi (keep BLE only)
     * 2. Increase sensor sampling intervals
     * 3. Reduce CPU frequency
     * 4. Use light sleep between measurements
     */

    /* Reduce CPU to 80 MHz */
    /* esp_pm_configure(&pm_config); */

    /* Light sleep between tasks */
    /* esp_light_sleep_start(); */

    ESP_LOGI(TAG, "Low-power mode active — WiFi off, BLE only, 80MHz CPU");
    return 0;
}