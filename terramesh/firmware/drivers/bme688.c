/**
 * @file    bme688.c
 * @brief   Terramesh — Bosch BME688 environmental sensor driver
 * @author  jayis1
 * @copyright Copyright © 2026 jayis1
 * @license GPL-2.0
 *
 * I²C driver for the BME688 integrated environmental sensor (temperature,
 * humidity, barometric pressure, and gas/VOC). Used for atmospheric
 * compensation of pore pressure measurements.
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "../board.h"
#include "../registers.h"

/* BME688 register map */
#define BME688_REG_CHIP_ID       0xD0
#define BME688_REG_RESET         0xE0
#define BME688_REG_CTRL_HUM      0x72
#define BME688_REG_CTRL_MEAS     0x74
#define BME688_REG_CONFIG        0x75
#define BME688_REG_STATUS        0x73
#define BME688_REG_DATA_START    0x1D  /* Start of sensor data (20 bytes) */
#define BME688_REG_CALIB_START   0x8A  /* Start of calibration data (26 bytes) */
#define BME688_REG_CALIB2_START  0xE1  /* Second calibration block (16 bytes) */

/* BME688 chip ID */
#define BME688_CHIP_ID           0x61

/* BME688 oversampling settings */
#define BME688_OSRS_NONE         0x00
#define BME688_OSRS_1X           0x01
#define BME688_OSRS_2X           0x02
#define BME688_OSRS_4X           0x03
#define BME688_OSRS_8X           0x04
#define BME688_OSRS_16X          0x05

/* BME688 IIR filter settings */
#define BME688_FILTER_OFF        0x00
#define BME688_FILTER_2          0x01
#define BME688_FILTER_4          0x02
#define BME688_FILTER_8          0x03
#define BME688_FILTER_16         0x04

/* BME688 operating modes */
#define BME688_MODE_SLEEP        0x00
#define BME688_MODE_FORCED       0x01
#define BME688_MODE_NORMAL       0x03

/* ======================================================================== *
 *  Calibration data structure                                                  *
 * ======================================================================== */
typedef struct {
    /* Temperature calibration */
    uint16_t par_t1;
    int16_t  par_t2;
    int8_t   par_t3;

    /* Pressure calibration */
    uint16_t par_p1;
    int16_t  par_p2;
    int8_t   par_p3;
    int16_t  par_p4;
    int16_t  par_p5;
    int8_t   par_p6;
    int8_t   par_p7;
    int16_t  par_p8;
    int16_t  par_p9;
    int8_t   par_p10;

    /* Humidity calibration */
    uint16_t par_h1;
    uint16_t par_h2;
    int8_t   par_h3;
    int8_t   par_h4;
    int8_t   par_h5;
    uint8_t  par_h6;
    int8_t   par_h7;

    /* Gas calibration */
    int8_t   par_g1;
    int16_t  par_g2;
    int8_t   par_g3;
    uint8_t  par_g4;
    int8_t   par_g5;
    uint8_t  par_g6;
    int8_t   par_g7;
    int8_t   par_g8;
    int16_t  par_g9;
    uint8_t  par_g10;
    int8_t   par_g11;
    int8_t   par_g12;
    int8_t   par_g13;
    int16_t  par_g14;
    int16_t  par_g15;
    int16_t  par_g16;
    int8_t   par_g17;
    int16_t  par_g18;
    int16_t  par_g19;
    int8_t   par_g20;
    int8_t   par_g21;
    int8_t   par_g22;
    int8_t   par_g23;
    int8_t   par_g24;
    int8_t   par_g25;
    int8_t   par_g26;
    uint8_t  res_heat_range;
    int8_t   res_heat_val;
    int8_t   range_sw_err;
} bme688_calib_t;

/* ======================================================================== *
 *  Private state                                                               *
 * ======================================================================== */
static bme688_calib_t g_calib;
static bool g_calib_loaded = false;
static int32_t g_t_fine = 0;  /* Fine temperature value for compensation */

/* ======================================================================== *
 *  Private helpers                                                             *
 * ======================================================================== */

static bool bme688_read_reg(uint8_t reg, uint8_t *val) {
    if (!i2c1_write(I2C_ADDR_BME688, &reg, 1)) return false;
    return i2c1_read(I2C_ADDR_BME688, val, 1);
}

static bool bme688_write_reg(uint8_t reg, uint8_t val) {
    uint8_t data[] = { reg, val };
    return i2c1_write(I2C_ADDR_BME688, data, 2);
}

static bool bme688_read_burst(uint8_t start_reg, uint8_t *data, uint32_t len) {
    if (!i2c1_write(I2C_ADDR_BME688, &start_reg, 1)) return false;
    return i2c1_read(I2C_ADDR_BME688, data, len);
}

static bool bme688_load_calibration(void) {
    uint8_t calib1[26];
    uint8_t calib2[16];

    if (!bme688_read_burst(BME688_REG_CALIB_START, calib1, 26)) return false;
    if (!bme688_read_burst(BME688_REG_CALIB2_START, calib2, 16)) return false;

    /* Parse calibration data */
    g_calib.par_t1 = (uint16_t)calib1[0] | ((uint16_t)calib1[1] << 8);
    g_calib.par_t2 = (int16_t)((uint16_t)calib1[2] | ((uint16_t)calib1[3] << 8));
    g_calib.par_t3 = (int8_t)calib1[4];

    g_calib.par_p1 = (uint16_t)calib1[5] | ((uint16_t)calib1[6] << 8);
    g_calib.par_p2 = (int16_t)((uint16_t)calib1[7] | ((uint16_t)calib1[8] << 8));
    g_calib.par_p3 = (int8_t)calib1[9];
    g_calib.par_p4 = (int16_t)((uint16_t)calib1[10] | ((uint16_t)calib1[11] << 8));
    g_calib.par_p5 = (int16_t)((uint16_t)calib1[12] | ((uint16_t)calib1[13] << 8));
    g_calib.par_p6 = (int8_t)calib1[14];
    g_calib.par_p7 = (int8_t)calib1[15];
    g_calib.par_p8 = (int16_t)((uint16_t)calib1[16] | ((uint16_t)calib1[17] << 8));
    g_calib.par_p9 = (int16_t)((uint16_t)calib1[18] | ((uint16_t)calib1[19] << 8));
    g_calib.par_p10 = (int8_t)calib1[20];

    g_calib.par_h1 = (uint16_t)(((uint16_t)calib1[22] << 4) | (calib1[21] & 0x0F));
    g_calib.par_h2 = (uint16_t)(((uint16_t)calib2[0] << 4) | (calib1[21] >> 4));
    g_calib.par_h3 = (int8_t)calib2[1];
    g_calib.par_h4 = (int8_t)calib2[2];
    g_calib.par_h5 = (int8_t)calib2[3];
    g_calib.par_h6 = calib2[4];
    g_calib.par_h7 = (int8_t)calib2[5];

    g_calib.par_g1 = (int8_t)calib2[6];
    g_calib.par_g2 = (int16_t)((uint16_t)calib2[7] | ((uint16_t)calib2[8] << 8));
    g_calib.par_g3 = (int8_t)calib2[9];
    g_calib.par_g4 = calib2[10];
    g_calib.par_g5 = (int8_t)calib2[11];
    g_calib.par_g6 = calib2[12];
    g_calib.par_g7 = (int8_t)calib2[13];
    g_calib.par_g8 = (int8_t)calib2[14];
    g_calib.par_g9 = (int16_t)((uint16_t)calib2[15] | ((uint16_t)calib1[23] << 8));
    g_calib.par_g10 = calib1[24];
    g_calib.par_g11 = (int8_t)calib1[25];

    /* Read additional gas calibration from register 0x30–0x3F */
    uint8_t gas_calib[16];
    if (!bme688_read_burst(0x30, gas_calib, 16)) return false;

    g_calib.par_g12 = (int8_t)gas_calib[0];
    g_calib.par_g13 = (int8_t)gas_calib[1];
    g_calib.par_g14 = (int16_t)((uint16_t)gas_calib[2] | ((uint16_t)gas_calib[3] << 8));
    g_calib.par_g15 = (int16_t)((uint16_t)gas_calib[4] | ((uint16_t)gas_calib[5] << 8));
    g_calib.par_g16 = (int16_t)((uint16_t)gas_calib[6] | ((uint16_t)gas_calib[7] << 8));
    g_calib.par_g17 = (int8_t)gas_calib[8];
    g_calib.par_g18 = (int16_t)((uint16_t)gas_calib[9] | ((uint16_t)gas_calib[10] << 8));
    g_calib.par_g19 = (int16_t)((uint16_t)gas_calib[11] | ((uint16_t)gas_calib[12] << 8));
    g_calib.par_g20 = (int8_t)gas_calib[13];
    g_calib.par_g21 = (int8_t)gas_calib[14];
    g_calib.par_g22 = (int8_t)gas_calib[15];

    /* Read range/error calibration */
    uint8_t range_calib[3];
    if (!bme688_read_burst(0x02, range_calib, 3)) return false;
    g_calib.res_heat_range = (range_calib[0] >> 4) & 0x03;
    g_calib.res_heat_val = (int8_t)range_calib[1];
    g_calib.range_sw_err = (int8_t)range_calib[2];

    g_calib_loaded = true;
    return true;
}

/* ======================================================================== *
 *  Compensation functions (from BME688 datasheet)                              *
 * ======================================================================== */

static int32_t compensate_temperature(int32_t raw_temp) {
    int32_t var1, var2, t;

    var1 = (int32_t)((raw_temp / 8) - ((int32_t)g_calib.par_t1 * 2));
    var1 = (var1 * ((int32_t)g_calib.par_t2)) / 2048;
    var2 = (int32_t)((raw_temp / 16) - ((int32_t)g_calib.par_t1 * 2));
    var2 = (var2 * var2) / 4096;
    var2 = (var2 * (int32_t)g_calib.par_t3) / 1024;
    g_t_fine = var1 + var2;
    t = (g_t_fine * 5 + 128) / 256;
    return t;
}

static uint32_t compensate_pressure(int32_t raw_press) {
    int32_t var1, var2;
    uint32_t p;

    var1 = ((int32_t)g_t_fine) / 2 - 64000;
    var2 = ((var1 / 4) * (var1 / 4)) / 2048;
    var2 = (var2 * (int32_t)g_calib.par_p6) / 4;
    var2 = var2 + (var1 * (int32_t)g_calib.par_p5) * 2;
    var2 = (var2 / 4) + ((int32_t)g_calib.par_p4) * 65536;
    var1 = (((int32_t)g_calib.par_p3 * (var1 / 4) * (var1 / 4)) / 8192) +
           ((int32_t)g_calib.par_p2 * var1) / 2;
    var1 = var1 / 262144;
    var1 = (32768 + var1) * (int32_t)g_calib.par_p1 / 8;

    if (var1 == 0) return 0;

    p = (uint32_t)(((int32_t)1048576 - raw_press) - (var2 / 4096)) * 3125;
    p = p / var1;
    var1 = ((int32_t)g_calib.par_p9 * (int32_t)((p / 8192) * (p / 8192))) / 33554432;
    var2 = ((int32_t)(p / 4) * (int32_t)g_calib.par_p8) / 256;
    p = (uint32_t)((int32_t)p + ((var1 + var2 + g_calib.par_p7) / 16));
    return p;
}

static uint32_t compensate_humidity(int32_t raw_hum) {
    int32_t temp_scaled = g_t_fine / 5;
    int32_t var1 = (int32_t)(raw_hum - ((int32_t)g_calib.par_h1 * 16)) -
                   ((temp_scaled * (int32_t)g_calib.par_h3) / 100);
    int32_t var2 = ((int32_t)g_calib.par_h2 * 262144) / 16384;
    int32_t var3 = var1 * var2 / 16384;
    int32_t var4 = ((int32_t)g_calib.par_h4 * 4) / 16;
    int32_t var5 = ((int32_t)g_calib.par_h5 * 4) / 16;
    int32_t var6 = (temp_scaled * var5) / 100;
    int32_t var7 = var3 + var4 + var6;
    int32_t var8 = var7 / 64;
    int32_t var9 = (int32_t)g_calib.par_h6 * 16;
    int32_t var10 = (temp_scaled * (int32_t)g_calib.par_h7) / 100;
    int32_t var11 = var8 * (var9 + var10) / 1024;
    uint32_t hum = var11 / 16384;

    if (hum > 100000) hum = 100000;
    if (hum < 0) hum = 0;

    return hum;
}

/* ======================================================================== *
 *  Public API                                                                  *
 * ======================================================================== */

bool bme688_init(void) {
    uint8_t chip_id;

    /* Verify chip ID */
    if (!bme688_read_reg(BME688_REG_CHIP_ID, &chip_id)) return false;
    if (chip_id != BME688_CHIP_ID) return false;

    /* Soft reset */
    if (!bme688_write_reg(BME688_REG_RESET, 0xB6)) return false;

    /* Wait for reset */
    for (volatile uint32_t i = 0; i < 50000; i++) { __asm__("nop"); }

    /* Load calibration data */
    if (!bme688_load_calibration()) return false;

    /* Configure humidity oversampling: 16× */
    if (!bme688_write_reg(BME688_REG_CTRL_HUM, BME688_OSRS_16X)) return false;

    /* Configure temperature/pressure oversampling: 16×, normal mode */
    if (!bme688_write_reg(BME688_REG_CTRL_MEAS,
                          (BME688_OSRS_16X << 5) |  /* Temperature */
                          (BME688_OSRS_16X << 2) |  /* Pressure */
                          BME688_MODE_NORMAL)) return false;

    /* Configure IIR filter: 16× */
    if (!bme688_write_reg(BME688_REG_CONFIG,
                          (BME688_FILTER_16 << 2) |  /* Filter */
                          0x00)) return false;        /* SPI 3-wire off */

    return true;
}

bool bme688_read_temperature(float *temp_c) {
    uint8_t data[3];

    if (!bme688_read_burst(0x22, data, 3)) return false;

    int32_t raw = ((int32_t)data[0] << 12) | ((int32_t)data[1] << 4) | (data[2] >> 4);
    int32_t t = compensate_temperature(raw);
    *temp_c = (float)t / 100.0f;

    return true;
}

bool bme688_read_pressure(float *press_pa) {
    uint8_t data[3];

    if (!bme688_read_burst(0x25, data, 3)) return false;

    int32_t raw = ((int32_t)data[0] << 12) | ((int32_t)data[1] << 4) | (data[2] >> 4);
    uint32_t p = compensate_pressure(raw);
    *press_pa = (float)p / 100.0f;

    return true;
}

bool bme688_read_humidity(float *humid_pct) {
    uint8_t data[2];

    if (!bme688_read_burst(0x28, data, 2)) return false;

    int32_t raw = ((int32_t)data[0] << 8) | data[1];
    uint32_t h = compensate_humidity(raw);
    *humid_pct = (float)h / 1000.0f;

    return true;
}

bool bme688_read_all(float *temp_c, float *press_pa, float *humid_pct) {
    uint8_t data[8];

    /* Read all sensor data starting from 0x1D (20 bytes) */
    if (!bme688_read_burst(0x1D, data, 8)) return false;

    /* Temperature (3 bytes at offset 5) */
    int32_t raw_temp = ((int32_t)data[5] << 12) | ((int32_t)data[6] << 4) | (data[7] >> 4);
    int32_t t = compensate_temperature(raw_temp);
    if (temp_c) *temp_c = (float)t / 100.0f;

    /* Pressure (3 bytes at offset 2) */
    int32_t raw_press = ((int32_t)data[2] << 12) | ((int32_t)data[3] << 4) | (data[4] >> 4);
    uint32_t p = compensate_pressure(raw_press);
    if (press_pa) *press_pa = (float)p / 100.0f;

    /* Humidity (2 bytes at offset 0) */
    int32_t raw_hum = ((int32_t)data[0] << 8) | data[1];
    uint32_t h = compensate_humidity(raw_hum);
    if (humid_pct) *humid_pct = (float)h / 1000.0f;

    return true;
}

void bme688_sleep(void) {
    bme688_write_reg(BME688_REG_CTRL_MEAS, BME688_MODE_SLEEP);
}

void bme688_wake(void) {
    bme688_write_reg(BME688_REG_CTRL_MEAS,
                     (BME688_OSRS_16X << 5) |
                     (BME688_OSRS_16X << 2) |
                     BME688_MODE_NORMAL);
}
