/*
 * drivers/skyir.c — MLX90632 sky infrared radiometer driver
 *
 * The MLX90632 is an SMD thermopile with factory calibration stored in
 * its EEPROM.  It reports raw thermopile and die-temperature ADC counts
 * over I²C; the host must apply the calibration constants to compute
 * the object (sky) temperature.  This driver reads the raw values,
 * applies the Melexis published algorithm (simplified, fixed-point),
 * and returns the sky radiating temperature in units of 0.01 °C.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#include "skyir.h"
#include "../board.h"

/* MLX90632 register map */
#define MLX_REG_STATUS    0x8000
#define MLX_REG_CTRL      0x8001
#define MLX_REG_RAW1      0x8009   /* thermopile raw (channel 1) */
#define MLX_REG_RAW2      0x800A   /* die temp raw (channel 2) */
#define MLX_REG_TMC       0x800C   /* measurement count */
#define MLX_REG_EE_K      0x24C1   /* EE start address for cal constants */
#define MLX_REG_EE_TA0    0x24C4
#define MLX_REG_EE_Ta25   0x24C6
#define MLX_REG_EE_Ka     0x24C8

/* Sleep mode: write 0x0006 to CTRL to enter sleep, 0x0007 to wake */
#define MLX_CTRL_SLEEP    0x0006
#define MLX_CTRL_WAKE     0x0007
#define MLX_CTRL_SOBM     0x01     /* start-of-measurement bit */

#define MLX_STATUS_DRDY   0x0008   /* data-ready bit in STATUS */

/* ------------------------------------------------------------------ */
/*  I²C primitive: write 16-bit register address, read N bytes         */
/* ------------------------------------------------------------------ */
static int mlx_read16(uint8_t dev, uint16_t reg, uint16_t *out)
{
    I2C_TypeDef *i2c = I2C1;
    uint32_t to;

    /* Wait if bus busy */
    to = 0x10000;
    while ((i2c->ISR & I2C_ISR_BUSY) && to--) ;

    /* Configure transfer: 2 bytes write (reg addr), no autoend */
    i2c->CR2 = ((uint32_t)dev << 1) | (2u << 16) | I2C_CR2_START;
    to = 0x10000;
    while (!(i2c->ISR & I2C_ISR_TXE) && to--) ;
    if (!(i2c->ISR & I2C_ISR_NACKF)) {
        i2c->TXDR = (reg >> 8) & 0xFF;
        to = 0x10000;
        while (!(i2c->ISR & I2C_ISR_TXE) && to--) ;
        i2c->TXDR = reg & 0xFF;
    }
    to = 0x10000;
    while (!(i2c->ISR & I2C_ISR_TC) && to--) ;

    /* Repeated start for read of 2 bytes, autoend */
    i2c->CR2 = ((uint32_t)dev << 1) | (2u << 16) | I2C_CR2_START |
               I2C_CR2_RD_WRN | I2C_CR2_AUTOEND;
    to = 0x10000;
    while (!(i2c->ISR & I2C_ISR_RXNE) && to--) ;
    uint8_t lo = i2c->RXDR & 0xFF;
    to = 0x10000;
    while (!(i2c->ISR & I2C_ISR_RXNE) && to--) ;
    uint8_t hi = i2c->RXDR & 0xFF;
    to = 0x10000;
    while (!(i2c->ISR & I2C_ISR_STOPF) && to--) ;
    i2c->ICR = 0x20;  /* clear STOPF via ICR */

    *out = ((uint16_t)hi << 8) | lo;
    return (to == 0) ? -1 : 0;
}

static int mlx_write16(uint8_t dev, uint16_t reg, uint16_t val)
{
    I2C_TypeDef *i2c = I2C1;
    uint32_t to;

    i2c->CR2 = ((uint32_t)dev << 1) | (4u << 16) | I2C_CR2_START |
               I2C_CR2_AUTOEND;
    to = 0x10000;
    while (!(i2c->ISR & I2C_ISR_TXE) && to--) ;
    i2c->TXDR = (reg >> 8) & 0xFF;
    to = 0x10000;
    while (!(i2c->ISR & I2C_ISR_TXE) && to--) ;
    i2c->TXDR = reg & 0xFF;
    to = 0x10000;
    while (!(i2c->ISR & I2C_ISR_TXE) && to--) ;
    i2c->TXDR = (val >> 8) & 0xFF;
    to = 0x10000;
    while (!(i2c->ISR & I2C_ISR_TXE) && to--) ;
    i2c->TXDR = val & 0xFF;
    to = 0x10000;
    while (!(i2c->ISR & I2C_ISR_STOPF) && to--) ;
    i2c->ICR = 0x20;
    return (to == 0) ? -1 : 0;
}

/* ------------------------------------------------------------------ */
/*  Wake the sensor from sleep                                         */
/* ------------------------------------------------------------------ */
int skyir_wake(void)
{
    int rc = mlx_write16(I2C_ADDR_MLX90632, MLX_REG_CTRL, MLX_CTRL_WAKE);
    delay_ms(5);   /* wake settling */
    return rc;
}

int skyir_sleep(void)
{
    return mlx_write16(I2C_ADDR_MLX90632, MLX_REG_CTRL, MLX_CTRL_SLEEP);
}

/* ------------------------------------------------------------------ */
/*  Trigger a single measurement and wait for DRDY                     */
/* ------------------------------------------------------------------ */
static int skyir_trigger_and_wait(uint16_t *raw1, uint16_t *raw2,
                                  uint16_t *tmc)
{
    uint16_t ctrl = 0;
    if (mlx_read16(I2C_ADDR_MLX90632, MLX_REG_CTRL, &ctrl) < 0)
        return -1;
    ctrl |= MLX_CTRL_SOBM;
    if (mlx_write16(I2C_ADDR_MLX90632, MLX_REG_CTRL, ctrl) < 0)
        return -1;

    /* Wait for data-ready (typ. 35 ms in single-shot mode) */
    uint32_t to = 200;  /* up to 200 ms */
    uint16_t status = 0;
    while (to--) {
        delay_ms(1);
        if (mlx_read16(I2C_ADDR_MLX90632, MLX_REG_STATUS, &status) < 0)
            return -1;
        if (status & MLX_STATUS_DRDY) break;
    }
    if (!(status & MLX_STATUS_DRDY)) return -1;

    if (mlx_read16(I2C_ADDR_MLX90632, MLX_REG_RAW1, raw1) < 0) return -1;
    if (mlx_read16(I2C_ADDR_MLX90632, MLX_REG_RAW2, raw2) < 0) return -1;
    if (mlx_read16(I2C_ADDR_MLX90632, MLX_REG_TMC,   tmc)   < 0) return -1;
    return 0;
}

/* ------------------------------------------------------------------ */
/*  Convert raw counts to temperature using Melexis algorithm          */
/*  (simplified fixed-point, Q16.16 internally, output in 0.01 °C)     */
/* ------------------------------------------------------------------ */
static int32_t compute_object_temp(int32_t raw1, int32_t raw2,
                                   int32_t K, int32_t Ta0, int32_t Ta25,
                                   int32_t Ka)
{
    /* Die temperature: Tdie = Ta25 + (raw2 - Ta0) / Ka   (°C, scaled) */
    int32_t tdie_cx100 = Ta25 + ((raw2 - Ta0) * 100) / (Ka ? Ka : 1);

    /* Object temperature (simplified, single-band calibration):
     *   Tobj = Tdie + K * (raw1 - raw1_at_Tdie)
     * We approximate raw1_at_Tdie as 0 (factory zeroed) and scale.
     * Full Melexis algorithm uses dual-band EE constants; this is
     * the single-band simplification adequate for sky temperature
     * where the object emissivity is ~0.98 (clear sky effective).
     */
    int32_t tobj_cx100 = tdie_cx100 + (K * raw1) / 256;

    /* Clamp to plausible sky range: -80.00 to +60.00 °C */
    if (tobj_cx100 < -8000) tobj_cx100 = -8000;
    if (tobj_cx100 > 6000)  tobj_cx100 = 6000;
    return tobj_cx100;
}

/* ------------------------------------------------------------------ */
/*  Public API: read sky temperature                                   */
/*  Returns temperature in units of 0.01 °C (e.g. -2350 = -23.50 °C)  */
/* ------------------------------------------------------------------ */
int skyir_read_sky_temp_cx100(int32_t *sky_temp_cx100)
{
    uint16_t raw1 = 0, raw2 = 0, tmc = 0;

    if (skyir_wake() < 0) return -1;
    if (skyir_trigger_and_wait(&raw1, &raw2, &tmc) < 0) {
        skyir_sleep();
        return -2;
    }

    /* Read calibration constants from EEPROM (cached at first call) */
    static int32_t s_K = 0, s_Ta0 = 0, s_Ta25 = 0, s_Ka = 0;
    static uint8_t s_cal_loaded = 0;
    if (!s_cal_loaded) {
        uint16_t k = 0, ta0 = 0, ta25 = 0, ka = 0;
        if (mlx_read16(I2C_ADDR_MLX90632, MLX_REG_EE_K,   &k)   < 0 ||
            mlx_read16(I2C_ADDR_MLX90632, MLX_REG_EE_TA0, &ta0) < 0 ||
            mlx_read16(I2C_ADDR_MLX90632, MLX_REG_EE_Ta25,&ta25)< 0 ||
            mlx_read16(I2C_ADDR_MLX90632, MLX_REG_EE_Ka,  &ka)  < 0) {
            skyir_sleep();
            return -3;
        }
        /* Sign-extend 16-bit signed EEPROM values to 32-bit */
        s_K   = (int32_t)(int16_t)k;
        s_Ta0 = (int32_t)(int16_t)ta0;
        s_Ta25= (int32_t)(int16_t)ta25;
        s_Ka  = (int32_t)(int16_t)ka;
        s_cal_loaded = 1;
    }

    *sky_temp_cx100 = compute_object_temp((int16_t)raw1, (int16_t)raw2,
                                          s_K, s_Ta0, s_Ta25, s_Ka);
    skyir_sleep();
    return 0;
}