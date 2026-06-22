/*
 * sensor_array.c — Multi-sensor sampling, heater profiling, calibration
 * BreathPrint — Portable Breath VOC Analyzer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 *
 * This driver manages all 8 gas sensors across I2C and ADC interfaces.
 * It handles heater profiling for the BME688 sensors, reads from
 * electrochemical and NDIR sensors, and applies calibration.
 */

#include "sensor_array.h"
#include "i2c_drv.h"
#include "adc_drv.h"
#include "../board.h"

/* ========================================================================
 * Sensor channel mapping
 * ========================================================================
 * 0: BME688 #1 — Acetone (I2C1, 0x77)
 * 1: BME688 #2 — Isoprene (I2C1, 0x76)
 * 2: SCD41     — CO2 (I2C1, 0x62)
 * 3: S8        — CH4 (USART3)
 * 4: DGS-EtOH  — Ethanol (I2C1, 0x48)
 * 5: DGS-NH3   — Ammonia (I2C1, 0x49)
 * 6: DGS-H2S   — H2S (I2C1, 0x4A)
 * 7: H2-500    — Hydrogen (ADC1, CH0)
 * ======================================================================== */

/* BME688 heater profile (4 steps, cycled during sampling) */
typedef struct {
    uint16_t temp[4];   /* Heater target temperature (°C) */
    uint16_t duration[4]; /* Duration per step (ms) */
} bme688_profile_t;

/* Acetone-selective heater profile (BME688 #1) */
static const bme688_profile_t profile_acetone = {
    .temp     = { 200, 250, 300, 350 },
    .duration = { 50,  50,  50,  50  }
};

/* Isoprene-selective heater profile (BME688 #2) */
static const bme688_profile_t profile_isoprene = {
    .temp     = { 150, 200, 280, 340 },
    .duration = { 50,  50,  50,  50  }
};

/* Current heater step for each BME688 */
static uint8_t bme688_1_step = 0;
static uint8_t bme688_2_step = 0;

/* Sensor health bitmask */
static uint8_t sensor_health_mask = 0xFF;

/* Last environmental readings for compensation */
static float last_temp = 21.0f;
static float last_humidity = 50.0f;
static float last_pressure = 1013.0f;

/* ========================================================================
 * BME688 Internal Registers
 * ======================================================================== */

#define BME688_REG_CTRL_HUM     0x72
#define BME688_REG_STATUS       0x73
#define BME688_REG_CTRL_MEAS    0x74
#define BME688_REG_CONFIG       0x75
#define BME688_REG_CTRL_GAS     0x71
#define BME688_REG_IDAC_HEAT    0x50
#define BME688_REG_RES_HEAT     0x5A
#define BME688_REG_GAS_WAIT     0x64
#define BME688_REG_CTRL_GAS2    0x72
#define BME688_REG_GAS_R_LSB    0x2B
#define BME688_REG_GAS_R_MSB    0x2A
#define BME688_REG_TEMP_MSB     0x22
#define BME688_REG_HUM_MSB      0x25
#define BME688_REG_PRESS_MSB    0x1F
#define BME688_CHIP_ID          0x61

/* ========================================================================
 * Initialize sensor array
 * ======================================================================== */

void sensor_array_init(void) {
    uint8_t id;
    sensor_health_mask = 0;

    /* --- BME688 #1 (Acetone) --- */
    if (i2c_read_reg(I2C1, BME688_ADDR_1, 0xD0, &id, 1) == 0 && id == BME688_CHIP_ID) {
        bme688_configure(BME688_ADDR_1, &profile_acetone);
        sensor_health_mask |= (1 << 0);
    }

    /* --- BME688 #2 (Isoprene) --- */
    if (i2c_read_reg(I2C1, BME688_ADDR_2, 0xD0, &id, 1) == 0 && id == BME688_CHIP_ID) {
        bme688_configure(BME688_ADDR_2, &profile_isoprene);
        sensor_health_mask |= (1 << 1);
    }

    /* --- SCD41 (CO2) --- */
    if (i2c_write_reg16(I2C1, SCD41_ADDR, SCD41_CMD_REINIT) == 0) {
        delay_ms(20);
        sensor_health_mask |= (1 << 2);
    }

    /* --- DGS Sensors (Ethanol, NH3, H2S) --- */
    /* Spec DGS sensors need I2C init command */
    if (dgs_sensor_init(DGS_ETHANOL_ADDR) == 0) {
        sensor_health_mask |= (1 << 4);
    }
    if (dgs_sensor_init(DGS_NH3_ADDR) == 0) {
        sensor_health_mask |= (1 << 5);
    }
    if (dgs_sensor_init(DGS_H2S_ADDR) == 0) {
        sensor_health_mask |= (1 << 6);
    }

    /* --- H2-500 (ADC) --- */
    /* ADC already initialized in board_init */
    /* Verify it's reading a valid voltage */
    float h2_v = adc_to_voltage(adc_read_channel(ADC1, H2_ADC_CHANNEL));
    if (h2_v > 0.1f && h2_v < 3.3f) {
        sensor_health_mask |= (1 << 7);
    }

    /* --- BMP390 (Pressure/Temp) --- */
    uint8_t bmp_id;
    if (i2c_read_reg(I2C1, BMP390_ADDR, BMP390_REG_CHIP_ID, &bmp_id, 1) == 0
        && bmp_id == 0x60) {
        bmp390_configure();
        sensor_health_mask |= 0x80;  /* Use bit 7 as BMP OK too */
    }
}

/* ========================================================================
 * Configure BME688 sensor with heater profile
 * ======================================================================== */

void bme688_configure(uint8_t addr, const bme688_profile_t *profile) {
    /* Set humidity oversampling ×1 */
    i2c_write_reg(I2C1, addr, BME688_REG_CTRL_HUM, 0x01);

    /* Set temperature, pressure oversampling ×1, and gas measurement */
    i2c_write_reg(I2C1, addr, BME688_REG_CTRL_MEAS, 0x25);  /* T×1 P×1 */
    i2c_write_reg(I2C1, addr, BME688_REG_CONFIG, 0x00);

    /* Configure heater steps */
    for (int i = 0; i < 4; i++) {
        /* Set heater resistance for each step */
        uint8_t res_heat = bme688_calc_res_heat(profile->temp[i]);
        i2c_write_reg(I2C1, addr, BME688_REG_RES_HEAT + i, res_heat);

        /* Set heater duration for each step */
        uint8_t gas_wait = bme688_calc_gas_wait(profile->duration[i]);
        i2c_write_reg(I2C1, addr, BME688_REG_GAS_WAIT + i, gas_wait);
    }

    /* Enable gas heater with 4-step profile */
    i2c_write_reg(I2C1, addr, BME688_REG_CTRL_GAS, 0x0C);  /* HEAT_OFF=0, RUN_GAS=1, NB_CONV=3 */
}

/* ========================================================================
 * Calculate heater resistance from target temperature
 * ======================================================================== */

uint8_t bme688_calc_res_heat(uint16_t temp) {
    /* Simplified calculation — real implementation uses calibration data */
    /* Range: 200-400 °C maps to resistance 0x00 - 0xFF */
    if (temp < 200) temp = 200;
    if (temp > 400) temp = 400;
    return (uint8_t)((temp - 200) * 255 / 200);
}

uint8_t bme688_calc_gas_wait(uint16_t ms) {
    /* Convert milliseconds to gas_wait register format */
    /* Simplified: factor=0 (1 ms multiplier), value = ms / 4 */
    if (ms > 4032) ms = 4032;
    return (uint8_t)(ms / 4);
}

/* ========================================================================
 * Read BME688 gas resistance
 * ======================================================================== */

uint32_t bme688_read_gas(uint8_t addr) {
    uint8_t data[2];

    if (i2c_read_reg(I2C1, addr, BME688_REG_GAS_R_MSB, data, 2) < 0) {
        return 0;
    }

    /* Check if gas measurement is valid */
    uint8_t gas_valid = (data[1] & 0x20) >> 5;
    if (!gas_valid) return 0;

    /* Calculate resistance */
    uint16_t adc_gas = ((uint16_t)(data[0] << 2)) | ((data[1] >> 2) & 0x03);
    uint8_t gas_range = data[1] & 0x0F;

    /* Convert to resistance using BME688 formula */
    /* R = (adc_gas * 2^(gas_range)) / 1000 (kOhms) */
    uint32_t resistance;
    if (adc_gas == 0) {
        resistance = 0;
    } else {
        /* Shift with range, clamp to avoid overflow */
        uint32_t shifted = adc_gas;
        if (gas_range < 16) {
            for (int i = 0; i < gas_range; i++) {
                shifted <<= 1;
                if (shifted > 0x00FFFFFFUL) break;
            }
        }
        resistance = shifted / 1000;  /* kOhms */
    }

    return resistance;
}

/* ========================================================================
 * Read BME688 temperature and humidity
 * ======================================================================== */

void bme688_read_env(uint8_t addr, float *temp, float *humidity) {
    uint8_t data[8];

    /* Read temperature (3 bytes) + pressure (3 bytes) + humidity (2 bytes) */
    if (i2c_read_reg(I2C1, addr, BME688_REG_TEMP_MSB, data, 8) < 0) {
        *temp = 21.0f;
        *humidity = 50.0f;
        return;
    }

    /* Parse temperature (simplified, without full calibration) */
    int32_t adc_temp = ((int32_t)data[0] << 12) | ((int32_t)data[1] << 4) |
                       ((data[2] >> 4) & 0x0F);
    /* Convert to °C (simplified — real uses calibration coefficients) */
    *temp = (float)adc_temp / 65535.0f * 40.0f + 10.0f;

    /* Parse humidity */
    uint16_t adc_hum = ((uint16_t)data[6] << 8) | data[7];
    *humidity = (float)adc_hum / 65535.0f * 100.0f;
    if (*humidity > 100.0f) *humidity = 100.0f;
    if (*humidity < 0.0f) *humidity = 0.0f;
}

/* ========================================================================
 * Read SCD41 CO2, temperature, humidity
 * ======================================================================== */

int scd41_read(uint16_t *co2, float *temp, float *humidity) {
    uint8_t data[9];

    if (i2c_read_cmd16(I2C1, SCD41_ADDR, SCD41_CMD_READ, data, 9) < 0) {
        return -1;
    }

    /* Verify CRC for each 2-byte + CRC group */
    for (int i = 0; i < 3; i++) {
        if (i2c_crc8(&data[i * 3], 2) != data[i * 3 + 2]) {
            return -1;  /* CRC error */
        }
    }

    /* Parse CO2 (ppm) */
    *co2 = ((uint16_t)data[0] << 8) | data[1];

    /* Parse temperature (°C = -45 + 175 * raw / 65535) */
    uint16_t raw_temp = ((uint16_t)data[3] << 8) | data[4];
    *temp = -45.0f + 175.0f * (float)raw_temp / 65535.0f;

    /* Parse humidity (%RH = 100 * raw / 65535) */
    uint16_t raw_hum = ((uint16_t)data[6] << 8) | data[7];
    *humidity = 100.0f * (float)raw_hum / 65535.0f;

    return 0;
}

/* ========================================================================
 * Read SenseAir S8 CH4 via UART (modbus-like protocol)
 * ======================================================================== */

uint16_t s8_read_ch4(void) {
    /* S8 uses a simplified UART protocol at 9600 baud */
    /* Command: 0xFE 0x04 0x00 0x00 0x00 0x01 0x65 0xCC */
    /* Response: 0xFE 0x04 0x02 [MSB] [LSB] [CRC_LO] [CRC_HI] */
    static const uint8_t cmd[] = { 0xFE, 0x04, 0x00, 0x00,
                                    0x00, 0x01, 0x65, 0xCC };

    /* Send command */
    for (int i = 0; i < 8; i++) {
        while (!(USART3->ISR & USART_ISR_TXE));
        USART3->TDR = cmd[i];
    }

    /* Read response (7 bytes, timeout 100 ms) */
    uint8_t resp[7];
    uint32_t start = millis();
    for (int i = 0; i < 7; i++) {
        while (!(USART3->ISR & USART_ISR_RXNE)) {
            if (millis() - start > 100) return 0;
        }
        resp[i] = (uint8_t)USART3->RDR;
    }

    /* Verify header */
    if (resp[0] != 0xFE || resp[1] != 0x04) return 0;

    /* Parse CH4 concentration */
    return ((uint16_t)resp[3] << 8) | resp[4];
}

/* ========================================================================
 * Spec DGS sensor (electrochemical) — I2C read
 * ======================================================================== */

int dgs_sensor_init(uint8_t addr) {
    /* DGS sensors start in standby; send start command */
    uint8_t cmd = 0x01;  /* Start continuous mode */
    return i2c_write(I2C1, addr, &cmd, 1, 1);
}

int dgs_sensor_read(uint8_t addr, uint16_t *gas_ppb, float *temp) {
    uint8_t data[8];

    /* Read 8 bytes: gas_ppb (2), temp (2), and status (4) */
    if (i2c_read(I2C1, addr, data, 8, 1) < 0) return -1;

    /* Parse gas concentration (ppb, big-endian) */
    *gas_ppb = ((uint16_t)data[0] << 8) | data[1];

    /* Parse temperature (×10, big-endian) */
    int16_t raw_temp = ((int16_t)data[2] << 8) | data[3];
    *temp = (float)raw_temp / 10.0f;

    return 0;
}

/* ========================================================================
 * BMP390 pressure sensor read
 * ======================================================================== */

void bmp390_configure(void) {
    /* Set oversampling: pressure ×1, temperature ×1 */
    i2c_write_reg(I2C1, BMP390_ADDR, BMP390_REG_OSR, 0x00);
    /* Set power mode: normal */
    i2c_write_reg(I2C1, BMP390_ADDR, BMP390_REG_CTRL, 0x30);
}

float bmp390_read_pressure(void) {
    uint8_t data[3];
    if (i2c_read_reg(I2C1, BMP390_ADDR, BMP390_REG_DATA, data, 3) < 0) {
        return 1013.0f;
    }
    /* Simplified conversion */
    uint32_t raw = ((uint32_t)data[2] << 16) | ((uint32_t)data[1] << 8) | data[0];
    /* Real implementation uses calibration coefficients */
    return (float)raw / 256.0f / 100.0f;  /* hPa (simplified) */
}

/* ========================================================================
 * Sample all sensors (called at 20 Hz during sampling phase)
 * ======================================================================== */

void sensor_array_sample_all(sensor_raw_t *sample, calib_data_t *calib) {
    /* BME688 #1 — Acetone gas resistance */
    sample->bme688_1_gas = (uint16_t)(bme688_read_gas(BME688_ADDR_1) & 0xFFFF);

    /* BME688 #2 — Isoprene gas resistance */
    sample->bme688_2_gas = (uint16_t)(bme688_read_gas(BME688_ADDR_2) & 0xFFFF);

    /* SCD41 — CO2, temperature, humidity */
    float scd_temp, scd_hum;
    if (scd41_read(&sample->scd41_co2, &scd_temp, &scd_hum) < 0) {
        sample->scd41_co2 = 0;
    }

    /* S8 — CH4 */
    sample->s8_ch4 = s8_read_ch4();

    /* DGS sensors — Ethanol, NH3, H2S */
    float dgs_temp;
    if (dgs_sensor_read(DGS_ETHANOL_ADDR, &sample->dgs_ethanol, &dgs_temp) < 0) {
        sample->dgs_ethanol = 0;
    }
    if (dgs_sensor_read(DGS_NH3_ADDR, &sample->dgs_nh3, &dgs_temp) < 0) {
        sample->dgs_nh3 = 0;
    }
    if (dgs_sensor_read(DGS_H2S_ADDR, &sample->dgs_h2s, &dgs_temp) < 0) {
        sample->dgs_h2s = 0;
    }

    /* H2 sensor — ADC */
    float h2_ppm = adc_read_h2();
    sample->h2_analog = (uint16_t)(h2_ppm * 10.0f);  /* Store as ppm × 10 */

    /* Environmental data */
    float bme_temp, bme_hum;
    bme688_read_env(BME688_ADDR_1, &bme_temp, &bme_hum);
    sample->temperature = (uint16_t)(bme_temp * 100.0f);
    sample->humidity = (uint16_t)(bme_hum * 100.0f);
    sample->pressure = (uint16_t)bmp390_read_pressure();

    last_temp = bme_temp;
    last_humidity = bme_hum;
    last_pressure = (float)sample->pressure;

    /* Apply calibration corrections */
    sensor_array_apply_calib(sample, calib);

    /* Timestamp */
    sample->timestamp_ms = millis();
}

/* ========================================================================
 * Apply calibration corrections to raw sample
 * ======================================================================== */

void sensor_array_apply_calib(sensor_raw_t *sample, calib_data_t *calib) {
    /* Apply zero-offset subtraction and span gain for each channel */
    float values[NUM_SENSORS];
    for (int i = 0; i < NUM_SENSORS; i++) {
        float raw = sensor_array_get_channel(sample, i);
        float corrected = (raw - calib->zero_offset[i]) * calib->span_gain[i];

        /* Temperature compensation */
        float temp_delta = last_temp - 21.0f;  /* Ref temp 21 °C */
        corrected -= calib->temp_coeff[i] * temp_delta;

        /* Humidity compensation */
        float hum_delta = last_humidity - 50.0f;  /* Ref 50% RH */
        corrected -= calib->hum_coeff[i] * hum_delta;

        if (corrected < 0.0f) corrected = 0.0f;
        sensor_array_set_channel(sample, i, (uint16_t)corrected);
    }
}

/* ========================================================================
 * Get/set sensor channel value by index
 * ======================================================================== */

float sensor_array_get_channel(const sensor_raw_t *sample, int channel) {
    switch (channel) {
    case 0: return (float)sample->bme688_1_gas;
    case 1: return (float)sample->bme688_2_gas;
    case 2: return (float)sample->scd41_co2;
    case 3: return (float)sample->s8_ch4;
    case 4: return (float)sample->dgs_ethanol;
    case 5: return (float)sample->dgs_nh3;
    case 6: return (float)sample->dgs_h2s;
    case 7: return (float)sample->h2_analog;
    default: return 0.0f;
    }
}

void sensor_array_set_channel(sensor_raw_t *sample, int channel, uint16_t val) {
    switch (channel) {
    case 0: sample->bme688_1_gas = val; break;
    case 1: sample->bme688_2_gas = val; break;
    case 2: sample->scd41_co2 = val; break;
    case 3: sample->s8_ch4 = val; break;
    case 4: sample->dgs_ethanol = val; break;
    case 5: sample->dgs_nh3 = val; break;
    case 6: sample->dgs_h2s = val; break;
    case 7: sample->h2_analog = val; break;
    default: break;
    }
}

/* ========================================================================
 * Read ambient baseline (quick, no heaters)
 * ======================================================================== */

void sensor_array_read_ambient(sensor_raw_t *sample, calib_data_t *calib) {
    /* Read all sensors without heater profiles for ambient baseline */
    sensor_array_sample_all(sample, calib);
}

/* ========================================================================
 * Feature extraction from sample buffer
 * ========================================================================
 * For each sensor channel, compute:
 *   - Peak amplitude (baseline-corrected)
 *   - Rise time (10% → 90% of peak)
 *   - Plateau (steady-state value at end)
 *   - Decay tau (exponential fit)
 *   - Area under curve (trapezoidal)
 * ========================================================================
 */

void sensor_array_extract_features(const sensor_raw_t *samples,
                                   uint32_t count,
                                   calib_data_t *calib,
                                   feature_vector_t *features) {
    for (int ch = 0; ch < NUM_SENSORS; ch++) {
        sensor_features_t *f = &features->sensors[ch];

        if (count < 2) {
            memset(f, 0, sizeof(*f));
            continue;
        }

        /* Extract raw signal for this channel */
        float signal[SAMPLE_COUNT];
        for (uint32_t i = 0; i < count && i < SAMPLE_COUNT; i++) {
            signal[i] = sensor_array_get_channel(&samples[i], ch);
        }

        /* Find baseline (first 5 samples average) */
        float baseline = 0;
        uint32_t baseline_count = (count < 5) ? count : 5;
        for (uint32_t i = 0; i < baseline_count; i++) {
            baseline += signal[i];
        }
        baseline /= (float)baseline_count;

        /* Find peak */
        float peak_val = 0;
        uint32_t peak_idx = 0;
        for (uint32_t i = 0; i < count; i++) {
            float corrected = signal[i] - baseline;
            if (corrected > peak_val) {
                peak_val = corrected;
                peak_idx = i;
            }
        }
        f->peak = peak_val;

        /* Rise time (10% to 90% of peak) */
        float thresh_10 = peak_val * 0.10f;
        float thresh_90 = peak_val * 0.90f;
        uint32_t t_10 = 0, t_90 = 0;
        for (uint32_t i = 0; i < count && i <= peak_idx; i++) {
            if ((signal[i] - baseline) >= thresh_10 && t_10 == 0) {
                t_10 = i;
            }
            if ((signal[i] - baseline) >= thresh_90 && t_90 == 0) {
                t_90 = i;
                break;
            }
        }
        f->rise_time = (float)(t_90 - t_10) / (float)SAMPLE_RATE_HZ;

        /* Plateau (average of last 10% of samples) */
        uint32_t plateau_start = count - (count / 10);
        if (plateau_start >= count) plateau_start = count - 1;
        float plateau_sum = 0;
        uint32_t plateau_count = count - plateau_start;
        for (uint32_t i = plateau_start; i < count; i++) {
            plateau_sum += signal[i] - baseline;
        }
        f->plateau = plateau_sum / (float)plateau_count;

        /* Decay tau (exponential fit after peak) */
        if (peak_idx < count - 2 && peak_val > 0) {
            /* tau = -dt / ln(V(t) / V_peak) */
            float v_peak = signal[peak_idx] - baseline;
            float v_late = signal[count - 1] - baseline;
            if (v_peak > 0 && v_late > 0 && v_late < v_peak) {
                float dt = (float)(count - 1 - peak_idx) / (float)SAMPLE_RATE_HZ;
                float ratio = v_late / v_peak;
                if (ratio > 0 && ratio < 1) {
                    /* ln using logf approximation: ln(x) ≈ 2 * atanh((x-1)/(x+1)) */
                    float t = (ratio - 1) / (ratio + 1);
                    float ln_ratio = 2 * (t + t*t*t/3 + t*t*t*t*t/5);
                    f->decay_tau = -dt / ln_ratio;
                    if (f->decay_tau < 0) f->decay_tau = 0;
                } else {
                    f->decay_tau = 0;
                }
            } else {
                f->decay_tau = 0;
            }
        } else {
            f->decay_tau = 0;
        }

        /* Area under curve (trapezoidal integration) */
        float auc = 0;
        for (uint32_t i = 1; i < count; i++) {
            float y0 = signal[i-1] - baseline;
            float y1 = signal[i] - baseline;
            auc += (y0 + y1) * 0.5f;
        }
        f->auc = auc / (float)SAMPLE_RATE_HZ;
    }
}

/* ========================================================================
 * Check if all sensors are ready after warmup
 * ======================================================================== */

uint8_t sensor_array_check_ready(void) {
    /* Verify each sensor responds */
    if (sensor_health_mask & (1 << 0)) {
        uint32_t gas = bme688_read_gas(BME688_ADDR_1);
        if (gas == 0) return 0;
    }
    if (sensor_health_mask & (1 << 2)) {
        uint16_t co2;
        float t, h;
        if (scd41_read(&co2, &t, &h) < 0) return 0;
    }
    return 1;
}

/* ========================================================================
 * Heater control
 * ======================================================================== */

void sensor_array_heaters_on(void) {
    /* Enable BME688 heaters by setting RUN_GAS bit */
    i2c_write_reg(I2C1, BME688_ADDR_1, BME688_REG_CTRL_GAS, 0x0C);
    i2c_write_reg(I2C1, BME688_ADDR_2, BME688_REG_CTRL_GAS, 0x0C);
}

void sensor_array_heaters_off(void) {
    /* Disable BME688 heaters */
    i2c_write_reg(I2C1, BME688_ADDR_1, BME688_REG_CTRL_GAS, 0x08);  /* HEAT_OFF=1 */
    i2c_write_reg(I2C1, BME688_ADDR_2, BME688_REG_CTRL_GAS, 0x08);
}

/* ========================================================================
 * Get sensor health bitmask
 * ======================================================================== */

uint8_t sensor_array_health(void) {
    return sensor_health_mask;
}

/* ========================================================================
 * Breath quality validation
 * ========================================================================
 * Uses CO2 level to determine if the breath sample is valid alveolar air.
 * Also checks sensor response amplitudes.
 * ========================================================================
 */

uint8_t breath_validate(const sensor_raw_t *samples, uint32_t count,
                        calib_data_t *calib) {
    if (count < 10) return BREATH_RETRY;

    /* Check CO2 — must exceed 3.5% (35000 ppm) for alveolar breath */
    uint16_t max_co2 = 0;
    for (uint32_t i = 0; i < count; i++) {
        if (samples[i].scd41_co2 > max_co2) max_co2 = samples[i].scd41_co2;
    }

    if (max_co2 < CO2_ALVEOLAR_MIN) {
        return BREATH_DEAD_SPACE;
    }

    /* Check if sensors responded (at least 2 channels should show change) */
    int responding = 0;
    for (int ch = 0; ch < NUM_SENSORS; ch++) {
        float first = sensor_array_get_channel(&samples[0], ch);
        float max_val = first;
        for (uint32_t i = 1; i < count; i++) {
            float v = sensor_array_get_channel(&samples[i], ch);
            if (v > max_val) max_val = v;
        }
        /* Sensor responded if >10% change from baseline */
        if (first > 0 && max_val > first * 1.10f) {
            responding++;
        }
    }

    if (responding < 2) {
        return BREATH_CONTAMINATED;
    }

    return BREATH_VALID;
}

/* ========================================================================
 * Estimate concentrations from features
 * ======================================================================== */

void breath_estimate(const sensor_raw_t *samples, uint32_t count,
                     calib_data_t *calib, feature_vector_t *features,
                     breath_result_t *result) {
    memset(result, 0, sizeof(*result));

    /* Acetone estimation from BME688 #1 features */
    /* Simplified model: ppm = k * peak / baseline (with calibration) */
    float acetone_raw = features->sensors[0].peak;
    if (calib->zero_offset[0] > 0) {
        result->acetone_ppm = (uint16_t)((acetone_raw * calib->span_gain[0]) * 100);
    }

    /* H2 from ADC channel */
    float h2_ppm = (float)samples[count-1].h2_analog / 10.0f;
    result->h2_ppm = (uint16_t)(h2_ppm * 10);

    /* CH4 from S8 NDIR */
    result->ch4_ppm = (uint16_t)(samples[count-1].s8_ch4 * 10);

    /* Ethanol from DGS */
    result->ethanol_ppm = (uint16_t)(samples[count-1].dgs_ethanol * 100);

    /* Isoprene from BME688 #2 */
    float isoprene_raw = features->sensors[1].peak;
    if (calib->zero_offset[1] > 0) {
        result->isoprene_ppb = (uint16_t)(isoprene_raw * calib->span_gain[1]);
    }

    /* NH3 from DGS */
    result->nh3_ppm = (uint16_t)(samples[count-1].dgs_nh3 * 10);

    /* H2S from DGS */
    result->h2s_ppb = samples[count-1].dgs_h2s;

    /* VOC index from BME688 #1 (simplified IAQ) */
    float voc_raw = features->sensors[0].auc;
    result->voc_index = (uint16_t)CLAMP(voc_raw / 100.0f, 0, 500);

    /* CO2 reference (peak CO2 during sample) */
    uint16_t max_co2 = 0;
    for (uint32_t i = 0; i < count; i++) {
        if (samples[i].scd41_co2 > max_co2) max_co2 = samples[i].scd41_co2;
    }
    result->co2_ppm = max_co2;

    /* Environmental */
    result->temperature = (int16_t)(samples[count-1].temperature / 10);
    result->humidity = samples[count-1].humidity / 10;
    result->pressure = samples[count-1].pressure;
}