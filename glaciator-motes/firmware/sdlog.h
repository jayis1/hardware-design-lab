/*
 * sdlog.h — microSD SPI logger with FAT-lite ring buffer
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#ifndef SDLOG_H
#define SDLOG_H

#include <stdint.h>
#include <stdbool.h>
#include "seismology.h"

bool   sdlog_init(void);
bool   sdlog_write_event(const event_meta_t *meta,
                         const uint8_t *waveform, uint32_t len);
bool   sdlog_write_continuous(const uint8_t *buf, uint32_t len);
uint32_t sdlog_capacity_mb(void);
uint32_t sdlog_used_mb(void);
bool   sdlog_sync(void);

#endif /* SDLOG_H */