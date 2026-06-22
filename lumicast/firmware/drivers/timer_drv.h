/*
 * timer_drv.h — System timer & delay driver
 * LumiCast — Open-Source Circadian Light & SPD Analyzer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef LUMICAST_TIMER_DRV_H
#define LUMICAST_TIMER_DRV_H

#include <stdint.h>
#include <stdbool.h>
#include "../registers.h"

void     timer_init(void);
void     timer_delay_ms(uint32_t ms);
void     timer_delay_us(uint32_t us);
uint32_t timer_millis(void);
uint32_t timer_micros(void);
bool     timer_elapsed(uint32_t start_ms, uint32_t interval_ms);

#endif /* LUMICAST_TIMER_DRV_H */