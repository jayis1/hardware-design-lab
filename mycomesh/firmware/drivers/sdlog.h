/*
 * sdlog.h — microSD binary data logger header for MycoMesh
 * MycoMesh — Open-Source Fungal Mycelium Electrophysiology & Chemical Sensing Network
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef MYCOMESH_SDLOG_H
#define MYCOMESH_SDLOG_H

#include "board.h"
#include <stdint.h>

/* Initialize SD card and open a new log file. */
int sdlog_init(void);

/* Write a block of raw channel data to the log.
 * Format: [header 16B] [16ch × 64 samples × 3B] [CRC16 2B] */
int sdlog_write_block(int16_t filtered[ADS1298_TOTAL_CHANS][DSP_BLOCK_SIZE],
                      uint16_t n_samples, uint32_t timestamp_ms);

/* Write an epoch summary record to the log. */
int sdlog_write_epoch(const epoch_summary_t *epoch);

/* Flush pending data to the SD card. */
int sdlog_flush(void);

/* Close the current log file. */
int sdlog_close(void);

/* Get the current log file size in bytes. */
uint32_t sdlog_file_size(void);

/* Get the number of blocks written in the current session. */
uint32_t sdlog_block_count(void);

#endif /* MYCOMESH_SDLOG_H */