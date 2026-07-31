/*
 * storage.h — Flash-backed configuration storage for Synthand.
 *
 * Stores calibration data and MIDI mapping in nRF5340 flash pages.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#ifndef SYNTHAND_STORAGE_H
#define SYNTHAND_STORAGE_H

#include <stdint.h>
#include "board.h"

/* Initialize flash storage (verify NVMC, check magic numbers). */
int storage_init(void);

/* Load calibration data from flash.
 * Returns 0 if valid calibration found, nonzero if missing/corrupt. */
int storage_load_calibration(calibration_t *calib);

/* Save calibration data to flash (erases page, then writes).
 * Returns 0 on success. */
int storage_save_calibration(const calibration_t *calib);

/* Load MIDI mapping from flash.
 * Returns 0 if valid mapping found, nonzero if missing/corrupt. */
int storage_load_mapping(mapping_t *mapping);

/* Save MIDI mapping to flash.
 * Returns 0 on success. */
int storage_save_mapping(const mapping_t *mapping);

/* Erase all stored configuration (factory reset). */
int storage_erase_all(void);

/* Compute CRC32 (used for verifying configuration integrity). */
uint32_t storage_crc32(const void *data, size_t length);

/* Flash page operations */
int storage_page_erase(uint32_t page_number);
int storage_page_write(uint32_t page_number, const void *data, size_t length);

/* Verify data at a flash address matches the given buffer */
int storage_verify(uint32_t addr, const void *data, size_t length);

#endif /* SYNTHAND_STORAGE_H */