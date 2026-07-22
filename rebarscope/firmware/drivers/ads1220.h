/*
 * drivers/ads1220.h — ADS1220 driver interface
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 */
#ifndef REBARSCOPE_ADS1220_H
#define REBARSCOPE_ADS1220_H

#include <stdint.h>

void     ads1220_init(void);
void     ads1220_write_reg(uint8_t reg, uint8_t val);
uint8_t  ads1220_read_reg(uint8_t reg);
int      ads1220_data_ready(void);
int32_t  ads1220_read_raw(void);
void     ads1220_select_mux(uint8_t mux);
float    ads1220_raw_to_mv(int32_t raw);
void     ads1220_powerdown(void);

#endif /* REBARSCOPE_ADS1220_H */