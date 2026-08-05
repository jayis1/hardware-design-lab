/*
 * drivers/flashio.h — Flash journal driver header
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#ifndef FROSTSENTINEL_FLASHIO_H
#define FROSTSENTINEL_FLASHIO_H

#include <stdint.h>

/* Initialize the flash journal. Loads metadata from the double-buffered
 * metadata sector. On first boot, initializes an empty journal. */
void flashio_init(void);

/* Write one 24-byte record to the journal. Returns 0 on success. */
int  flashio_write_record(const uint8_t *record);

/* Read the Nth most recent record (0 = newest). Returns 0 on success,
 * -1 if index is out of range. */
int  flashio_read_record(uint32_t index_from_newest, uint8_t *out);

/* Flush metadata to flash (call before sleep). */
void flashio_flush(void);

/* Get total records ever written. */
uint32_t flashio_get_record_count(void);

/* Build a 24-byte record from the current global sensor state. */
void flashio_build_record(uint8_t *buf, uint32_t timestamp);

#endif /* FROSTSENTINEL_FLASHIO_H */