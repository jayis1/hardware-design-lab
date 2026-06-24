/*
 * flash_drv.h — QSPI flash storage for NILM model and calibration
 * WattLens — 3-Phase Power Quality Analyzer & NILM
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */

#ifndef WATTLENS_FLASH_DRV_H
#define WATTLENS_FLASH_DRV_H

#include "board.h"

void flash_drv_init(void);
int  flash_drv_read_cal(cal_data_t *cal);
int  flash_drv_write_cal(const cal_data_t *cal);
bool flash_drv_validate_cal(const cal_data_t *cal);
int  flash_drv_read_model_header(uint8_t *buf, int len);
int  flash_drv_write_model(const uint8_t *data, int len);
int  flash_drv_read_model_weights(void *w1, void *b1, void *w2, void *b2,
                                   void *w3, void *b3, void *w4, void *b4);
uint32_t flash_crc32(const void *data, int len);

#endif /* WATTLENS_FLASH_DRV_H */