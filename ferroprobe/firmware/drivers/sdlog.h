/*
 * sdlog.h — FAT32 SD card data logger interface
 * FerroProbe — Open-Source 3-Axis Fluxgate Magnetometer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef FERROPROBE_SDLOG_H
#define FERROPROBE_SDLOG_H

#include <stdint.h>
#include "lockin.h"
#include "gps.h"
#include "imu.h"

/* Initialize the SD card and FAT32 logging system.
 * Uses SPI1 (shared with ADS1256 via CS multiplexing).
 */
uint8_t sdlog_init(void);

/* Start a new survey log file.
 * Generates a filename based on date/time: SURVEY_NN.BIN
 * Returns 1 on success, 0 on failure.
 */
uint8_t sdlog_start(const char *filename);

/* Write a single measurement record to the log.
 * Combines field, GPS, and orientation data into a 32-byte binary record.
 */
uint8_t sdlog_write(const field_measurement_t *field,
                     const gps_data_t *gps,
                     const imu_data_t *imu,
                     uint8_t flags);

/* Close the current log file (flush + sync) */
void sdlog_stop(void);

/* Get the number of records written in the current session */
uint32_t sdlog_record_count(void);

/* Read back records from a log file (for BLE download) */
uint8_t sdlog_read_record(uint32_t index, uint8_t *buf, uint8_t len);

/* Get current log filename */
const char *sdlog_get_filename(void);

/* Erase all log files on the card */
uint8_t sdlog_erase_all(void);

/* Log flags */
#define LOG_FLAG_GPS_FIX    0x01
#define LOG_FLAG_ANOMALY    0x02
#define LOG_FLAG_CALIB_OK   0x04

#endif /* FERROPROBE_SDLOG_H */