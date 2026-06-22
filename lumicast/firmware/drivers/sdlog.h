/*
 * sdlog.h — microSD card logging driver
 * LumiCast — Open-Source Circadian Light & SPD Analyzer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef LUMICAST_SDLOG_H
#define LUMICAST_SDLOG_H

#include <stdint.h>
#include <stdbool.h>

#define SD_BLOCK_SIZE     512

typedef enum {
    SD_OK = 0,
    SD_ERR_INIT = -1,
    SD_ERR_TIMEOUT = -2,
    SD_ERR_CRC = -3,
    SD_ERR_WRITE = -4,
    SD_ERR_NOCARD = -5,
} sd_status_t;

int  sdlog_init(void);
bool sdlog_present(void);
int  sdlog_write_block(uint32_t block_addr, const uint8_t *data);
int  sdlog_read_block(uint32_t block_addr, uint8_t *data);
int  sdlog_append(const uint8_t *data, uint32_t len);
int  sdlog_sync(void);
uint32_t sdlog_get_file_size(void);

#endif /* LUMICAST_SDLOG_H */