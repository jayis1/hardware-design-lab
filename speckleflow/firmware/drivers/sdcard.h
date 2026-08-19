/*
 * sdcard.h — microSD logging driver interface
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#ifndef SPECKLEFLOW_SDCARD_H
#define SPECKLEFLOW_SDCARD_H

#include <stdint.h>

/**
 * Initialize the SDMMC1 interface and probe the card.
 * @return 0 on success, -1 on failure
 */
int sdcard_init(void);

/**
 * Start a new logging session (increments session counter).
 */
int sdcard_start_session(void);

/**
 * Log a single flow-map frame to the card.
 * @param flow_map     Pointer to frame data
 * @param frame_size   Size in bytes
 * @param timestamp_ms  System timestamp
 * @return 0 on success, -1 on write error
 */
int sdcard_log_frame(const uint8_t *flow_map, uint32_t frame_size,
                     uint32_t timestamp_ms);

/**
 * Get the current session number.
 */
uint32_t sdcard_get_session_number(void);

/**
 * Get the number of frames logged in the current session.
 */
uint32_t sdcard_get_frame_count(void);

/**
 * Check if a card is physically present.
 */
int sdcard_is_present(void);

#endif /* SPECKLEFLOW_SDCARD_H */