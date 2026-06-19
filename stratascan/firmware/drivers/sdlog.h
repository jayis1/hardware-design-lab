/*
 * sdlog.h — MicroSD Card SEG-Y Logging
 * StrataScan — Open-Source Stepped-Frequency Ground Penetrating Radar
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef STRATASCAN_SDLOG_H
#define STRATASCAN_SDLOG_H

#include <stdint.h>

int  sdlog_init(void);
int  sdlog_start_survey(uint8_t band_idx, float eps_r);
int  sdlog_write_trace(const float *trace, uint16_t n,
                       uint32_t trace_num, float lat, float lon);
int  sdlog_end_survey(void);
int  sdlog_sync(void);

#endif /* STRATASCAN_SDLOG_H */