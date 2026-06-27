/*
 * flash_drv.h — SPI flash driver header
 * TremorSense — Wearable Tremor Characterization Band
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 */
#ifndef TREMORSENSE_FLASH_DRV_H
#define TREMORSENSE_FLASH_DRV_H

#include <stdint.h>
#include <stddef.h>

int     flash_init(void);
int     flash_read(uint32_t addr, uint8_t *buf, uint32_t len);
int     flash_write_page(uint32_t addr, const uint8_t *data, uint16_t len);
int     flash_erase_sector(uint32_t addr);
int     flash_erase_block64(uint32_t addr);
int     flash_chip_erase(void);
void    flash_enter_powerdown(void);
void    flash_wakeup(void);
uint32_t flash_read_jedec_id(void);
uint8_t  flash_read_status(void);
void    flash_write_enable(void);
void    flash_wait_ready(void);

/* Append-mode logging (circular buffer) */
int     flash_append_record(const uint8_t *data, uint16_t len);
uint32_t flash_get_write_pointer(void);
int     flash_read_record(uint32_t addr, uint8_t *buf, uint16_t max_len,
                          uint16_t *record_len);
int     flash_read_all_records(uint8_t *buf, uint32_t max_len, uint16_t *count);
void    flash_reset_write_pointer(void);

/* Wear leveling */
uint32_t flash_get_write_count(uint32_t sector);
void     flash_mark_sector_worn(uint32_t sector);

#endif /* TREMORSENSE_FLASH_DRV_H */