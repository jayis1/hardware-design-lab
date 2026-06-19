/*
 * pll.h — ADF4159 Fractional-N PLL Driver
 * StrataScan — Open-Source Stepped-Frequency Ground Penetrating Radar
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef STRATASCAN_PLL_H
#define STRATASCAN_PLL_H

#include <stdint.h>

int  pll_init(void);
void pll_set_frequency(uint32_t freq_hz);
uint32_t pll_read_frequency(void);
void pll_power_down(void);
void pll_power_up(void);

/* ADF4159 register addresses */
#define PLL_REG_R0    0x0
#define PLL_REG_R1    0x1
#define PLL_REG_R2    0x2
#define PLL_REG_R3    0x3
#define PLL_REG_R4    0x4
#define PLL_REG_R5    0x5
#define PLL_REG_R6    0x6
#define PLL_REG_R7    0x7

/* PLL constants */
#define PLL_REF_FREQ_HZ    10000000ULL   /* 10 MHz reference */
#define PLL_INT_MAX        32767
#define PLL_FRAC_MAX       16777216   /* 2^24 fractional range */

#endif /* STRATASCAN_PLL_H */