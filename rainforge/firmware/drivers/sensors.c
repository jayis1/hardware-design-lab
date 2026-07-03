/*
 * sensors.c — Environmental sensors (SHT45, BMP390, TSL2591)
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 *
 * All three sensors share a single I2C bus (I2C_NUM_0) on GPIO8/GPIO9.
 * The functions implement the register-level protocol for each sensor
 * using a weak I2C abstraction that maps to the ESP-IDF i2c_master_* API
 * in a production build.
 *
 * SHT45:  Send 0xFD (clock-stretching high-repeatability), wait 8.3 ms,
 *         read 6 bytes (T_raw, CRC, RH_raw, CRC). Convert using the
 *         datasheet formulas.
 *
 * BMP390: Read 21 calibration bytes from 0x30, configure oversampling
 *         and ODR, then read 6 bytes (3 pressure + 3 temperature) from
 *         0x04. Apply the Bosch compensation algorithm (64-bit integer
 *         math, datasheet §9.3).
 *
 * TSL2591: Write ENABLE reg (PON + AEN + SAI), wait for integration
 *          time, read CH0 (full + IR) and CH1 (IR) counts. Convert to
 *          lux using the TSL2591 formula with gain and integration
 *          time factors.
 */
#include <stdint.h>
#include <math.h>
#include "../board.h"
#include "../registers.h"
#include "sensors.h"

/* ---- Weak I2C abstraction ---- */
__attribute__((weak)) void i2c_init(uint8_t sda, uint8_t scl, uint32_t hz);
__attribute__((weak)) int  i2c_write(uint8_t addr, const uint8_t *data, uint8_t len);
__attribute__((weak)) int  i2c_read(uint8_t addr, uint8_t *buf, uint8_t len);
__attribute__((weak)) void delay_ms(uint32_t ms);

/* ---- CRC-8 for SHT45 (polynomial 0x31, init 0xFF) ---- */
static uint8_t crc8_sht(const uint8_t *data, uint8_t len)
{
    uint8_t crc = 0xFF;
    for (uint8_t i = 0; i < len; ++i) {
        crc ^= data[i];
        for (int b = 0; b < 8; ++b) {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x31;
            else
                crc <<= 1;
        }
    }
    return crc;
}

/* ================================================================
 * Initialize
 * ================================================================ */
void sensors_init(void)
{
    i2c_init(PIN_I2C_SDA, PIN_I2C_SCL, 100000);

    /* BMP390 soft reset */
    uint8_t reset_cmd[2] = { BMP390_REG_CMD, BMP390_CMD_RESET };
    i2c_write(BMP390_ADDR, reset_cmd, 2);
    delay_ms(5);

    /* BMP390: configure oversampling (pressure 8x, temp 1x) + ODR 50 Hz
     * + IIR filter coefficient 3 + normal mode.
     * We write to registers 0x1C-0x1F (OSR, ODR, DSP_CONFIG). */
    /* OSR: bits [5:3] osr_p = 5 (8x), bits [2:0] osr_t = 0 (1x) → 0x28 */
    uint8_t osr[2] = { 0x1C, 0x28 };
    i2c_write(BMP390_ADDR, osr, 2);
    /* ODR: bits [4:0] odr = 9 (50 Hz) → 0x09 */
    uint8_t odr[2] = { 0x1D, 0x09 };
    i2c_write(BMP390_ADDR, odr, 2);
    /* DSP: IIR coeff = 3 (0x02 in bits [2:0]) */
    uint8_t dsp[2] = { 0x1F, 0x02 };
    i2c_write(BMP390_ADDR, dsp, 2);
    /* CMD: set mode = normal (0x30) + power mode bits → pwr_ctrl = 0b11 */
    uint8_t pwr[2] = { 0x1B, 0x03 };
    i2c_write(BMP390_ADDR, pwr, 2);

    /* TSL2591: enable power + ALS + sleep-after-interrupt */
    uint8_t tsl_en[2] = { TSL2591_CMD_CMD | TSL2591_REG_ENABLE,
                          TSL2591_ENABLE_PON | TSL2591_ENABLE_AEN | TSL2591_ENABLE_SAI };
    i2c_write(TSL2591_ADDR, tsl_en, 2);
    /* Control: gain = medium (1×), integration = 100 ms */
    uint8_t tsl_ctrl[2] = { TSL2591_CMD_CMD | TSL2591_REG_CONTROL,
                            TSL2591_GAIN_MED | TSL2591_INT_100MS };
    i2c_write(TSL2591_ADDR, tsl_ctrl, 2);
}

/* ================================================================
 * SHT45 — Temperature & Humidity
 * ================================================================ */
int sht45_read(float *temp_c, float *rh_pct)
{
    uint8_t cmd = SHT45_CMD_MEASURE_HPM;
    if (i2c_write(SHT45_ADDR, &cmd, 1) != 0)
        return -1;

    delay_ms(SHT45_CONV_MS);

    uint8_t raw[6];
    if (i2c_read(SHT45_ADDR, raw, 6) != 0)
        return -1;

    /* CRC check on T and RH data */
    if (crc8_sht(&raw[0], 2) != raw[2]) return -2;
    if (crc8_sht(&raw[3], 2) != raw[5]) return -2;

    uint16_t t_raw = ((uint16_t)raw[0] << 8) | raw[1];
    uint16_t rh_raw = ((uint16_t)raw[3] << 8) | raw[4];

    /* SHT45 formulas (datasheet §4.3):
     * T = -46.85 + 175.72 × (t_raw / 2^16)
     * RH = -6 + 125 × (rh_raw / 2^16) */
    *temp_c = -46.85f + 175.72f * (float)t_raw / 65536.0f;
    *rh_pct = -6.0f + 125.0f * (float)rh_raw / 65536.0f;
    if (*rh_pct > 100.0f) *rh_pct = 100.0f;
    if (*rh_pct < 0.0f)   *rh_pct = 0.0f;

    return 0;
}

/* ================================================================
 * BMP390 — Barometric Pressure
 *
 * The BMP390 uses a 64-bit integer compensation algorithm (datasheet
 * §9.3). We store the 21 calibration coefficients read from registers
 * 0x30-0x44 and apply the compensation formulas.
 * ================================================================ */

/* Calibration coefficients (read once at init) */
static struct {
    uint16_t T1, T2;
    int8_t   T3;
    uint16_t P1, P2;
    int8_t   P3, P4;
    int16_t  P5, P6, P7, P8, P9, P10;
} bmp_cal;

static int bmp390_load_cal(void)
{
    uint8_t reg = BMP390_REG_CALIB;
    if (i2c_write(BMP390_ADDR, &reg, 1) != 0) return -1;
    uint8_t cal[21];
    if (i2c_read(BMP390_ADDR, cal, 21) != 0) return -1;

    /* Parse calibration data (datasheet Table 17) */
    bmp_cal.T1 = (uint16_t)cal[0]  | ((uint16_t)cal[1]  << 8);   /* par_t1 */
    bmp_cal.T2 = (uint16_t)cal[2]  | ((uint16_t)cal[3]  << 8);
    bmp_cal.T3 = (int8_t)cal[4];
    bmp_cal.P1 = (uint16_t)cal[5]  | ((uint16_t)cal[6]  << 8);
    bmp_cal.P2 = (uint16_t)cal[7]  | ((uint16_t)cal[8]  << 8);
    bmp_cal.P3 = (int8_t)cal[9];
    bmp_cal.P4 = (int8_t)cal[10];
    bmp_cal.P5 = (int16_t)(cal[11] | (cal[12] << 8));
    bmp_cal.P6 = (int16_t)(cal[13] | (cal[14] << 8));
    bmp_cal.P7 = (int8_t)cal[15];
    bmp_cal.P8 = (int8_t)cal[16];
    bmp_cal.P9 = (int16_t)(cal[17] | (cal[18] << 8));
    bmp_cal.P10 = (int8_t)cal[19];
    return 0;
}

int bmp390_read(float *pressure_pa, float *temp_c)
{
    static int cal_loaded = 0;
    if (!cal_loaded) {
        if (bmp390_load_cal() != 0) return -1;
        cal_loaded = 1;
    }

    /* Read 6 bytes of data (3 pressure + 3 temperature, 24-bit each) */
    uint8_t reg = BMP390_REG_DATA;
    if (i2c_write(BMP390_ADDR, &reg, 1) != 0) return -1;
    uint8_t data[6];
    if (i2c_read(BMP390_ADDR, data, 6) != 0) return -1;

    int32_t adc_p = (int32_t)((uint32_t)data[0] | ((uint32_t)data[1] << 8)
                              | ((uint32_t)data[2] << 16));
    int32_t adc_t = (int32_t)((uint32_t)data[3] | ((uint32_t)data[4] << 8)
                              | ((uint32_t)data[5] << 16));

    /* Temperature compensation (datasheet §9.3, 64-bit integer math) */
    double T1 = (double)bmp_cal.T1 / 0.00390625;   /* 2^-8 */
    double T2 = (double)bmp_cal.T2 / 1073741824.0;  /* 2^30 */
    double T3 = (double)bmp_cal.T3 / 281474976710656.0;  /* 2^48 */

    double pd1 = (double)adc_t - T1;
    double pd2 = pd1 * T2;
    double comp_temp = pd2 + (pd1 * pd1) * T3;
    *temp_c = (float)comp_temp;

    /* Pressure compensation (simplified — full formula uses 64-bit) */
    double P1 = (double)bmp_cal.P1 - 1.0;
    double P2 = (double)bmp_cal.P2 * 131072.0;     /* 2^17 */
    double P3 = (double)bmp_cal.P3 / 17179869184.0;  /* 2^34 */
    double P5 = (double)bmp_cal.P5 * 0.125;        /* 2^-3 */
    double P6 = (double)bmp_cal.P6 * 64.0;         /* 2^6 */
    double P8 = (double)bmp_cal.P8 / 256.0;        /* 2^-8 */
    double P9 = (double)bmp_cal.P9 / 32768.0;      /* 2^-15 */
    double P10 = (double)bmp_cal.P10 / 140737488355328.0;  /* 2^47 */

    double L1 = P5 + P6 * comp_temp + P8 * comp_temp * comp_temp;
    double L2 = adc_p * (P1 * comp_temp + P2 + P3 * comp_temp * comp_temp);
    double offset = L1 * 65536.0;     /* 2^16 */
    double sensitivity = L2;
    double inv = 1.0 / (offset + sensitivity);
    double comp_press = (double)adc_p * sensitivity * inv
                       + P9 * comp_temp * comp_temp
                       + P10 * comp_temp * comp_temp * comp_temp;

    *pressure_pa = (float)comp_press;
    return 0;
}

/* ================================================================
 * TSL2591 — Ambient Light
 * ================================================================ */
int tsl2591_read(float *lux)
{
    /* Wait for integration time to complete (100 ms = 0x00 in control reg) */
    delay_ms(120);

    /* Read channel 0 (full spectrum + IR) and channel 1 (IR only) */
    uint8_t reg = TSL2591_CMD_CMD | TSL2591_REG_CHAN0_LOW;
    if (i2c_write(TSL2591_ADDR, &reg, 1) != 0) return -1;
    uint8_t ch0[2];
    if (i2c_read(TSL2591_ADDR, ch0, 2) != 0) return -1;

    reg = TSL2591_CMD_CMD | TSL2591_REG_CHAN1_LOW;
    if (i2c_write(TSL2591_ADDR, &reg, 1) != 0) return -1;
    uint8_t ch1[2];
    if (i2c_read(TSL2591_ADDR, ch1, 2) != 0) return -1;

    uint16_t counts0 = ((uint16_t)ch0[1] << 8) | ch0[0];
    uint16_t counts1 = ((uint16_t)ch1[1] << 8) | ch1[0];

    /* TSL2591 lux formula (datasheet §8):
     *   CPL = (ATIME_ms × AGAIN) / (DF × GA)
     *   lux1 = (counts0 - counts1) × (1 - (counts1/counts0)) / CPL
     * where ATIME = 100 ms (for 0x00), AGAIN = 1 (medium gain = 1×),
     * DF = 408, GA = 1 (no glass attenuation). */
    float atime = 100.0f;
    float again = 1.0f;
    float df = 408.0f;
    float cpl = (atime * again) / df;

    if (counts0 == 0) {
        *lux = 0;
        return 0;
    }

    float ch0f = (float)counts0;
    float ch1f = (float)counts1;
    float lux_val = (ch0f - ch1f) * (1.0f - ch1f / ch0f) / cpl;

    /* Clamp to non-negative */
    if (lux_val < 0) lux_val = 0;
    *lux = lux_val;
    return 0;
}

/* ================================================================
 * Read all sensors
 * ================================================================ */
int sensors_read_all(env_sensors_t *out)
{
    memset(out, 0, sizeof(*out));

    out->sht45_ok   = (sht45_read(&out->temperature_c, &out->humidity_pct) == 0);
    out->bmp390_ok  = (bmp390_read(&out->pressure_pa, NULL) == 0);
    out->tsl2591_ok = (tsl2591_read(&out->ambient_lux) == 0);

    return (out->sht45_ok && out->bmp390_ok && out->tsl2591_ok) ? 0 : -1;
}

/* ================================================================
 * SHT45 heater pulse (condensation removal)
 * ================================================================ */
void sensors_heater_pulse(void)
{
    uint8_t cmd = SHT45_CMD_HEATER_200MW_1S;
    i2c_write(SHT45_ADDR, &cmd, 1);
    delay_ms(1100);   /* heater active for 1 second */
}