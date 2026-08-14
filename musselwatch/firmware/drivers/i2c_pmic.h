/*
 * drivers/i2c_pmic.h — I2C driver for BQ25870 charger + FM24C64 FRAM
 *
 * Author:  jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */
#ifndef MUSSEL_I2C_PMIC_H
#define MUSSEL_I2C_PMIC_H

#include <stdint.h>
#include <stdbool.h>

void i2c1_init(void);
bool i2c1_write(uint8_t addr, uint8_t reg, uint8_t val);
bool i2c1_read(uint8_t addr, uint8_t reg, uint8_t *val);
bool i2c1_read_burst(uint8_t addr, uint8_t reg, uint8_t *buf, uint8_t len);

/* BQ25870 charger */
bool pmic_set_charge_current_ma(uint16_t current_ma);
bool pmic_get_charger_state(uint8_t *state);
bool pmic_enable_shipping_mode(void);

/* FM24C64 FRAM event log */
bool fram_write_event(uint32_t addr, const uint8_t *data, uint8_t len);
bool fram_read_event(uint32_t addr, uint8_t *buf, uint8_t len);

#endif