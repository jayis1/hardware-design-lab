/*
 * EAG-7000 — I2C Driver Header
 *
 * Copyright (c) 2026 EAG-7000 Project
 * SPDX-License-Identifier: MIT
 */

#ifndef EAG7000_I2C_H
#define EAG7000_I2C_H

#include <stdint.h>

/* I2C device handle */
typedef struct {
    uintptr_t base;        /* I2C base address (I2C1/2/3_BASE) */
    uint32_t  speed_hz;    /* Configured I2C speed */
} i2c_dev_t;

/**
 * Initialize I2C peripheral.
 * @param dev       I2C device handle
 * @param base_addr I2C base address
 * @param speed_hz  Desired I2C speed (up to 400 kHz)
 * @return 0 on success, negative on error
 */
int i2c_init(i2c_dev_t *dev, uintptr_t base_addr, uint32_t speed_hz);

/**
 * Write multiple bytes to an I2C slave register.
 * @param dev         I2C device handle
 * @param slave_addr  7-bit slave address
 * @param reg         Register address
 * @param data        Data to write
 * @param len         Number of bytes to write
 * @return 0 on success, negative on error
 */
int i2c_write_reg(i2c_dev_t *dev, uint8_t slave_addr,
                   uint8_t reg, const uint8_t *data, uint16_t len);

/**
 * Read multiple bytes from an I2C slave register.
 * @param dev         I2C device handle
 * @param slave_addr  7-bit slave address
 * @param reg         Register address
 * @param buf         Buffer for received data
 * @param len         Number of bytes to read
 * @return 0 on success, negative on error
 */
int i2c_read_reg(i2c_dev_t *dev, uint8_t slave_addr,
                  uint8_t reg, uint8_t *buf, uint16_t len);

/**
 * Write a single byte to an I2C slave register.
 * Convenience wrapper for i2c_write_reg with len=1.
 */
int i2c_write_byte_reg(i2c_dev_t *dev, uint8_t slave_addr,
                        uint8_t reg, uint8_t value);

/**
 * Read a single byte from an I2C slave register.
 * @return Register value on success, negative on error
 */
int i2c_read_byte_reg(i2c_dev_t *dev, uint8_t slave_addr, uint8_t reg);

#endif /* EAG7000_I2C_H */