/*
 * sdlog.h — SD Card Data Logging Driver
 *
 * LignoScan — Portable Acoustic Tomography Scanner
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#ifndef LIGNOSCAN_SDLOG_H
#define LIGNOSCAN_SDLOG_H

#include <stdint.h>
#include "ble.h"

void sdlog_init(void);
int sdlog_write_scan(const char *tree_id, const char *timestamp,
                     gps_fix_t *gps, int n_sensors, float diameter,
                     float *tof_matrix, float *amplitude, int *quality);
int sdlog_write_header(const char *tree_id, const char *timestamp,
                       gps_fix_t *gps, int n_sensors, float diameter);
int sdlog_write_matrix(float *tof, float *amp, int *qual, int n);

/* Low-level SD card SPI interface */
int sd_card_init(void);
int sd_read_block(uint32_t block, uint8_t *buf);
int sd_write_block(uint32_t block, const uint8_t *buf);

#endif /* LIGNOSCAN_SDLOG_H */