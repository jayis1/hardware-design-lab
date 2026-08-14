/*
 * drivers/onewire.h — 1-Wire driver for DS18B20 water temperature sensor
 *
 * Author:  jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */
#ifndef MUSSEL_ONEWIRE_H
#define MUSSEL_ONEWIRE_H

#include <stdint.h>
#include <stdbool.h>

bool    ow_init(void);
bool    ow_reset(void);
void    ow_write_byte(uint8_t byte);
uint8_t ow_read_byte(void);
void    ow_write_bit(uint8_t bit);
uint8_t ow_read_bit(void);

bool    ds18b20_read_temp_c10(int16_t *temp_c10);
bool    ow_read_rom(uint8_t rom[8]);

#endif