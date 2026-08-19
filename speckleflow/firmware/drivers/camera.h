/*
 * camera.h — OV9281 camera driver interface
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#ifndef SPECKLEFLOW_CAMERA_H
#define SPECKLEFLOW_CAMERA_H

#include <stdint.h>

/**
 * Initialize the OV9281 camera for 1280×800 monochrome global-shutter
 * operation at 120 fps via 8-bit DVP.
 * @return 0 on success, negative on error (-1 = I2C fail, -2 = wrong ID)
 */
int camera_init(void);

/**
 * Set exposure time in microseconds (manual AEC mode).
 * @param us  Exposure in µs (100–50000)
 * @return 0 on success, -1 on I2C error
 */
int camera_set_exposure(uint32_t us);

/**
 * Set analog gain (manual AGC mode).
 * @param gain  0x0010 = 1×, 0x0020 = 2×, 0x0040 = 4× (max 0x7F)
 * @return 0 on success, -1 on I2C error
 */
int camera_set_gain(uint16_t gain);

/**
 * Set frame rate by adjusting VTS.
 * @param fps  Desired frames per second (10–120)
 * @return 0 on success, -1 on I2C error
 */
int camera_set_fps(uint8_t fps);

/**
 * Trigger a single frame (global-shutter single-shot mode).
 */
void camera_trigger(void);

/**
 * Put camera in software standby.
 * @return 0 on success, -1 on I2C error
 */
int camera_standby(void);

/**
 * Read the OV9281 chip ID (should be 0x9281).
 * @param id  Pointer to receive 16-bit ID
 * @return 0 on success, -1 on I2C error
 */
int camera_read_id(uint16_t *id);

#endif /* SPECKLEFLOW_CAMERA_H */