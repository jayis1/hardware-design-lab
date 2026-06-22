/*
 * sdlog.h — SD card logging header
 * BreathPrint — Portable Breath VOC Analyzer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */

#ifndef SDLOG_H
#define SDLOG_H

#include <stdint.h>
#include <string.h>
#include "../board.h"

void sd_spi_init(void);
int sd_init(void);
uint8_t sd_detect(void);
int sd_read_block(uint32_t block_addr, uint8_t *buffer);
int sd_write_block(uint32_t block_addr, const uint8_t *buffer);
int sdlog_write_result(const breath_result_t *result);
int sdlog_read_block(uint32_t block_num, uint8_t *buffer);
uint32_t sdlog_get_block_count(void);
void sdlog_flush(void);

#endif /* SDLOG_H */