/*
 * drivers/ad5940.h — AD5940 driver interface
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 */
#ifndef REBARSCOPE_AD5940_H
#define REBARSCOPE_AD5940_H

#include <stdint.h>

void  ad5940_init(void);
float ad5940_wenner_measure(float alpha_mm, uint8_t n_avg);
void  ad5940_lpr_start(float ecorr_mv);
float ad5940_lpr_sample_ua(void);
void  ad5940_lpr_stop(void);
void  ad5940_powerdown(void);

#endif /* REBARSCOPE_AD5940_H */