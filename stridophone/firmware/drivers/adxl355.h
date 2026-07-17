/*
 * adxl355.h — 3-axis vibration accelerometer driver.
 * Author : jayis1
 * License: MIT
 */
#ifndef STRIDOPHONE_ADXL355_H
#define STRIDOPHONE_ADXL355_H

#include <stdint.h>
#include "board.h"

/* ADXL355 register map subset. */
#define ADXL_DEVID_AD     0x00
#define ADXL_DEVID_MST    0x01
#define ADXL_PARTID       0x02
#define ADXL_STATUS       0x04
#define ADXL_FIFO_ENTRIES 0x05
#define ADXL_TEMP2        0x07
#define ADXL_TEMP1        0x06
#define ADXL_XDATA3       0x08
#define ADXL_YDATA3       0x09
#define ADXL_ZDATA3       0x0A
#define ADXL_FILTER       0x28
#define ADXL_FIFO_SAMPLES 0x29
#define ADXL_INT_MAP      0x2A
#define ADXL_SYNC         0x2B
#define ADXL_RANGE        0x2C
#define ADXL_POWER_CTL    0x2D
#define ADXL_RESET        0x2F

/* Operating modes. */
#define ADXL_MODE_STANDBY   0x00
#define ADXL_MODE_MEASURE   0x01

/* Range codes. */
#define ADXL_RANGE_2G   0x01
#define ADXL_RANGE_4G   0x02
#define ADXL_RANGE_8G   0x03

/* Initialise SPI2, set range +/-2g, ODR 1 kHz HPF off, standby. */
void adxl355_init(void);

/* Enter measure mode. */
void adxl355_start(void);

/* Enter standby (low power). */
void adxl355_stop(void);

/* Read up to n samples per axis from the FIFO into dst[3][n] in g units.
 * Returns the number actually read. */
int adxl355_read_block(float dst[3][DSP_FRAME_N], int n);

/* Read chip temperature in degrees C. */
float adxl355_read_temp(void);

#endif