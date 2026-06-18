/*
 * depth.c — HydroFluor MS5837-02BA depth/pressure sensor (I²C)
 * Author: jayis1
 * License: MIT
 *
 * Reads the MS5837-02BA 30 bar waterproof pressure sensor over I²C at
 * address 0x76. Performs the 6-coefficient factory calibration math
 * per the TE Connectivity DS to produce temperature-compensated pressure
 * in mbar and depth in cm (assuming freshwater; salinity correction is
 * applied in software if needed by the app).
 */
#include "depth.h"
#include "../registers.h"

/* MS5837 commands */
#define MS5837_CMD_RESET          0x1E
#define MS5837_CMD_CONV_D1_4096   0x48   /* OSR=4096, max resolution */
#define MS5837_CMD_CONV_D2_4096   0x58
#define MS5837_CMD_READ_ADC      0x00
#define MS5837_CMD_READ_PROM     0xA0   /* + 2*offset */

static uint16_t g_c[7];   /* calibration coefficients C1-C6 */
static int32_t  g_last_mbar = 0;
static int32_t  g_last_t01  = 0;   /* 0.01 °C */

static void i2c_init(void)
{
    RCC->APB1ENR1 |= (1U << 21);  /* I2C1 */
    RCC->AHB2ENR  |= (1U << 1);  /* GPIOB */
    gpio_mode(GPIOB, I2C1_SCL_PIN, GPIO_MODE_AF); gpio_af(GPIOB, I2C1_SCL_PIN, 4);
    gpio_mode(GPIOB, I2C1_SDA_PIN, GPIO_MODE_AF); gpio_af(GPIOB, I2C1_SDA_PIN, 4);
    gpio_pupd(GPIOB, I2C1_SCL_PIN, GPIO_PUPD_PULLUP);
    gpio_pupd(GPIOB, I2C1_SDA_PIN, GPIO_PUPD_PULLUP);
    /* I2C1 TIMINGR for 100 kHz, 8 MHz PCLK source (simplified) */
    I2C1->TIMINGR = 0x00201D2B;   /* 100 kHz @ 80 MHz PCLK */
    I2C1->CR1 = I2C_CR1_PE;
}

static int i2c_write(uint8_t addr7, uint8_t cmd)
{
    I2C1->CR2 = ((uint32_t)addr7 << 1) | I2C_CR2_NBYTES(1) | I2C_CR2_AUTOEND;
    I2C1->CR2 |= I2C_CR2_START;
    while (!(I2C1->ISR & I2C_ISR_TXE) && !(I2C1->ISR & I2C_ISR_NACKF)) { }
    if (I2C1->ISR & I2C_ISR_NACKF) { I2C1->ICR = 1U << 16; return -1; }
    I2C1->TXDR = cmd;
    while (!(I2C1->ISR & I2C_ISR_TC)) { }
    return 0;
}

static int i2c_read3(uint8_t addr7, uint8_t *buf3)
{
    I2C1->CR2 = ((uint32_t)addr7 << 1) | I2C_CR2_NBYTES(3)
              | I2C_CR2_AUTOEND | I2C_CR2_RD_WRN;
    I2C1->CR2 |= I2C_CR2_START;
    for (int i = 0; i < 3; ++i) {
        while (!(I2C1->ISR & I2C_ISR_RXNE)) { }
        buf3[i] = (uint8_t)I2C1->RXDR;
    }
    return 0;
}

static uint16_t i2c_read16(uint8_t addr7, uint8_t cmd)
{
    if (i2c_write(addr7, cmd) < 0) return 0;
    uint8_t b[3];
    i2c_read3(addr7, b);
    return ((uint16_t)b[0] << 8) | b[1];
}

static uint32_t i2c_read24(uint8_t addr7, uint8_t cmd)
{
    if (i2c_write(addr7, cmd) < 0) return 0;
    uint8_t b[3];
    i2c_read3(addr7, b);
    return ((uint32_t)b[0] << 16) | ((uint32_t)b[1] << 8) | b[2];
}

void depth_init(void)
{
    i2c_init();
    /* Reset sensor */
    i2c_write(DEPTH_I2C_ADDR, MS5837_CMD_RESET);
    for (volatile int i = 0; i < 20000; ++i) { }
    /* Read 6 PROM calibration coefficients */
    for (int i = 1; i <= 6; ++i) {
        g_c[i] = i2c_read16(DEPTH_I2C_ADDR, MS5837_CMD_READ_PROM + (i << 1));
    }
}

static void depth_convert(void)
{
    /* Read D1 (pressure) and D2 (temperature) at OSR=4096 */
    i2c_write(DEPTH_I2C_ADDR, MS5837_CMD_CONV_D1_4096);
    for (volatile int i = 0; i < 100000; ++i) { }   /* ~9 ms conversion */
    uint32_t D1 = i2c_read24(DEPTH_I2C_ADDR, MS5837_CMD_READ_ADC);
    i2c_write(DEPTH_I2C_ADDR, MS5837_CMD_CONV_D2_4096);
    for (volatile int i = 0; i < 100000; ++i) { }
    uint32_t D2 = i2c_read24(DEPTH_I2C_ADDR, MS5837_CMD_READ_ADC);

    /* Per MS5837 datasheet conversion math (second-order corrected). */
    int32_t dT = (int32_t)D2 - ((int32_t)g_c[5] << 8);
    int64_t OFF  = ((int64_t)g_c[2] << 16) + ((int64_t)dT * g_c[4]) / 128;
    int64_t SENS = ((int64_t)g_c[1] << 15) + ((int64_t)dT * g_c[3]) / 256;
    int32_t T = 2000 + ((int32_t)dT * g_c[6]) / 8192;

    /* Second-order temperature compensation */
    int64_t OFF2 = 0, SENS2 = 0;
    if (T < 2000) {
        int32_t Ti = (11 * (int32_t)dT * (int32_t)dT) >> 35;
        OFF2  = (int64_t)(3 * (T - 2000) * (T - 2000)) >> 3;
        SENS2 = (int64_t)(5 * (T - 2000) * (T - 2000)) >> 3;
        if (T < -1500) {
            OFF2  += (int64_t)7 * (T + 1500) * (T + 1500);
            SENS2 += (int64_t)(4 * (T + 1500) * (T + 1500)) + 2;
        }
        T    -= Ti;
        OFF  -= OFF2;
        SENS -= SENS2;
    }

    int32_t P = (int32_t)((((int64_t)D1 * SENS) >> 21) - OFF) >> 13;
    /* P is in mbar (1 mbar = 100 Pa). Convert to depth in cm assuming
     * freshwater (ρ=1000 kg/m³, g=9.81 m/s²): 1 mbar ≈ 1.0197 cm H2O. */
    g_last_mbar = P;
    g_last_t01  = T;   /* in 0.01 °C */
}

uint32_t depth_read_cm(void)
{
    depth_convert();
    /* Subtract atmospheric pressure (~1013 mbar) → gauge → depth cm */
    int32_t gauge_mbar = g_last_mbar - 1013;
    if (gauge_mbar < 0) gauge_mbar = 0;
    return (uint32_t)(gauge_mbar * 10197 / 1000);   /* mbar→cmH2O */
}

int32_t depth_read_mbar(void)
{
    depth_convert();
    return g_last_mbar;
}

float depth_temperature_c(void)
{
    return g_last_t01 / 100.0f;
}