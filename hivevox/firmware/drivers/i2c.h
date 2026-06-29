/*
 * drivers/i2c.h — I2C + sensor driver header
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 */
#ifndef HIVEVOX_I2C_H
#define HIVEVOX_I2C_H

#include <stdint.h>

void i2c3_init(void);
int  i2c3_write(uint8_t addr7, const uint8_t *data, uint8_t n);
int  i2c3_read(uint8_t addr7, uint8_t *buf, uint8_t n);

int sht45_read(int16_t *temp_centi, uint16_t *rh_centi);
int veml7700_init(void);
int veml7700_read_lux(uint16_t *lux);

void delay_ms(uint32_t ms);

#endif /* HIVEVOX_I2C_H */