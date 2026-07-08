/*
 * spectral_drv.c — AS7341 I²C driver and lichen spectral index computation.
 *
 * The AS7341 has 11 spectral channels (F1..F8 = 415..670 nm, plus NIR, clear,
 * and flicker). We read all channels with the integrated LED off (ambient
 * reflectance) and compute lichen-specific indices.
 *
 * Author: jayis1
 * License: MIT
 */

#include "spectral_drv.h"
#include "../board.h"
#include "../registers.h"
#include <string.h>

/* AS7341 register map (subset) */
#define AS7341_REG_ENABLE   0x80  /* power / enable       */
#define AS7341_REG_ATIME    0x81  /* integration time      */
#define AS7341_REG_WTIME    0x83  /* wait time             */
#define AS7341_REG_CONFIG   0xAD
#define AS7341_REG_CH_DATA   0x95  /* first channel data   */
#define AS7341_REG_STATUS    0x93  /* avalid bit            */

#define AS7341_ENABLE_PON   0x01  /* power on              */
#define AS7341_ENABLE_SPEN  0x08  /* spectral measurement   */

static int s_initialized = 0;

/* ------------------------------------------------------------------------ */
/* I²C primitives (blocking)                                                 */
/* ------------------------------------------------------------------------ */
static void i2c_wait_tx(void)
{
    while (!(I2C1->ISR & I2C_ISR_TXIS)) {
        if (I2C1->ISR & I2C_ISR_NACKF) return;
    }
}

static int i2c_write_reg(uint8_t addr, uint8_t reg, uint8_t val)
{
    /* Start a write transaction to the device. */
    I2C1->CR2 = ((uint32_t)addr << 1)
              | I2C_CR2_NBYTES(2)
              | (0 << I2C_CR2_RD_WRN)
              | I2C_CR2_AUTOEND;
    I2C1->CR2 |= I2C_CR2_START;

    /* Send register address. */
    i2c_wait_tx();
    I2C1->TXDR = reg;
    i2c_wait_tx();
    I2C1->TXDR = val;

    /* Wait for auto-stop. */
    while (!(I2C1->ISR & I2C_ISR_STOPF)) { }
    I2C1->ICR = I2C_ISR_STOPF;
    return 0;
}

static int i2c_read_burst(uint8_t addr, uint8_t reg, uint8_t *buf, uint8_t n)
{
    /* Write the register pointer. */
    I2C1->CR2 = ((uint32_t)addr << 1)
              | I2C_CR2_NBYTES(1)
              | (0 << I2C_CR2_RD_WRN);
    I2C1->CR2 |= I2C_CR2_START;
    i2c_wait_tx();
    I2C1->TXDR = reg;

    /* Repeated start for read. */
    I2C1->CR2 = ((uint32_t)addr << 1)
              | I2C_CR2_NBYTES(n)
              | I2C_CR2_RD_WRN
              | I2C_CR2_AUTOEND;
    I2C1->CR2 |= I2C_CR2_START;

    for (uint8_t i = 0; i < n; i++) {
        while (!(I2C1->ISR & I2C_ISR_RXNE)) { }
        buf[i] = (uint8_t)I2C1->RXDR;
    }
    while (!(I2C1->ISR & I2C_ISR_STOPF)) { }
    I2C1->ICR = I2C_ISR_STOPF;
    return 0;
}

/* ------------------------------------------------------------------------ */
/* Public API                                                                 */
/* ------------------------------------------------------------------------ */
int spectral_init(void)
{
    /* Configure I2C1 pins PB8/PB9 as AF4 (I2C). */
    {
        volatile uint32_t *pb = (volatile uint32_t *)GPIOB;
        /* AF4 for PB8 (AFRL nibble 8) and PB9 (AFRH nibble 1) */
        pb[7] = (pb[7] & ~(0xFU << (8*4))) | (0x4U << (8*4));  /* PB8 AFRL */
        pb[8] = (pb[8] & ~(0xFU << (1*4))) | (0x4U << (1*4));  /* PB9 AFRH */
        pb[0] = (pb[0] & ~(0x3U << (8*2))) | (GPIO_MODE_AF << (8*2));
        pb[0] = (pb[0] & ~(0x3U << (9*2))) | (GPIO_MODE_AF << (9*2));
    }

    /* Configure I2C1 timing for 100 kHz from 80 MHz. */
    I2C1->TIMINGR = 0x10909CEC;  /* PRESC=1, SCLL/SCLH for 100 kHz at 80 MHz */
    I2C1->CR1 = I2C_CR1_PE;

    /* Power on the AS7341. */
    i2c_write_reg(AS7341_I2C_ADDR, AS7341_REG_ENABLE, AS7341_ENABLE_PON);
    /* Set integration time: 0x00 → 0.182 ms steps; we use 100 → ~18 ms. */
    i2c_write_reg(AS7341_I2C_ADDR, AS7341_REG_ATIME, 100);
    /* Enable spectral measurement. */
    i2c_write_reg(AS7341_I2C_ADDR, AS7341_REG_ENABLE, AS7341_ENABLE_PON | AS7341_ENABLE_SPEN);

    s_initialized = 1;
    return 0;
}

int spectral_read(spectral_result_t *res)
{
    if (!s_initialized || res == NULL) return -1;
    memset(res, 0, sizeof(*res));

    /* Trigger a new measurement. */
    i2c_write_reg(AS7341_I2C_ADDR, AS7341_REG_ENABLE,
                  AS7341_ENABLE_PON | AS7341_ENABLE_SPEN);

    /* Wait for avalid (poll status register). */
    uint8_t status = 0;
    for (int i = 0; i < 50; i++) {
        i2c_read_burst(AS7341_I2C_ADDR, AS7341_REG_STATUS, &status, 1);
        if (status & 0x01) break;
        /* brief delay */
        for (volatile int d = 0; d < 1000; d++) { }
    }

    /* Read all 11 channel data registers (2 bytes each = 22 bytes). */
    uint8_t raw[22];
    i2c_read_burst(AS7341_I2C_ADDR, AS7341_REG_CH_DATA, raw, 22);
    for (int i = 0; i < 11; i++) {
        res->ch[i] = (uint16_t)raw[2*i] | ((uint16_t)raw[2*i + 1] << 8);
    }

    /* Map AS7341 channels to bands:
     *   F1=415, F2=445, F3=480, F4=515, F5=555, F6=590, F7=630, F8=680,
     *   CLEAR, NIR(855)
     */
    uint16_t b415 = res->ch[0];
    uint16_t b560 = res->ch[4];  /* ~555 nm, call it 560 */
    uint16_t b670 = res->ch[7];  /* ~680 nm, call it 670 */
    uint16_t nir  = res->ch[9];

    /* Lichen NDVI analog. */
    if (nir + b670 > 0) {
        res->lndvi = ((float)nir - (float)b670) / ((float)nir + (float)b670);
    } else {
        res->lndvi = 0.0f;
    }

    /* Chlorophyll index. */
    if (b670 > 0) {
        res->chl_index = (float)b560 / (float)b670;
    } else {
        res->chl_index = 0.0f;
    }

    /* Melanin / UV-screening pigment proxy. */
    if (b560 > 0) {
        res->melanin_proxy = (float)b415 / (float)b560;
    } else {
        res->melanin_proxy = 0.0f;
    }

    return 0;
}