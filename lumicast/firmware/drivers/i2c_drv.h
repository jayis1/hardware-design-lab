/*
 * i2c_drv.h — I2C peripheral driver
 * LumiCast — Open-Source Circadian Light & SPD Analyzer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef LUMICAST_I2C_DRV_H
#define LUMICAST_I2C_DRV_H

#include <stdint.h>
#include <stdbool.h>
#include "../registers.h"

void i2c1_init(void);
void i2c3_init(void);
int  i2c_write_reg(I2C_TypeDef *i2c, uint8_t addr8bit, uint8_t reg, uint8_t val);
int  i2c_read_reg(I2C_TypeDef *i2c, uint8_t addr8bit, uint8_t reg, uint8_t *val);
int  i2c_write_burst(I2C_TypeDef *i2c, uint8_t addr8bit, uint8_t reg,
                    const uint8_t *data, uint8_t len);
int  i2c_read_burst(I2C_TypeDef *i2c, uint8_t addr8bit, uint8_t reg,
                    uint8_t *buf, uint8_t len);
int  i2c_write_buf(I2C_TypeDef *i2c, uint8_t addr8bit,
                   const uint8_t *data, uint8_t len);
int  i2c_read_buf(I2C_TypeDef *i2c, uint8_t addr8bit,
                  uint8_t *buf, uint8_t len);

#endif /* LUMICAST_I2C_DRV_H */