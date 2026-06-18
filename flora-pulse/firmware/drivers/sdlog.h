/*
 * sdlog.h — SD card data logging driver interface
 * FloraPulse — Plant Electrophysiology & Sap-Flow Stress Monitor
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef FLORAPULSE_SDLOG_H
#define FLORAPULSE_SDLOG_H

#include <stdint.h>

/* Initialize SD card via SPI2 (shared with ADS1220, CS-multiplexed).
 * Performs SD initialization sequence (CMD0, CMD8, ACMD41).
 * Returns 0 on success, non-zero on error.
 */
uint8_t sd_init(void);

/* Open a new log file (creates sequential filename LOG00001.BIN, etc.)
 * Returns 0 on success.
 */
uint8_t sd_open_log(void);

/* Append a binary record to the log file.
 * len must be LOG_RECORD_SIZE (48 bytes).
 * Returns 0 on success.
 */
uint8_t sd_write_record(const uint8_t *data, uint16_t len);

/* Read records from the log starting at a given record index.
 * Returns number of records actually read.
 */
uint16_t sd_read_records(uint32_t start_idx, uint8_t *buf, uint16_t max_records);

/* Get total number of records stored */
uint32_t sd_get_record_count(void);

/* Close the log file (flush) */
void sd_close_log(void);

/* Erase all log files (factory reset) */
uint8_t sd_erase_logs(void);

/* Check if SD card is present and writable */
uint8_t sd_is_present(void);

/* Get total capacity in MB */
uint32_t sd_get_capacity_mb(void);

#endif /* FLORAPULSE_SDLOG_H */