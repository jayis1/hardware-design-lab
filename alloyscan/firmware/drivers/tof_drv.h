/*
 * tof_drv.h — VL53L0X time-of-flight distance sensor driver
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef ALLOYSCAN_TOF_DRV_H
#define ALLOYSCAN_TOF_DRV_H

#include <stdint.h>
#include <stdbool.h>

/* VL53L0X register addresses */
#define VL53L0X_REG_SYS_RANGE_START          0x000
#define VL53L0X_REG_RESULT_RANGE_STATUS      0x014
#define VL53L0X_REG_RESULT_INTERRUPT_STATUS  0x015
#define VL53L0X_REG_RESULT_INTERRUPT_CLEAR   0x016
#define VL53L0X_REG_CROSS_TALK_COMP_RATE      0x009
#define VL53L0X_REG_MSRC_CONFIG_CONTROL      0x009
#define VL53L0X_REG_SYSTEM_SEQUENCE          0x00A
#define VL53L0X_REG_SYSTEM_CONFIG            0x008
#define VL53L0X_REG_I2C_SLAVE_DEVICE_ADDR    0x020
#define VL53L0X_REG_GPIO_HV_MUX_CTRL         0x084

/* Initialize I2C3 and VL53L0X sensor */
bool tof_drv_init(void);

/* Start a single range measurement (non-blocking) */
bool tof_drv_start_measurement(void);

/* Check if measurement is complete */
bool tof_drv_is_ready(void);

/* Read the measured distance in millimeters (0 if error/timeout) */
uint16_t tof_drv_read_distance_mm(void);

/* Perform a blocking distance measurement with timeout (ms) */
uint16_t tof_drv_measure_mm(uint32_t timeout_ms);

/* Get last error code */
uint8_t tof_drv_get_error(void);

#endif /* ALLOYSCAN_TOF_DRV_H */