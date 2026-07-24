/*
 * tof_drv.c — VL53L0X time-of-flight distance sensor driver
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Drives the VL53L0X laser time-of-flight sensor via I2C3 to measure
 * probe-to-surface distance (lift-off). This is used for lift-off
 * compensation in the alloy classifier.
 *
 * The VL53L0X communicates over I2C at address 0x29 (7-bit).
 * We use the default address since only one sensor is on the bus.
 */

#include "tof_drv.h"
#include "registers.h"
#include "board.h"

static uint8_t tof_error = 0;
static bool tof_initialized = false;

/* ---- I2C3 low-level helpers ---- */

static void i2c3_delay(void)
{
    for (volatile int i = 0; i < 1000; i++)
        ;
}

static bool i2c3_wait_flag(uint32_t flag, bool set, uint32_t timeout)
{
    uint32_t count = 0;
    while (1) {
        bool is_set = (I2C3->ISR & flag) != 0;
        if (is_set == set)
            return true;
        if (count++ > timeout)
            return false;
        i2c3_delay();
    }
}

static bool i2c3_write_reg(uint8_t reg, uint8_t value)
{
    /* Send start + address + write */
    I2C3->CR2 = (VL53L0X_ADDR << 1) | (2 << I2C_CR2_NBYTES_SHIFT);
    I2C3->CR2 |= I2C_CR2_START;

    /* Wait for TX empty */
    if (!i2c3_wait_flag(I2C_ISR_TXE, true, 10000))
        return false;

    /* Send register address */
    I2C3->TXDR = reg;
    if (!i2c3_wait_flag(I2C_ISR_TXE, true, 10000))
        return false;

    /* Send value */
    I2C3->TXDR = value;

    /* Wait for transfer complete */
    if (!i2c3_wait_flag(I2C_ISR_STOPF, true, 10000))
        return false;

    /* Clear STOPF flag */
    I2C3->ICR = (1UL << 5);

    return true;
}

static bool i2c3_read_reg(uint8_t reg, uint8_t *value)
{
    /* Phase 1: Write register address */
    I2C3->CR2 = (VL53L0X_ADDR << 1) | (1 << I2C_CR2_NBYTES_SHIFT);
    I2C3->CR2 |= I2C_CR2_START;

    if (!i2c3_wait_flag(I2C_ISR_TXE, true, 10000))
        return false;
    I2C3->TXDR = reg;

    if (!i2c3_wait_flag(I2C_ISR_TC, true, 10000))  /* Transfer complete */
        return false;

    /* Phase 2: Read 1 byte */
    I2C3->CR2 = (VL53L0X_ADDR << 1) | (1 << I2C_CR2_NBYTES_SHIFT)
              | I2C_CR2_RD_WRN | I2C_CR2_AUTOEND | I2C_CR2_START;

    if (!i2c3_wait_flag(I2C_ISR_RXNE, true, 10000))
        return false;

    *value = (uint8_t)I2C3->RXDR;

    /* Wait for stop */
    i2c3_wait_flag(I2C_ISR_STOPF, true, 10000);
    I2C3->ICR = (1UL << 5);

    return true;
}

static bool i2c3_read_reg16(uint8_t reg, uint16_t *value)
{
    uint8_t hi, lo;
    if (!i2c3_read_reg(reg, &hi))
        return false;
    if (!i2c3_read_reg(reg + 1, &lo))
        return false;
    *value = ((uint16_t)hi << 8) | lo;
    return true;
}

/* ---- VL53L0X initialization ---- */

bool tof_drv_init(void)
{
    if (tof_initialized)
        return true;

    /* Enable I2C3 and GPIOA clocks */
    RCC_APB1ENR1 |= RCC_APB1ENR1_I2C3;
    RCC_AHB2ENR |= RCC_AHB2ENR_GPIOA;

    /* Configure PA8 (SCL) and PA9 (SDA) as AF4 (I2C3) */
    GPIOA->MODER &= ~(3UL << (I2C3_SCL_PIN * 2));
    GPIOA->MODER |= (GPIO_MODE_AF << (I2C3_SCL_PIN * 2));
    GPIOA->AFRL &= ~(0xFUL << (I2C3_SCL_PIN * 4));
    GPIOA->AFRL |= (I2C3_AF << (I2C3_SCL_PIN * 4));
    GPIOA->OTYPER |= (1UL << I2C3_SCL_PIN);  /* Open-drain */
    GPIOA->OSPEEDR |= (GPIO_OSPEED_HIGH << (I2C3_SCL_PIN * 2));

    GPIOA->MODER &= ~(3UL << (I2C3_SDA_PIN * 2));
    GPIOA->MODER |= (GPIO_MODE_AF << (I2C3_SDA_PIN * 2));
    GPIOA->AFRL &= ~(0xFUL << (I2C3_SDA_PIN * 4));
    GPIOA->AFRL |= (I2C3_AF << (I2C3_SDA_PIN * 4));
    GPIOA->OTYPER |= (1UL << I2C3_SDA_PIN);  /* Open-drain */
    GPIOA->PUPDR |= (GPIO_PUPD_PU << (I2C3_SDA_PIN * 2));

    /* Configure I2C3 timing for 400 kHz fast mode
     * PRESC=0, SCLL=9, SCLH=7, SDADEL=1, SCLDEL=0
     * Timing register = (0<<28)|(1<<20)|(0<<16)|(9<<8)|(7<<0)
     * This gives ~400 kHz at 170 MHz clock. */
    I2C3->TIMINGR = 0x00100907UL;

    /* Enable I2C3 */
    I2C3->CR1 = I2C_CR1_PE;

    /* VL53L0X initialization sequence (simplified from ST API) */

    /* Data init: set I2C standard mode, disable signal rate limit */
    if (!i2c3_write_reg(0x0088, 0x00)) return false;
    if (!i2c3_write_reg(0x0080, 0x01)) return false;
    if (!i2c3_write_reg(0x00FF, 0x00)) return false;
    if (!i2c3_write_reg(0x0000, 0x01)) return false;

    /* Read model ID and revision (verify sensor is present) */
    uint8_t model_id = 0;
    if (!i2c3_read_reg(0x00C0, &model_id) || model_id != 0xEE) {
        tof_error = 1;
        return false;
    }

    /* Set default signal rate limit (0.25 MCPS) */
    if (!i2c3_write_reg(0x0044, 0x00)) return false;
    if (!i2c3_write_reg(0x0045, 0x20)) return false;  /* 0.25 in 9.7 fix format */

    /* Configure GPIO for interrupt on data ready */
    uint8_t gpio_config = 0;
    if (!i2c3_read_reg(VL53L0X_REG_GPIO_HV_MUX_CTRL, &gpio_config)) return false;
    gpio_config = (gpio_config & 0xF0) | 0x04;  /* Active low interrupt */
    if (!i2c3_write_reg(VL53L0X_REG_GPIO_HV_MUX_CTRL, gpio_config)) return false;

    /* Enable temperature compensation */
    if (!i2c3_write_reg(0x0088, 0x00)) return false;

    /* Set system config: GPIO interrupt active low */
    if (!i2c3_write_reg(VL53L0X_REG_SYSTEM_CONFIG, 0x04)) return false;

    /* Configure system sequence (enable SPAD and range) */
    if (!i2c3_write_reg(VL53L0X_REG_SYSTEM_SEQUENCE, 0xFF)) return false;

    /* Start measurement in continuous mode */
    if (!i2c3_write_reg(VL53L0X_REG_SYS_RANGE_START, 0x03)) return false;

    tof_initialized = true;
    return true;
}

bool tof_drv_start_measurement(void)
{
    if (!tof_initialized)
        return false;

    /* Clear interrupt status */
    if (!i2c3_write_reg(VL53L0X_REG_RESULT_INTERRUPT_CLEAR, 0x01))
        return false;

    /* Start single-shot measurement */
    if (!i2c3_write_reg(VL53L0X_REG_SYS_RANGE_START, 0x01))
        return false;

    return true;
}

bool tof_drv_is_ready(void)
{
    if (!tof_initialized)
        return false;

    uint8_t status = 0;
    if (!i2c3_read_reg(VL53L0X_REG_RESULT_INTERRUPT_STATUS, &status))
        return false;

    /* Bit 0 = new sample ready */
    return (status & 0x07) != 0;
}

uint16_t tof_drv_read_distance_mm(void)
{
    if (!tof_initialized)
        return 0;

    /* Read 12-bit distance from result register */
    uint16_t distance = 0;
    if (!i2c3_read_reg16(VL53L0X_REG_RESULT_RANGE_STATUS + 10, &distance))
        return 0;

    /* Clear interrupt */
    i2c3_write_reg(VL53L0X_REG_RESULT_INTERRUPT_CLEAR, 0x01);

    return distance;
}

uint16_t tof_drv_measure_mm(uint32_t timeout_ms)
{
    if (!tof_drv_start_measurement())
        return 0;

    uint32_t count = 0;
    while (!tof_drv_is_ready() && count < timeout_ms * 10) {
        count++;
        i2c3_delay();
    }

    if (!tof_drv_is_ready())
        return 0;

    return tof_drv_read_distance_mm();
}

uint8_t tof_drv_get_error(void)
{
    return tof_error;
}