/*
 * tdc.h — TDC-GP22 Time-to-Digital Converter Driver
 *
 * LignoScan — Portable Acoustic Tomography Scanner
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#ifndef LIGNOSCAN_TDC_H
#define LIGNOSCAN_TDC_H

#include <stdint.h>
#include "board.h"

/* TDC-GP22 SPI command opcodes */
#define TDC_CMD_WRITE_CFG       0xB0  /* Write 4 bytes to config */
#define TDC_CMD_READ_CFG        0xB1  /* Read config */
#define TDC_CMD_START_TOF       0x01  /* Start ToF measurement */
#define TDC_CMD_START_CAL       0x02  /* Start calibration */
#define TDC_CMD_READ_RESULT     0x03  /* Read result register */
#define TDC_CMD_READ_STAT       0x04  /* Read status */
#define TDC_CMD_INIT            0x70  /* Initialize */
#define TDC_CMD_POWER_DOWN      0x80  /* Power down */

/* TDC-GP22 register addresses */
#define TDC_REG_CFG0            0x00
#define TDC_REG_CFG1            0x01
#define TDC_REG_CFG2            0x02
#define TDC_REG_CFG3            0x03
#define TDC_REG_CFG4            0x04
#define TDC_REG_CFG5            0x05
#define TDC_REG_CFG6            0x06

/* Configuration bit definitions */
#define TDC_CFG0_FILL_1        (1U << 3)   /* Enable 1 stop channel */
#define TDC_CFG0_CALIBRATE     (1U << 4)   /* Auto calibration enabled */
#define TDC_CFG0_DIV4         (1U << 7)   /* Divide clock by 4 */

#define TDC_CFG1_START_EDGE_R  (0U << 0)   /* Rising edge start */
#define TDC_CFG1_STOP_EDGE_R   (0U << 1)   /* Rising edge stop */
#define TDC_CFG1_HIT1         (1U << 2)   /* Hit 1 mode */
#define TDC_CFG1_QUAD_RES     (1U << 4)   /* 4x resolution */

/* Conversion: TDC raw count to nanoseconds.
 * With 4 MHz calibration clock and 4x resolution:
 * 1 LSB = 1/(4MHz * 4) = 62.5 ps
 * For 22 ps resolution mode with interpolation: see datasheet.
 * In practice: ns = raw * 0.0625 (calibrated) */
#define TDC_NS_PER_LSB        0.0625f

/* TDC measurement result */
typedef struct {
    float tof_ns;           /* Time of flight in nanoseconds */
    uint32_t raw_count;     /* Raw TDC count */
    uint8_t status;         /* Status flags */
    uint8_t valid;          /* 1 if measurement valid */
} tdc_result_t;

/* Function prototypes */
void tdc_init(void);
void tdc_configure(void);
void tdc_arm(void);
int tdc_wait_result(float *tof_ns, uint32_t timeout_us);
float tdc_measure_cable_delay(int channel);
void tdc_calibrate(void);
void tdc_power_down(void);

/* Low-level SPI helpers */
void tdc_spi_write(uint8_t cmd, uint32_t data);
uint32_t tdc_spi_read(uint8_t cmd);
void tdc_cs_low(void);
void tdc_cs_high(void);

#endif /* LIGNOSCAN_TDC_H */