/*
 * flash_drv.h — External flash driver header
 * BreathPrint — Portable Breath VOC Analyzer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */

#ifndef FLASH_DRV_H
#define FLASH_DRV_H

#include <stdint.h>
#include <string.h>
#include "../board.h"

/* Device configuration structure (stored in flash) */
typedef struct {
    uint32_t device_id;        /* Unique device ID */
    uint32_t firmware_version;  /* Firmware version (BCD) */
    uint8_t  device_name[16];   /* User-configurable name */
    uint8_t  display_brightness;
    uint8_t  sample_rate;       /* Hz */
    uint8_t  auto_sleep_min;    /* Minutes before auto-sleep */
    uint8_t  reserved[8];
    uint32_t crc32;
} __attribute__((packed)) device_config_t;

void flash_spi_init(void);
void flash_read_id(uint8_t id[3]);
int flash_read(uint32_t addr, uint8_t *data, uint32_t len);
int flash_write(uint32_t addr, const uint8_t *data, uint32_t len);
int flash_erase_sector(uint32_t addr);
int flash_erase_chip(void);
uint32_t flash_crc32(const uint8_t *data, uint32_t len);
int flash_read_calib(calib_data_t *calib);
int flash_write_calib(const calib_data_t *calib);
int flash_read_config(device_config_t *config);
int flash_write_config(const device_config_t *config);
void flash_power_down(void);
void flash_power_up(void);

#endif /* FLASH_DRV_H */