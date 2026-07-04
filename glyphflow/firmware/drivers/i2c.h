/*
 * i2c.h — Minimal I²C1 driver (100/400 kHz, blocking)
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 */
#ifndef GLYPHFLOW_I2C_H
#define GLYPHFLOW_I2C_H

#include <stdint.h>

int i2c_init(void);
int i2c_write_reg(uint8_t dev_addr_7bit, uint8_t reg, uint8_t val);
int i2c_read_reg(uint8_t dev_addr_7bit, uint8_t reg, uint8_t *val);
int i2c_read_burst(uint8_t dev_addr_7bit, uint8_t reg, uint8_t *buf, uint8_t n);

#endif /* GLYPHFLOW_I2C_H */