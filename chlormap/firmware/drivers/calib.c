/*
 * calib.c — White reference + SPAD calibration storage (Flash)
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 *
 * Calibration data is stored in the last Flash page of the STM32L432
 * (page 62, 2 KB at 0x0807F800). The structure includes a magic number
 * and CRC-16 for integrity validation.
 */

#include "calib.h"
#include "board.h"
#include "registers.h"
#include <string.h>

static calib_data_t g_calib;
static bool g_calib_valid = false;

/* ---- CRC-16 (CCITT) ---- */
static uint16_t crc16(const uint8_t *data, uint32_t len)
{
    uint16_t crc = 0xFFFF;
    for (uint32_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (int b = 0; b < 8; b++) {
            crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : (crc << 1);
        }
    }
    return crc;
}

/* ---- Flash operations (STM32L4) ---- */
static void flash_unlock(void)
{
    /* FLASH->KEYR = FLASH_KEY1; FLASH->KEYR = FLASH_KEY2; */
}

static void flash_lock(void)
{
    /* FLASH->CR |= FLASH_CR_LOCK; */
}

static bool flash_erase_page(uint32_t page_addr)
{
    /* while(FLASH->SR & FLASH_SR_BSY);
     * FLASH->CR |= FLASH_CR_PER;
     * FLASH->CR = (FLASH->CR & ~FLASH_CR_PNB) | (page << 3);
     * FLASH->CR |= FLASH_CR_STRT;
     * while(FLASH->SR & FLASH_SR_BSY);
     * FLASH->CR &= ~FLASH_CR_PER;
     */
    (void)page_addr;
    return true; /* stub */
}

static bool flash_write_dword(uint32_t addr, uint32_t data)
{
    /* while(FLASH->SR & FLASH_SR_BSY);
     * FLASH->CR |= FLASH_CR_PG;
     * *(volatile uint32_t*)addr = data;
     * *(volatile uint32_t*)(addr + 4) = 0;  — L4 writes 64-bit (2 × 32-bit)
     * while(FLASH->SR & FLASH_SR_BSY);
     * FLASH->CR &= ~FLASH_CR_PG;
     */
    (void)addr; (void)data;
    return true; /* stub */
}

static bool flash_write_buffer(uint32_t addr, const uint8_t *buf, uint32_t len)
{
    /* STM32L4 writes in 64-bit (8-byte) units.
     * Pad to 8-byte boundary and write sequentially.
     */
    uint32_t offset = 0;
    while (offset < len) {
        uint32_t dw0 = 0, dw1 = 0;
        if (offset < len) memcpy(&dw0, buf + offset, (len - offset >= 4) ? 4 : (len - offset));
        if (offset + 4 < len) memcpy(&dw1, buf + offset + 4, (len - offset - 4 >= 4) ? 4 : (len - offset - 4));
        if (!flash_write_dword(addr + offset, dw0)) return false;
        if (!flash_write_dword(addr + offset + 4, dw1)) return false;
        offset += 8;
    }
    return true;
}

/* ---- Public API ---- */

bool calib_init(void)
{
    /* Read calibration from Flash */
    const calib_data_t *flash_calib = (const calib_data_t *)CALIB_ADDR;

    memcpy(&g_calib, flash_calib, sizeof(calib_data_t));

    /* Validate magic + CRC */
    if (g_calib.magic == CALIB_MAGIC) {
        uint16_t calc_crc = crc16((const uint8_t *)&g_calib,
                                  offsetof(calib_data_t, crc));
        if (calc_crc == g_calib.crc) {
            g_calib_valid = true;
            return true;
        }
    }

    /* No valid calibration: use defaults */
    g_calib.magic = 0;
    g_calib_valid = false;
    g_calib.spad_slope_x1000 = 1000; /* slope = 1.0 */
    g_calib.spad_offset = 0;
    return false;
}

bool calib_is_valid(void)
{
    return g_calib_valid;
}

bool calib_store_white_reference(const int32_t *ref)
{
    if (!ref) return false;

    /* Copy into local struct */
    memcpy(g_calib.white_ref, ref, sizeof(int32_t) * ARRAY_ELEMENTS);
    g_calib.magic = CALIB_MAGIC;
    g_calib.version = 1;
    g_calib.timestamp = 0; /* would use RTC */
    g_calib.crc = crc16((const uint8_t *)&g_calib, offsetof(calib_data_t, crc));

    /* Erase Flash page and write */
    flash_unlock();
    if (!flash_erase_page(CALIB_ADDR)) {
        flash_lock();
        return false;
    }
    if (!flash_write_buffer(CALIB_ADDR, (const uint8_t *)&g_calib, sizeof(calib_data_t))) {
        flash_lock();
        return false;
    }
    flash_lock();

    g_calib_valid = true;
    return true;
}

bool calib_get_white_reference(int32_t *ref)
{
    if (!g_calib_valid || !ref) return false;
    memcpy(ref, g_calib.white_ref, sizeof(int32_t) * ARRAY_ELEMENTS);
    return true;
}

bool calib_set_spad_coefficients(int16_t slope_x1000, int16_t offset)
{
    g_calib.spad_slope_x1000 = slope_x1000;
    g_calib.spad_offset = offset;
    g_calib.crc = crc16((const uint8_t *)&g_calib, offsetof(calib_data_t, crc));

    flash_unlock();
    flash_erase_page(CALIB_ADDR);
    flash_write_buffer(CALIB_ADDR, (const uint8_t *)&g_calib, sizeof(calib_data_t));
    flash_lock();
    return true;
}

bool calib_get_spad_coefficients(int16_t *slope_x1000, int16_t *offset)
{
    if (!g_calib_valid) return false;
    *slope_x1000 = g_calib.spad_slope_x1000;
    *offset = g_calib.spad_offset;
    return true;
}

bool calib_erase(void)
{
    flash_unlock();
    flash_erase_page(CALIB_ADDR);
    flash_lock();
    memset(&g_calib, 0, sizeof(g_calib));
    g_calib_valid = false;
    return true;
}