/*
 * temp_rtd.c — MAX31865 PT100 RTD + SHT41 Ambient Driver
 *
 * SPI driver for the MAX31865 RTD-to-digital converter connected to a
 * 3-wire PT100 sensor (Class A, 1/10 DIN) for precise liquid temperature
 * measurement. Also includes I2C driver for the SHT41 ambient
 * temperature/humidity sensor used for dew-point computation.
 *
 * The MAX31865 uses a 15-bit ADC to measure the RTD resistance ratio
 * (RTD / Rref). Temperature is computed via the Callendar-Van Dusen
 * equation for PT100 sensors.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: GPL-3.0
 */

#include "temp_rtd.h"
#include "../board.h"
#include "../registers.h"
#include "driver/spi_master.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>
#include <string.h>

static const char *TAG = "temp_rtd";

static spi_device_handle_t s_rtd_spi;

/* ------------------------------------------------------------------- */
/* SPI transfer helper                                                 */
/* ------------------------------------------------------------------- */

static void rtd_write_reg(uint8_t addr, uint8_t val)
{
    spi_transaction_t t = {0};
    uint8_t tx[2] = { addr & 0x7F, val };  /* MSB=0 for write */
    t.tx_buffer = tx;
    t.length = 16;
    spi_device_polling_transmit(s_rtd_spi, &t);
}

static uint8_t rtd_read_reg(uint8_t addr)
{
    spi_transaction_t t = {0};
    uint8_t tx[2] = { addr | 0x80, 0x00 };  /* MSB=1 for read */
    uint8_t rx[2] = {0};
    t.tx_buffer = tx;
    t.rx_buffer = rx;
    t.length = 16;
    spi_device_polling_transmit(s_rtd_spi, &t);
    return rx[1];
}

/* ------------------------------------------------------------------- */
/* Callendar-Van Dusen equation: RTD resistance → temperature          */
/* ------------------------------------------------------------------- */

static float rtd_resistance_to_temp(float r_rtd)
{
    /* Callendar-Van Dusen coefficients for PT100 (IEC 60751):
     *   R(t) = R0 * (1 + A*t + B*t^2)         for t >= 0
     *   R(t) = R0 * (1 + A*t + B*t^2 + C*(t-100)*t^3)  for t < 0
     *
     *   A = 3.9083e-3, B = -5.775e-7, C = -4.183e-12
     */
    const float A = 3.9083e-3f;
    const float B = -5.775e-7f;
    const float C = -4.183e-12f;

    if (r_rtd <= 0.0f)
        return -999.0f;

    float ratio = r_rtd / RTD_RNOM_OHMS;

    /* For t >= 0°C: solve quadratic: R/R0 = 1 + A*t + B*t^2 */
    if (ratio >= 1.0f) {
        float t = (-A + sqrtf(A * A - 4.0f * B * (1.0f - ratio))) / (2.0f * B);
        return t;
    }

    /* For t < 0°C: use iterative approach (C term makes it quartic) */
    /* Simplified: linear approximation below 0°C, refined with Newton-Raphson */
    float t = (ratio - 1.0f) / A;  /* initial estimate */
    for (int i = 0; i < 10; i++) {
        float f = 1.0f + A * t + B * t * t +
                  C * (t - 100.0f) * t * t * t - ratio;
        float df = A + 2.0f * B * t +
                   C * (4.0f * t * t * t - 300.0f * t * t);
        if (fabsf(df) < 1e-12f)
            break;
        t = t - f / df;
    }
    return t;
}

/* ------------------------------------------------------------------- */
/* MAX31865 initialization                                             */
/* ------------------------------------------------------------------- */

int temp_rtd_init(void)
{
    /* Configure SPI device */
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = SPI_BUS_MAX_FREQ,
        .mode = 1,              /* MAX31865 uses SPI mode 1 (CPOL=0, CPHA=1) */
        .spics_io_num = RTD_CS,
        .queue_size = 1,
        .flags = SPI_DEVICE_HALFDUPLEX,
    };
    esp_err_t err = spi_bus_add_device(SPI2_HOST, &devcfg, &s_rtd_spi);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "SPI device add failed: %s", esp_err_to_name(err));
        return -1;
    }

    vTaskDelay(pdMS_TO_TICKS(10));

    /* Configure MAX31865:
     *  - VBIAS enabled (needed for conversion)
     *  - Auto conversion mode (continuous)
     *  - 50Hz filter (European mains; use 60Hz in US — configurable)
     *  - 3-wire RTD mode (set bit 4 = 1 for 3-wire) */
    uint8_t config = MAX31865_CFG_VBIAS | MAX31865_CFG_CONV_AUTO |
                     MAX31865_CFG_50HZ | 0x10;  /* 3-wire mode */
    rtd_write_reg(MAX31865_REG_CONFIG, config);

    /* Clear any existing fault */
    rtd_write_reg(MAX31865_REG_CONFIG, config | MAX31865_CFG_CONV_FAULT_CLR);

    vTaskDelay(pdMS_TO_TICKS(50));

    ESP_LOGI(TAG, "MAX31865 initialized (PT100 3-wire, 50Hz filter)");
    return 0;
}

/* ------------------------------------------------------------------- */
/* Read RTD temperature                                                */
/* ------------------------------------------------------------------- */

int temp_rtd_read(float *temp_c)
{
    /* Read 2-byte RTD register */
    uint8_t msb = rtd_read_reg(MAX31865_REG_RTD_MSB);
    uint8_t lsb = rtd_read_reg(MAX31865_REG_RTD_LSB);

    /* Check for fault bit (D0 of LSB) */
    if (lsb & 0x01) {
        uint8_t fault = rtd_read_reg(MAX31865_REG_FAULT);
        ESP_LOGW(TAG, "RTD fault: 0x%02X", fault);
        /* Clear fault */
        uint8_t cfg = rtd_read_reg(MAX31865_REG_CONFIG);
        rtd_write_reg(MAX31865_REG_CONFIG, cfg | MAX31865_CFG_CONV_FAULT_CLR);
        return -1;
    }

    /* 15-bit RTD code: MSB[7:0] + LSB[7:1] → 15 bits */
    uint16_t rtd_code = ((uint16_t)msb << 7) | (lsb >> 1);

    if (rtd_code == 0) {
        ESP_LOGW(TAG, "RTD code is 0 — connection issue");
        return -1;
    }

    /* Convert to resistance: R_rtd = (code / 2^15) * R_ref */
    float r_rtd = ((float)rtd_code / 32768.0f) * RTD_RREF_OHMS;

    /* Convert resistance to temperature */
    *temp_c = rtd_resistance_to_temp(r_rtd);

    return 0;
}

/* ------------------------------------------------------------------- */
/* SHT41 ambient temperature & humidity (I2C)                          */
/* ------------------------------------------------------------------- */

static uint8_t sht41_crc(const uint8_t *data, int len)
{
    uint8_t crc = 0xFF;
    for (int i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x31;
            else
                crc <<= 1;
        }
    }
    return crc;
}

int temp_ambient_read(float *temp_c, float *rh_pct)
{
    /* Send high-repeatability measurement command */
    uint8_t cmd = SHT41_CMD_MEAS_HIGHREP;
    esp_err_t err = i2c_master_write_to_device(I2C_PORT_NUM, SHT41_I2C_ADDR,
                                               &cmd, 1, pdMS_TO_TICKS(50));
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "SHT41 write failed: %s", esp_err_to_name(err));
        *temp_c = 25.0f;
        *rh_pct = 50.0f;
        return -1;
    }

    vTaskDelay(pdMS_TO_TICKS(SHT41_MEAS_TIMEOUT_MS));

    /* Read 6 bytes: T_MSB, T_LSB, T_CRC, RH_MSB, RH_LSB, RH_CRC */
    uint8_t rx[6];
    err = i2c_master_read_from_device(I2C_PORT_NUM, SHT41_I2C_ADDR,
                                      rx, 6, pdMS_TO_TICKS(100));
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "SHT41 read failed: %s", esp_err_to_name(err));
        *temp_c = 25.0f;
        *rh_pct = 50.0f;
        return -1;
    }

    /* Verify CRC */
    if (sht41_crc(&rx[0], 2) != rx[2] || sht41_crc(&rx[3], 2) != rx[5]) {
        ESP_LOGW(TAG, "SHT41 CRC error");
        *temp_c = 25.0f;
        *rh_pct = 50.0f;
        return -1;
    }

    /* Convert raw to physical values */
    uint16_t t_raw = ((uint16_t)rx[0] << 8) | rx[1];
    uint16_t rh_raw = ((uint16_t)rx[3] << 8) | rx[4];

    *temp_c = -45.0f + 175.0f * (float)t_raw / 65535.0f;
    float rh = -6.0f + 125.0f * (float)rh_raw / 65535.0f;
    *rh_pct = CLAMP(rh, 0.0f, 100.0f);

    return 0;
}