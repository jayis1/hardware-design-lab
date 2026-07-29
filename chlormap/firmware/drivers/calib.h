/*
 * calib.h — White reference + SPAD calibration storage
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#ifndef DRIVERS_CALIB_H
#define DRIVERS_CALIB_H

#include <stdint.h>
#include <stdbool.h>
#include "board.h"

/* Calibration data structure (stored in Flash) */
typedef struct {
    uint32_t magic;                          /* CALIB_MAGIC */
    uint16_t version;
    uint16_t crc;
    int32_t  white_ref[ARRAY_ELEMENTS];      /* dark-corrected white reference */
    int16_t  spad_slope_x1000;               /* SPAD calibration slope */
    int16_t  spad_offset;                    /* SPAD calibration offset */
    uint32_t timestamp;                      /* calibration time */
} calib_data_t;

/* Initialize calibration (load from Flash) */
bool calib_init(void);

/* Check if valid calibration exists */
bool calib_is_valid(void);

/* Store white reference spectrum (128 elements, dark-corrected) */
bool calib_store_white_reference(const int32_t *ref);

/* Get white reference spectrum */
bool calib_get_white_reference(int32_t *ref);

/* Set SPAD calibration coefficients */
bool calib_set_spad_coefficients(int16_t slope_x1000, int16_t offset);

/* Get SPAD calibration coefficients */
bool calib_get_spad_coefficients(int16_t *slope_x1000, int16_t *offset);

/* Erase calibration */
bool calib_erase(void);

#endif /* DRIVERS_CALIB_H */