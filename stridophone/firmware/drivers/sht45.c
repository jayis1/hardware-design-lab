/*
 * sht45.c — SHT45 I2C driver.
 *
 * Author : jayis1
 * License: MIT
 *
 * Single-shot measurement: command 0xFD (clock-stretching) or 0x2400
 * (no-stretch). We use 0xFD and read 6 bytes: T_ticks(2)+CRC, RH_ticks(2)+CRC.
 * Conversion formulas (datasheet):
 *   T  = -46.85 + 175.72 * T_ticks / (2^16 - 1)
 *   RH = -6.0    + 125.0  * RH_ticks / (2^16 - 1)
 */
#include "sht45.h"
#include "../registers.h"

static void i2c_wait_idle(void) {
    while ((I2C4->ISR & (1U<<15)) == 0) { }  /* BUSY */
    while ((I2C4->ISR & (1U<<15)) != 0) { }
}

static int i2c_write(uint8_t addr, const uint8_t *data, int n) {
    I2C4->CR2 = ((uint32_t)addr << 1)
              | ((uint32_t)n << I2C_CR2_NBYTES_Pos)
              | I2C_CR2_START;
    for (int i = 0; i < n; i++) {
        while (!(I2C4->ISR & I2C_ISR_TXE)) {
            if (I2C4->ISR & I2C_ISR_NACKF) { I2C4->ICR = I2C_ISR_NACKF; return -1; }
        }
        I2C4->TXDR = data[i];
    }
    while (!(I2C4->ISR & I2C_ISR_TC)) { }
    I2C4->CR2 |= I2C_CR2_STOP;
    return 0;
}

static int i2c_read(uint8_t addr, uint8_t *buf, int n) {
    I2C4->CR2 = ((uint32_t)addr << 1) | 1U   /* READ */
              | ((uint32_t)n << I2C_CR2_NBYTES_Pos)
              | I2C_CR2_START;
    for (int i = 0; i < n; i++) {
        while (!(I2C4->ISR & I2C_ISR_RXNE)) {
            if (I2C4->ISR & I2C_ISR_NACKF) { I2C4->ICR = I2C_ISR_NACKF; return -1; }
        }
        buf[i] = (uint8_t)I2C4->RXDR;
    }
    while (!(I2C4->ISR & I2C_ISR_TC)) { }
    I2C4->CR2 |= I2C_CR2_STOP;
    return 0;
}

void sht45_init(void) {
    RCC->APB4ENR |= RCC_APB4ENR_I2C4;
    /* Timing for 100 kHz from 120 MHz PCLK: PRESC=5, SCLDEL=12, SDADEL=0,
     * SCLH=142, SCLL=185. Approximate; exact tuning not critical. */
    I2C4->TIMINGR = (5U<<28) | (12U<<20) | (0U<<16) | (142U<<8) | (185U<<0);
    I2C4->CR1 = I2C_CR1_PE;
    i2c_wait_idle();
}

int sht45_read(float *temp_c, float *rh_pct) {
    uint8_t cmd = 0xFD;   /* clock-stretching single shot */
    if (i2c_write(SHT45_ADDR, &cmd, 1) != 0) return -1;
    /* Typical conversion < 4 ms. */
    for (volatile int i = 0; i < 20000; i++) { }
    uint8_t buf[6];
    if (i2c_read(SHT45_ADDR, buf, 6) != 0) return -1;
    /* CRC-8 (polynomial 0x31) is in buf[2] and buf[5]; we skip verify. */
    uint16_t t_ticks  = ((uint16_t)buf[0] << 8) | buf[1];
    uint16_t rh_ticks = ((uint16_t)buf[3] << 8) | buf[4];
    *temp_c  = -46.85f + 175.72f * (float)t_ticks  / 65535.0f;
    *rh_pct  = -6.0f   + 125.0f   * (float)rh_ticks / 65535.0f;
    if (*rh_pct < 0.0f)   *rh_pct = 0.0f;
    if (*rh_pct > 100.0f) *rh_pct = 100.0f;
    return 0;
}