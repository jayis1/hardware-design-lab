/*
 * flashio.h — W25Q64 SPI NOR flash ring-journal for offline session logging
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */
#ifndef INKWELL_DRIVERS_FLASHIO_H
#define INKWELL_DRIVERS_FLASHIO_H

#include <stdint.h>
#include <stdbool.h>

void     flashio_init(void);
bool     flashio_append(const void *rec, uint32_t len);
uint32_t flashio_fill_pct(void);
bool     flashio_read(uint32_t offset, void *buf, uint32_t len);
uint32_t flashio_get_write_ptr(void);
bool     flashio_erase_sector(uint32_t sector_idx);
bool     flashio_replay_range(uint32_t seq_start, uint32_t seq_end,
                             void (*emit)(const void *rec, uint32_t len));

#endif