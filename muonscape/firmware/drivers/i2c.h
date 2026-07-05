/*
 * i2c.h — I²C bus driver interface
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: MIT
 */
#ifndef MUONSCAPE_DRV_I2C_H
#define MUONSCAPE_DRV_I2C_H

#include <stdint.h>
#include "board.h"

#ifdef __cplusplus
extern "C" {
#endif

muon_status_t i2c_init(void);
muon_status_t i2c_write(uint8_t addr, const uint8_t *data, uint16_t n);
muon_status_t i2c_read(uint8_t addr, uint8_t *data, uint16_t n);
muon_status_t i2c_write_reg(uint8_t addr, uint8_t reg, uint8_t val);
muon_status_t i2c_read_reg(uint8_t addr, uint8_t reg, uint8_t *val, uint16_t n);

#ifdef __cplusplus
}
#endif
#endif /* MUONSCAPE_DRV_I2C_H */