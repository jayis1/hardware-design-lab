/*
 * i2c_drv.h — I2C driver header
 * BreathPrint — Portable Breath VOC Analyzer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */

#ifndef I2C_DRV_H
#define I2C_DRV_H

#include <stdint.h>
#include "../registers.h"

void i2c_init(i2c_reg_t *i2c, uint32_t timing);
int i2c_write(i2c_reg_t *i2c, uint8_t addr, const uint8_t *data,
              uint16_t len, uint8_t stop);
int i2c_read(i2c_reg_t *i2c, uint8_t addr, uint8_t *data,
             uint16_t len, uint8_t stop);
int i2c_write_reg(i2c_reg_t *i2c, uint8_t addr, uint8_t reg, uint8_t val);
int i2c_write_reg16(i2c_reg_t *i2c, uint8_t addr, uint16_t reg);
int i2c_read_reg(i2c_reg_t *i2c, uint8_t addr, uint8_t reg,
                 uint8_t *data, uint16_t len);
int i2c_read_cmd16(i2c_reg_t *i2c, uint8_t addr, uint16_t cmd,
                   uint8_t *data, uint16_t len);
uint8_t i2c_crc8(const uint8_t *data, uint8_t len);
int i2c_scan(i2c_reg_t *i2c, uint8_t *found_addrs, uint8_t max_addrs);
void i2c_reset(i2c_reg_t *i2c);
int i2c_read_dma(i2c_reg_t *i2c, uint8_t addr, uint8_t *data,
                 uint16_t len, dma_stream_reg_t *dma);

#endif /* I2C_DRV_H */