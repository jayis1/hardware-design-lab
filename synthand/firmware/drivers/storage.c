/*
 * storage.c — Flash-backed configuration storage for Synthand.
 *
 * Stores calibration data and MIDI mapping in nRF5340 flash pages 253-254.
 * Uses CRC32 for data integrity verification.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#include <stdint.h>
#include <string.h>
#include "board.h"
#include "registers.h"
#include "drivers/storage.h"

/* -------------------------------------------------------------------------
 * CRC32 computation (IEEE 802.3 polynomial)
 * Author: jayis1
 * ------------------------------------------------------------------------- */
uint32_t storage_crc32(const void *data, size_t length)
{
    const uint8_t *bytes = (const uint8_t *)data;
    uint32_t crc = 0xFFFFFFFF;

    for (size_t i = 0; i < length; i++) {
        crc ^= bytes[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc >>= 1;
            }
        }
    }

    return crc ^ 0xFFFFFFFF;
}

/* -------------------------------------------------------------------------
 * Flash page operations
 * Author: jayis1
 * ------------------------------------------------------------------------- */

/* Flash page size on nRF5340: 4096 bytes */
#define FLASH_PAGE_SIZE 4096U

int storage_page_erase(uint32_t page_number)
{
    uint32_t page_addr = page_number * FLASH_PAGE_SIZE;

    /* Wait for NVMC to be ready */
    while (NVMC->READY == 0);

    /* Enable erase */
    NVMC->CONFIG = NVMC_CONFIG_EEN;
    while (NVMC->READY == 0);

    /* Erase the page */
    NVMC->ERASEPAGE = page_addr;
    while (NVMC->READY == 0);

    /* Return to read-only */
    NVMC->CONFIG = NVMC_CONFIG_REN;
    while (NVMC->READY == 0);

    return 0;
}

int storage_page_write(uint32_t page_number, const void *data, size_t length)
{
    uint32_t page_addr = page_number * FLASH_PAGE_SIZE;
    const uint32_t *src = (const uint32_t *)data;
    volatile uint32_t *dst = (volatile uint32_t *)page_addr;

    if (length > FLASH_PAGE_SIZE)
        return -1;

    /* Wait for NVMC */
    while (NVMC->READY == 0);

    /* Enable write */
    NVMC->CONFIG = NVMC_CONFIG_WEN;
    while (NVMC->READY == 0);

    /* Write 32-bit words (nRF flash requires word-aligned writes) */
    size_t words = (length + 3) / 4;
    for (size_t i = 0; i < words; i++) {
        uint32_t word = 0xFFFFFFFF;
        if (i * 4 < length) {
            memcpy(&word, &src[i], (length - i * 4 < 4) ? (length - i * 4) : 4);
        }
        dst[i] = word;
        while (NVMC->READY == 0);
    }

    /* Return to read-only */
    NVMC->CONFIG = NVMC_CONFIG_REN;
    while (NVMC->READY == 0);

    return 0;
}

int storage_verify(uint32_t addr, const void *data, size_t length)
{
    const uint8_t *expected = (const uint8_t *)data;
    const uint8_t *actual = (const uint8_t *)addr;

    for (size_t i = 0; i < length; i++) {
        if (actual[i] != expected[i]) {
            return -1;
        }
    }
    return 0;
}

/* -------------------------------------------------------------------------
 * Initialize storage
 * Author: jayis1
 * ------------------------------------------------------------------------- */
int storage_init(void)
{
    /* Verify NVMC is accessible */
    if (NVMC->READY == 0) {
        return -1;
    }
    return 0;
}

/* -------------------------------------------------------------------------
 * Load calibration from flash
 * Author: jayis1
 * ------------------------------------------------------------------------- */
int storage_load_calibration(calibration_t *calib)
{
    uint32_t addr = CALIBRATION_FLASH_PAGE * FLASH_PAGE_SIZE;
    const calibration_t *stored = (const calibration_t *)addr;

    /* Check magic number */
    if (stored->magic != CALIBRATION_MAGIC) {
        return -1;
    }

    /* Verify CRC (exclude the CRC field itself) */
    uint32_t expected_crc = stored->crc32;
    uint32_t computed_crc = storage_crc32(stored,
                                           offsetof(calibration_t, crc32));
    if (expected_crc != computed_crc) {
        return -2;
    }

    /* Copy to RAM */
    memcpy(calib, stored, sizeof(calibration_t));
    return 0;
}

/* -------------------------------------------------------------------------
 * Save calibration to flash
 * Author: jayis1
 * ------------------------------------------------------------------------- */
int storage_save_calibration(const calibration_t *calib)
{
    /* Compute CRC */
    calibration_t calib_copy = *calib;
    calib_copy.crc32 = storage_crc32(&calib_copy, offsetof(calibration_t, crc32));

    /* Erase the calibration page */
    if (storage_page_erase(CALIBRATION_FLASH_PAGE) != 0)
        return -1;

    /* Write the calibration data */
    if (storage_page_write(CALIBRATION_FLASH_PAGE, &calib_copy,
                            sizeof(calibration_t)) != 0)
        return -2;

    /* Verify */
    uint32_t addr = CALIBRATION_FLASH_PAGE * FLASH_PAGE_SIZE;
    if (storage_verify(addr, &calib_copy, sizeof(calibration_t)) != 0)
        return -3;

    return 0;
}

/* -------------------------------------------------------------------------
 * Load mapping from flash
 * Author: jayis1
 * ------------------------------------------------------------------------- */
int storage_load_mapping(mapping_t *mapping)
{
    uint32_t addr = MAPPING_FLASH_PAGE * FLASH_PAGE_SIZE;
    const mapping_t *stored = (const mapping_t *)addr;

    if (stored->magic != MAPPING_MAGIC) {
        return -1;
    }

    uint32_t expected_crc = stored->crc32;
    uint32_t computed_crc = storage_crc32(stored, offsetof(mapping_t, crc32));
    if (expected_crc != computed_crc) {
        return -2;
    }

    memcpy(mapping, stored, sizeof(mapping_t));
    return 0;
}

/* -------------------------------------------------------------------------
 * Save mapping to flash
 * Author: jayis1
 * ------------------------------------------------------------------------- */
int storage_save_mapping(const mapping_t *mapping)
{
    mapping_t mapping_copy = *mapping;
    mapping_copy.crc32 = storage_crc32(&mapping_copy, offsetof(mapping_t, crc32));

    if (storage_page_erase(MAPPING_FLASH_PAGE) != 0)
        return -1;

    if (storage_page_write(MAPPING_FLASH_PAGE, &mapping_copy,
                            sizeof(mapping_t)) != 0)
        return -2;

    uint32_t addr = MAPPING_FLASH_PAGE * FLASH_PAGE_SIZE;
    if (storage_verify(addr, &mapping_copy, sizeof(mapping_t)) != 0)
        return -3;

    return 0;
}

/* -------------------------------------------------------------------------
 * Erase all stored configuration (factory reset)
 * Author: jayis1
 * ------------------------------------------------------------------------- */
int storage_erase_all(void)
{
    if (storage_page_erase(CALIBRATION_FLASH_PAGE) != 0)
        return -1;
    if (storage_page_erase(MAPPING_FLASH_PAGE) != 0)
        return -2;
    return 0;
}

/*
 * Author: jayis1
 * End of storage.c
 */