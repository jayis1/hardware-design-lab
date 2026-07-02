/*
 * temp.c — TMP117 temperature sensor driver and compensation helpers
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 *
 * Reads the TMP117 (I2C, address 0x48) for board temperature, used to
 * compensate the magnetoelastic mu_eff (Curie-Weiss), the cable-frequency
 * thermal expansion correction (applied in accel.c), and the AE propagation
 * velocity (not yet applied in v1). The MCU internal sensor is read as a
 * sanity check / fallback.
 */
#include "tensilguard.h"
#include "board.h"
#include "registers.h"
#include "hal.h"

#define TMP117_ADDR        0x48
#define TMP117_REG_TEMP    0x00
#define TMP117_REG_CFGR    0x01
#define TMP117_REG_EEPR    0x04

static float s_last_temp_c = 25.0f;
static float s_last_mcu_c  = 25.0f;

/* ----------------------------- Prototypes --------------------------------- */
static int16_t i2c_read_temp_reg(void);
static float   adc_read_mcu_temp_c(void);

/* ----------------------------- Public API --------------------------------- */

bool temp_init(void)
{
    /* configure TMP117: 1 sample / 1 Hz, no averaging, normal mode */
    uint16_t cfg = i2c_read16(TMP117_ADDR, TMP117_REG_CFGR);
    /* bits [10:9] = conversion cycle: 0000 = 15.5 ms; we want 1 Hz → 0x40
     * (1 second). Set MOD=00 (continuous), AVG=00 (no avg). */
    cfg = (cfg & 0x0380) | 0x0020;  /* 1 Hz, continuous */
    i2c_write16(TMP117_ADDR, TMP117_REG_CFGR, cfg);
    return true;
}

float temp_read_c(void)
{
    int16_t raw = i2c_read_temp_reg();
    if (raw == 0x8000) {
        /* sensor not present — fall back to MCU internal */
        s_last_temp_c = adc_read_mcu_temp_c();
        return s_last_temp_c;
    }
    /* TMP117 resolution: 0.0078125 °C/LSB, two's-complement */
    s_last_temp_c = (float)raw * 0.0078125f;
    s_last_mcu_c = adc_read_mcu_temp_c();
    return s_last_temp_c;
}

float temp_last_c(void) { return s_last_temp_c; }
float temp_mcu_c(void)  { return s_last_mcu_c; }

/*
 * temp_compensate_mu_eff — apply the per-node temperature coefficient.
 * mu_eff(T) = mu_eff_meas * (1 - tempcoef_ppm * (T - T_cal))
 */
float temp_compensate_mu_eff(float mu_eff_raw, float t_c, const cal_page_t *cal)
{
    float dT = t_c - 20.0f;          /* reference temperature */
    float comp = 1.0f - cal->tempcoef_ppm * 1e-6f * dT;
    if (comp < 0.5f) comp = 0.5f;    /* safety clamp */
    return mu_eff_raw * comp;
}

/*
 * temp_compensate_f1 — correct f1 for thermal expansion of the free length.
 * L(T) = L0 * (1 + alpha * (T - 20)), and f1 ~ 1/L so f1(T) = f1_meas / (1+...)
 */
float temp_compensate_f1(float f1_meas, float t_c, float L0_m)
{
    (void)L0_m;
    float alpha = 12e-6f;            /* steel */
    float dT = t_c - 20.0f;
    return f1_meas / (1.0f + alpha * dT);
}

/* ----------------------------- Internals ---------------------------------- */

static int16_t i2c_read_temp_reg(void)
{
    uint16_t v = i2c_read16(TMP117_ADDR, TMP117_REG_TEMP);
    if (v == 0xFFFF) return (int16_t)0x8000;
    /* TMP117 returns 16-bit two's complement in bits [15:0], already scaled */
    return (int16_t)v;
}

/* i2c_read16 / i2c_write16 are defined in temp.c as HAL stubs; other modules
 * (power.c) provide their own definitions, so these are non-static and
 * shared across the build. */

uint16_t i2c_read16(uint8_t addr, uint8_t reg)
{
    (void)addr; (void)reg;
    /* real firmware: I2C1 master write reg, then read 2 bytes */
    return 0xFFFF;
}

void i2c_write16(uint8_t addr, uint8_t reg, uint16_t val)
{
    (void)addr; (void)reg; (void)val;
}

static float adc_read_mcu_temp_c(void)
{
    /* STM32G4 internal temperature sensor: VTS at 30°C ≈ 0.76 V, slope
     * ≈ 2.5 mV/°C. Read via VREFINT/TS_CAL from ADC channel 12. */
    return s_last_mcu_c;   /* placeholder */
}