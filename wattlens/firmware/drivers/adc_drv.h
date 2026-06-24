/*
 * adc_drv.h — ADS131M08 8-channel 24-bit ADC driver
 * WattLens — 3-Phase Power Quality Analyzer & NILM
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */

#ifndef WATTLENS_ADC_DRV_H
#define WATTLENS_ADC_DRV_H

#include "board.h"

/* ADS131M08 register addresses */
#define ADS_REG_ID         0x00
#define ADS_REG_CONFIG1    0x01
#define ADS_REG_CONFIG2    0x02
#define ADS_REG_CONFIG3    0x03
#define ADS_REG_OSR        0x04
#define ADS_REG_CLOCK      0x05
#define ADS_REG_GAIN       0x09
#define ADS_REG_CHANNEL    0x0A
#define ADS_REG_STATUS     0x22

/* SPI command frame format (6 bytes) */
#define ADS_CMD_READ       0xA000
#define ADS_CMD_WRITE      0x4000
#define ADS_CMD_RESET      0x0011

void adc_drv_init(void);
void adc_drv_start(void);
void adc_drv_stop(void);
void adc_start_frame_read(uint8_t *dest);
int  adc_read_register(uint8_t reg, uint16_t *val);
int  adc_write_register(uint8_t reg, uint16_t val);

#endif /* WATTLENS_ADC_DRV_H */