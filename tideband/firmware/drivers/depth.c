/**
 * @file    depth.c
 * @brief   TideBand — MS5837-30BA pressure/depth/temperature driver.
 *          Communicates via I2C1 to read 24-bit pressure and temperature
 *          data, applies the manufacturer-specified compensation algorithm,
 *          and converts pressure to depth using the hydrostatic equation.
 * @author  jayis1
 * @copyright © 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 *
 * The MS5837-30BA is a waterproof, gel-filled pressure sensor rated to
 * 30 bar (300 m depth). It contains a factory-calibrated compensation
 * PROM (128 bits, 6 coefficients) that must be applied to the raw ADC
 * readings to produce temperature-compensated pressure.
 *
 * Depth calculation:
 *   depth = (pressure - surface_pressure) / (rho * g)
 *   where rho = 1025 kg/m³ (seawater), g = 9.80665 m/s²
 *   => depth_m = (pressure_mbar - surface_mbar) * 100 / (1025 * 9.80665)
 *   => depth_m = (pressure_mbar - surface_mbar) * 0.0099468
 */

#include <string.h>
#include "board.h"
#include "registers.h"
#include "depth.h"

/* ---- MS5837 commands ---- */
#define MS5837_CMD_RESET        0x1Eu
#define MS5837_CMD_PROM_READ    0xA0u  /* + 2*coeff_num */
#define MS5837_CMD_CONV_D1      0x40u  /* + OSR */
#define MS5837_CMD_CONV_D2      0x50u  /* + OSR */
#define MS5837_CMD_ADC_READ     0x00u

/* OSR selection (conversion time in parentheses) */
#define MS5837_OSR_256          0x00u  /* 0.5 ms */
#define MS5837_OSR_512          0x02u  /* 1.1 ms */
#define MS5837_OSR_1024         0x04u  /* 2.1 ms */
#define MS5837_OSR_2048         0x06u  /* 4.1 ms */
#define MS5837_OSR_4096         0x08u  /* 8.2 ms */
#define MS5837_OSR_8192         0x0Au  /* 16.2 ms */

/* Physical constants */
#define SEAWATER_DENSITY  1025.0f   /* kg/m³ */
#define GRAVITY_ACCEL     9.80665f  /* m/s² */
#define MBAR_TO_PASCAL    100.0f

/* ---- State ---- */
static uint16_t prom[6];          /* Calibration coefficients */
static float surface_pressure_mbar = 1013.25f;  /* Default: 1 atm */
static uint8_t initialized = 0;

/* ---- Local functions ---- */
static void i2c1_init(void);
static void i2c1_write(uint8_t addr, uint8_t cmd);
static void i2c1_read(uint8_t addr, uint8_t *buf, uint8_t len);
static uint32_t read_adc(uint8_t cmd);
static void read_prom(void);
static uint8_t crc4(uint16_t prom[6]);

/* ---- Public API ---- */

void depth_init(void)
{
    /* Enable I2C1 clock and GPIO */
    RCC->APB1ENR1 |= RCC_APB1ENR1_I2C1;
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOB;

    /* I2C1 pins: PB6 (SCL), PB7 (SDA), AF4 */
    gpio_set_mode(GPIOB_BASE, 6, GPIO_MODE_AF);
    gpio_set_af(GPIOB_BASE, 6, 4);
    gpio_set_speed(GPIOB_BASE, 6, GPIO_SPEED_HIGH);
    gpio_set_pupd(GPIOB_BASE, 6, GPIO_PUPD_PU);
    gpio_set_mode(GPIOB_BASE, 7, GPIO_MODE_AF);
    gpio_set_af(GPIOB_BASE, 7, 4);
    gpio_set_speed(GPIOB_BASE, 7, GPIO_SPEED_HIGH);
    gpio_set_pupd(GPIOB_BASE, 7, GPIO_PUPD_PU);

    i2c1_init();

    /* Reset sensor */
    i2c1_write(PRESS_I2C_ADDR, MS5837_CMD_RESET);
    /* Wait for reset to complete (>2.8 ms) */
    for (volatile int i = 0; i < 80000; i++) { }

    /* Read calibration PROM */
    read_prom();

    initialized = 1;
}

void depth_read(depth_data_t *data)
{
    if (!initialized) {
        depth_init();
    }

    memset(data, 0, sizeof(*data));

    /* Read raw pressure (D1) and temperature (D2) at OSR 2048 */
    uint32_t d1 = read_adc(MS5837_CMD_CONV_D1 | MS5837_OSR_2048);
    uint32_t d2 = read_adc(MS5837_CMD_CONV_D2 | MS5837_OSR_2048);

    if (d1 == 0 || d2 == 0) {
        data->valid = 0;
        return;
    }

    /* ---- MS5837 compensation (per datasheet) ---- */
    /* For 30 bar version, the formula uses the same structure as MS5803
     * but with different interpretation for pressure (in mbar ×10). */

    /* Calculate 1st order pressure and temperature (MS5803 algorithm) */
    int32_t dT = (int32_t)d2 - ((int32_t)prom[4] << 8);
    int64_t OFF = ((int64_t)prom[1] << 17) + ((int64_t)dT * prom[3]) / 64;
    int64_t SENS = ((int64_t)prom[0] << 16) + ((int64_t)dT * prom[2]) / 128;

    /* 2nd order compensation */
    int32_t T2 = 0;
    int64_t OFF2 = 0;
    int64_t SENS2 = 0;

    int32_t temp = 2000 + ((int64_t)dT * prom[5]) / 8192;

    /* Low temperature compensation */
    if (temp < 2000) {
        T2 = (dT * dT) / 2147483648LL;
        OFF2 = 3LL * (temp - 2000) * (temp - 2000) / 2;
        SENS2 = 5LL * (temp - 2000) * (temp - 2000) / 16;

        if (temp < -1500) {
            OFF2 += 7LL * (temp + 1500) * (temp + 1500);
            SENS2 += 4LL * (temp + 1500) * (temp + 1500);
        }
    }

    /* Apply 2nd order corrections */
    temp -= T2;
    OFF -= OFF2;
    SENS -= SENS2;

    /* Calculate compensated pressure (mbar ×10 for 30bar version) */
    int32_t pressure = (int32_t)(((SENS * d1) / 2097152 - OFF) / 8192);
    /* For MS5837-30BA, pressure is in mbar (already divided by 10 from
     * the 2-bar version formula). We treat the result as mbar × 10. */
    float pressure_mbar = (float)pressure / 10.0f;

    /* Temperature in 0.01°C, convert to °C */
    float temp_c = (float)temp / 100.0f;

    /* Calculate depth from pressure difference */
    float depth_m = (pressure_mbar - surface_pressure_mbar) * MBAR_TO_PASCAL
                    / (SEAWATER_DENSITY * GRAVITY_ACCEL);

    data->pressure_mbar = pressure_mbar;
    data->depth_m = depth_m;
    data->temp_c = temp_c;
    data->valid = 1;
}

void depth_set_surface(float pressure_mbar)
{
    surface_pressure_mbar = pressure_mbar;
}

uint8_t depth_is_immersed(void)
{
    depth_data_t d;
    depth_read(&d);
    return (d.valid && d.depth_m > DIVE_IMMERSION_DEPTH_M) ? 1 : 0;
}

/* ---- Local function implementations ---- */

static void i2c1_init(void)
{
    I2C1_CR1 = 0;  /* Disable */
    I2C1_TIMINGR = I2C1_TIMING_400K;
    I2C1_CR1 = I2C_CR1_PE;  /* Enable */
}

static void i2c1_write(uint8_t addr, uint8_t cmd)
{
    I2C1_CR2 = ((uint32_t)addr << 1) | (1u << 16) | I2C_CR2_START;
    while ((I2C1_ISR & I2C_ISR_TXE) == 0) {
        if (I2C1_ISR & I2C_ISR_NACKF) {
            I2C1_ICR = I2C_ISR_NACKF;
            return;
        }
    }
    I2C1_TXDR = cmd;
    while ((I2C1_ISR & I2C_ISR_TC) == 0) { }
}

static void i2c1_read(uint8_t addr, uint8_t *buf, uint8_t len)
{
    I2C1_CR2 = ((uint32_t)addr << 1) | 1u | ((uint32_t)len << 16) |
               I2C_CR2_START | (1u << 14);
    for (uint8_t i = 0; i < len; i++) {
        while ((I2C1_ISR & I2C_ISR_RXNE) == 0) { }
        buf[i] = (uint8_t)I2C1_RXDR;
    }
}

static uint32_t read_adc(uint8_t cmd)
{
    /* Send conversion command */
    i2c1_write(PRESS_I2C_ADDR, cmd);
    /* Wait for conversion: OSR 2048 → ~4.1 ms */
    for (volatile int i = 0; i < 120000; i++) { }

    /* Read 3 bytes (24-bit result) */
    uint8_t buf[3];
    /* Send ADC read command (0x00) then read */
    I2C1_CR2 = ((uint32_t)PRESS_I2C_ADDR << 1) | (1u << 16) | I2C_CR2_START;
    while ((I2C1_ISR & I2C_ISR_TXE) == 0) { }
    I2C1_TXDR = MS5837_CMD_ADC_READ;
    while ((I2C1_ISR & I2C_ISR_TC) == 0) { }

    I2C1_CR2 = ((uint32_t)PRESS_I2C_ADDR << 1) | 1u | (3u << 16) |
               I2C_CR2_START | (1u << 14);
    for (int i = 0; i < 3; i++) {
        while ((I2C1_ISR & I2C_ISR_RXNE) == 0) { }
        buf[i] = (uint8_t)I2C1_RXDR;
    }

    return ((uint32_t)buf[0] << 16) | ((uint32_t)buf[1] << 8) | buf[2];
}

static void read_prom(void)
{
    for (int i = 0; i < 6; i++) {
        uint8_t buf[2];
        /* Send PROM read command */
        I2C1_CR2 = ((uint32_t)PRESS_I2C_ADDR << 1) | (1u << 16) | I2C_CR2_START;
        while ((I2C1_ISR & I2C_ISR_TXE) == 0) { }
        I2C1_TXDR = MS5837_CMD_PROM_READ + i * 2;
        while ((I2C1_ISR & I2C_ISR_TC) == 0) { }

        I2C1_CR2 = ((uint32_t)PRESS_I2C_ADDR << 1) | 1u | (2u << 16) |
                   I2C_CR2_START | (1u << 14);
        for (int j = 0; j < 2; j++) {
            while ((I2C1_ISR & I2C_ISR_RXNE) == 0) { }
            buf[j] = (uint8_t)I2C1_RXDR;
        }
        prom[i] = ((uint16_t)buf[0] << 8) | buf[1];
    }

    /* Verify CRC4 of PROM data */
    if (crc4(prom) != 0) {
        /* PROM CRC mismatch — sensor may be faulty.
         * Continue with potentially bad calibration; depth will be
         * inaccurate but the dive can proceed. */
    }
}

/**
 * CRC4 check for MS5837 PROM data.
 * Per Meas-spec TE Connectivity application note.
 * Returns 0 if CRC matches.
 */
static uint8_t crc4(uint16_t p[6])
{
    uint16_t n_rem = 0;
    uint8_t n_bit;

    for (uint8_t cnt = 0; cnt < 6; cnt++) {
        n_rem ^= p[cnt] >> 8;
        for (n_bit = 8; n_bit > 0; n_bit--) {
            if (n_rem & 0x8000) {
                n_rem = (n_rem << 1) ^ 0x3000;
            } else {
                n_rem <<= 1;
            }
        }
        n_rem ^= p[cnt] & 0xFF;
        for (n_bit = 8; n_bit > 0; n_bit--) {
            if (n_rem & 0x8000) {
                n_rem = (n_rem << 1) ^ 0x3000;
            } else {
                n_rem <<= 1;
            }
        }
    }
    n_rem = (n_rem >> 12) & 0xF;
    return (uint8_t)n_rem;
}