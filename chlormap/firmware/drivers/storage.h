/*
 * storage.h — microSD FAT32 CSV logging interface
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#ifndef DRIVERS_STORAGE_H
#define DRIVERS_STORAGE_H

#include <stdint.h>
#include <stdbool.h>

/* Initialize SD card (SPI mode) + mount FAT32 */
bool storage_init(void);

/* Append a CSV line to the log file */
bool storage_append_line(const char *line);

/* Write header line if file is new */
bool storage_write_header(void);

/* Check if SD card is present */
bool storage_is_present(void);

/* Get number of logged measurements */
uint32_t storage_get_count(void);

/* Flush pending writes */
bool storage_flush(void);

/* Format/erase log file */
bool storage_clear_log(void);

#endif /* DRIVERS_STORAGE_H */