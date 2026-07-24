/*
 * flash_drv.h — W25Q128 SPI flash driver
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef ALLOYSCAN_FLASH_DRV_H
#define ALLOYSCAN_FLASH_DRV_H

#include <stdint.h>
#include <stdbool.h>

/* W25Q128 JEDEC ID */
#define W25Q128_JEDEC_ID    0xEF4018

/* Flash commands */
#define FLASH_CMD_READ       0x03
#define FLASH_CMD_FAST_READ  0x0B
#define FLASH_CMD_WRITE_EN   0x06
#define FLASH_CMD_WRITE_DISABLE 0x04
#define FLASH_CMD_PAGE_PROG  0x02
#define FLASH_CMD_SECTOR_ERASE 0x20
#define FLASH_CMD_BLOCK_ERASE_32K 0x52
#define FLASH_CMD_BLOCK_ERASE_64K 0xD8
#define FLASH_CMD_CHIP_ERASE 0x60
#define FLASH_CMD_READ_ID    0x9F
#define FLASH_CMD_READ_STATUS 0x05
#define FLASH_CMD_WRITE_STATUS 0x01
#define FLASH_CMD_POWER_DOWN 0xB9
#define FLASH_CMD_POWER_UP   0xAB
#define FLASH_CMD_READ_UID   0x4B

/* Status register bits */
#define FLASH_SR_BUSY        0x01
#define FLASH_SR_WEN         0x02
#define FLASH_SR_BP0          0x04
#define FLASH_SR_BP1          0x08
#define FLASH_SR_BP2          0x10
#define FLASH_SR_TB          0x20
#define FLASH_SR_SEC         0x40
#define FLASH_SR_SRP         0x80

#define FLASH_PAGE_SIZE      256
#define FLASH_SECTOR_SIZE    4096
#define FLASH_BLOCK_32K      32768
#define FLASH_BLOCK_64K      65536
#define FLASH_TOTAL_SIZE     (16 * 1024 * 1024)  /* 16 MB */

/* Initialize SPI3 and flash driver */
bool flash_drv_init(void);

/* Read JEDEC ID (should return W25Q128_JEDEC_ID) */
uint32_t flash_drv_read_id(void);

/* Read data from flash (up to 256 bytes per call recommended) */
bool flash_drv_read(uint32_t addr, uint8_t *data, uint32_t len);

/* Write a page (256 bytes max). Address must be page-aligned for best results. */
bool flash_drv_write_page(uint32_t addr, const uint8_t *data, uint32_t len);

/* Erase a 4K sector */
bool flash_drv_erase_sector(uint32_t addr);

/* Erase a 64K block */
bool flash_drv_erase_block_64k(uint32_t addr);

/* Write data (handles page boundary crossing and erase automatically) */
bool flash_drv_write(uint32_t addr, const uint8_t *data, uint32_t len);

/* Wait for flash to be idle (not busy) */
bool flash_drv_wait_idle(uint32_t timeout_ms);

/* Read flash status register */
uint8_t flash_drv_read_status(void);

/* Power down flash (low power mode) */
void flash_drv_power_down(void);

/* Wake up flash from power down */
void flash_drv_power_up(void);

#endif /* ALLOYSCAN_FLASH_DRV_H */