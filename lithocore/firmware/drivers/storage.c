/*
 * storage.c — Flash ring buffer for cell test results.
 *
 * Uses the last 16 flash pages (32 KB) of the STM32G474's 512 KB flash
 * to store up to 256 cell test results in a ring buffer. Each record is
 * a compact 96-byte structure containing the SoH score, degradation mode,
 * Randles parameters, OCV, DCIR, and a timestamp. The full sweep data
 * (48 impedance points) is not stored in flash — it's sent to the app
 * in real time and stored there.
 *
 * The config (litho_config_t) is stored in a separate dedicated page.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#include "storage.h"
#include "../board.h"
#include "../registers.h"
#include "ble.h"
#include <string.h>

/* -------------------------------------------------------------------------
 * Compact result record (96 bytes) — stored in flash
 * ------------------------------------------------------------------------- */
typedef struct __attribute__((packed)) {
    uint32_t magic;           /* 0x4C434F52 = "LCOR" (LithoCore Result) */
    uint32_t timestamp;       /* tick count or RTC time */
    uint8_t  soh_score;
    uint8_t  degradation;
    uint8_t  verdict;
    uint8_t  chemistry_idx;
    uint16_t ocv_mv;
    uint16_t temp_dc;
    uint16_t dcir_mohm;
    int32_t  self_discharge_uv_per_min;
    uint8_t  fit_valid;
    uint8_t  reserved[3];
    /* Randles parameters */
    int32_t  rs_mohm;
    int32_t  rsei_mohm;
    int32_t  csei_mF;
    int32_t  rct_mohm;
    int32_t  cdl_mF;
    int32_t  sigma;
    /* Key impedance points (3 representative frequencies) */
    int32_t  z10hz_re, z10hz_im;
    int32_t  z1khz_re, z1khz_im;
    int32_t  z100khz_re, z100khz_im;
    /* Padding to 96 bytes */
    uint8_t  pad[16];
} storage_record_t;

#define STORAGE_MAGIC   0x4C434F52U
#define RECORD_SIZE     sizeof(storage_record_t)

/* Compile-time check: record must be ≤ 128 bytes */
_Static_assert(sizeof(storage_record_t) <= 128, "Record too large");

/* -------------------------------------------------------------------------
 * Flash helpers
 * ------------------------------------------------------------------------- */
static void flash_unlock(void)
{
    FLASH_REG->KEYR = 0x45670123;
    FLASH_REG->KEYR = 0xCDEF89AB;
}

static void flash_lock(void)
{
    FLASH_REG->CR |= FLASH_CR_LOCK;
}

static void flash_wait_busy(void)
{
    while (FLASH_REG->SR & FLASH_SR_BSY) { }
}

static int flash_erase_page(uint16_t page_idx)
{
    flash_wait_busy();
    FLASH_REG->CR = (page_idx << FLASH_CR_PNB_SHIFT) | FLASH_CR_PER;
    FLASH_REG->CR |= FLASH_CR_STRT;
    flash_wait_busy();
    FLASH_REG->CR = 0;
    return (FLASH_REG->SR & 0xFU) ? -1 : 0;  /* check error bits */
}

static int flash_write_word(uint32_t addr, uint32_t data)
{
    flash_wait_busy();
    FLASH_REG->CR = FLASH_CR_PG;
    *(volatile uint32_t *)addr = data;
    flash_wait_busy();
    FLASH_REG->CR = 0;
    return (FLASH_REG->SR & 0xFU) ? -1 : 0;
}

/* -------------------------------------------------------------------------
 * Storage init
 *
 * Scans the storage pages to find the write head (first empty record).
 * ------------------------------------------------------------------------- */
static uint16_t g_write_index = 0;  /* next record index to write */
static uint16_t g_result_count = 0;

int storage_init(void)
{
    uint32_t base_addr = 0x08000000 + STORAGE_PAGE_START * FLASH_PAGE_SIZE;

    /* Scan for the first empty (0xFFFFFFFF magic) record */
    g_write_index = 0;
    g_result_count = 0;

    for (uint16_t i = 0; i < STORAGE_MAX_RESULTS; i++) {
        uint32_t addr = base_addr + (uint32_t)i * RECORD_SIZE;
        uint32_t magic = *(volatile uint32_t *)addr;
        if (magic == 0xFFFFFFFF) {
            g_write_index = i;
            g_result_count = i;
            break;
        }
        if (magic == STORAGE_MAGIC) {
            g_result_count = i + 1;
        }
    }

    /* If all slots are used, wrap around (ring buffer) */
    if (g_write_index >= STORAGE_MAX_RESULTS) {
        g_write_index = 0;  /* overwrite oldest */
    }

    return 0;
}

/* -------------------------------------------------------------------------
 * Save a result
 *
 * Author: jayis1
 * ------------------------------------------------------------------------- */
int storage_save_result(const soh_result_t *result)
{
    storage_record_t rec;
    memset(&rec, 0, sizeof(rec));

    rec.magic = STORAGE_MAGIC;
    rec.timestamp = g_ticks;  /* from main.c SysTick */
    rec.soh_score = result->soh_score;
    rec.degradation = (uint8_t)result->degradation;
    rec.verdict = (uint8_t)result->verdict;
    rec.chemistry_idx = result->chemistry_idx;
    rec.ocv_mv = result->ocv_mv;
    rec.temp_dc = result->temp_dc;
    rec.dcir_mohm = result->dcir_mohm;
    rec.self_discharge_uv_per_min = result->self_discharge_uv_per_min;
    rec.fit_valid = result->fit_valid;

    if (result->fit_valid) {
        rec.rs_mohm = result->randles.rs_mohm;
        rec.rsei_mohm = result->randles.rsei_mohm;
        rec.csei_mF = result->randles.csei_mF;
        rec.rct_mohm = result->randles.rct_mohm;
        rec.cdl_mF = result->randles.cdl_mF;
        rec.sigma = result->randles.sigma;
    }

    /* Extract 3 representative impedance points */
    for (uint16_t i = 0; i < result->sweep_data.num_points; i++) {
        const lockin_result_t *pt = &result->sweep_data.points[i];
        if (!pt->valid) continue;
        if (pt->freq_hz >= 9 && pt->freq_hz <= 11) {
            rec.z10hz_re = pt->re_z;
            rec.z10hz_im = pt->im_z;
        }
        if (pt->freq_hz >= 900 && pt->freq_hz <= 1100) {
            rec.z1khz_re = pt->re_z;
            rec.z1khz_im = pt->im_z;
        }
        if (pt->freq_hz >= 90000 && pt->freq_hz <= 110000) {
            rec.z100khz_re = pt->re_z;
            rec.z100khz_im = pt->im_z;
        }
    }

    /* Write to flash */
    uint32_t base_addr = 0x08000000 + STORAGE_PAGE_START * FLASH_PAGE_SIZE;
    uint32_t addr = base_addr + (uint32_t)g_write_index * RECORD_SIZE;

    /* Check if we need to erase the page first */
    if (g_write_index == 0) {
        flash_unlock();
        for (uint16_t p = 0; p < STORAGE_PAGE_COUNT; p++) {
            flash_erase_page(STORAGE_PAGE_START + p);
        }
        flash_lock();
    }

    /* Write the record as a sequence of 32-bit words */
    flash_unlock();
    uint32_t *src = (uint32_t *)&rec;
    for (uint16_t w = 0; w < RECORD_SIZE / 4; w++) {
        flash_write_word(addr + w * 4, src[w]);
    }
    flash_lock();

    g_write_index++;
    if (g_write_index >= STORAGE_MAX_RESULTS)
        g_write_index = 0;
    g_result_count++;

    return 0;
}

/* -------------------------------------------------------------------------
 * Load / save config
 * ------------------------------------------------------------------------- */
int storage_load_config(litho_config_t *config)
{
    /* Config is stored in the page before the result storage */
    uint32_t cfg_addr = 0x08000000 + (STORAGE_PAGE_START - 1) * FLASH_PAGE_SIZE;
    uint32_t magic = *(volatile uint32_t *)cfg_addr;

    if (magic == STORAGE_MAGIC) {
        memcpy(config, (void *)(cfg_addr + 4), sizeof(litho_config_t));
        return 0;
    }

    /* No stored config — use defaults (already set in main.c) */
    return -1;
}

int storage_save_config(const litho_config_t *config)
{
    uint32_t cfg_addr = 0x08000000 + (STORAGE_PAGE_START - 1) * FLASH_PAGE_SIZE;

    flash_unlock();
    flash_erase_page(STORAGE_PAGE_START - 1);
    flash_write_word(cfg_addr, STORAGE_MAGIC);
    uint32_t *src = (uint32_t *)config;
    for (uint16_t w = 0; w < sizeof(litho_config_t) / 4; w++) {
        flash_write_word(cfg_addr + 4 + w * 4, src[w]);
    }
    flash_lock();

    return 0;
}

/* -------------------------------------------------------------------------
 * Get a result by index
 * ------------------------------------------------------------------------- */
int storage_get_result(uint16_t index, soh_result_t *result)
{
    if (index >= g_result_count)
        return -1;

    uint32_t base_addr = 0x08000000 + STORAGE_PAGE_START * FLASH_PAGE_SIZE;
    uint32_t addr = base_addr + (uint32_t)index * RECORD_SIZE;
    storage_record_t *rec = (storage_record_t *)addr;

    if (rec->magic != STORAGE_MAGIC)
        return -1;

    memset(result, 0, sizeof(*result));
    result->soh_score = rec->soh_score;
    result->degradation = (degradation_mode_t)rec->degradation;
    result->verdict = (quality_verdict_t)rec->verdict;
    result->chemistry_idx = rec->chemistry_idx;
    result->ocv_mv = rec->ocv_mv;
    result->temp_dc = rec->temp_dc;
    result->dcir_mohm = rec->dcir_mohm;
    result->self_discharge_uv_per_min = rec->self_discharge_uv_per_min;
    result->fit_valid = rec->fit_valid;
    result->timestamp = rec->timestamp;

    if (rec->fit_valid) {
        result->randles.rs_mohm = rec->rs_mohm;
        result->randles.rsei_mohm = rec->rsei_mohm;
        result->randles.csei_mF = rec->csei_mF;
        result->randles.rct_mohm = rec->rct_mohm;
        result->randles.cdl_mF = rec->cdl_mF;
        result->randles.sigma = rec->sigma;
    }

    return 0;
}

int storage_get_count(uint16_t *count)
{
    *count = g_result_count;
    return 0;
}

/* -------------------------------------------------------------------------
 * Send history over BLE (compact list of stored results)
 * ------------------------------------------------------------------------- */
void storage_send_history_ble(void)
{
    uint16_t count = g_result_count;
    if (count > STORAGE_MAX_RESULTS)
        count = STORAGE_MAX_RESULTS;

    /* Send count first */
    uint8_t count_data[2];
    count_data[0] = count & 0xFF;
    count_data[1] = (count >> 8) & 0xFF;
    ble_send_notification(count_data, 2);

    /* Send each record summary */
    for (uint16_t i = 0; i < count; i++) {
        uint32_t base_addr = 0x08000000 + STORAGE_PAGE_START * FLASH_PAGE_SIZE;
        storage_record_t *rec = (storage_record_t *)
            (base_addr + (uint32_t)i * RECORD_SIZE);

        if (rec->magic != STORAGE_MAGIC) continue;

        /* Compact 8-byte summary: [soh][mode][verdict][chem] [ocv_lo][ocv_hi]
           [ts_lo][ts_hi] */
        uint8_t summary[8];
        summary[0] = rec->soh_score;
        summary[1] = rec->degradation;
        summary[2] = rec->verdict;
        summary[3] = rec->chemistry_idx;
        summary[4] = rec->ocv_mv & 0xFF;
        summary[5] = (rec->ocv_mv >> 8) & 0xFF;
        summary[6] = rec->timestamp & 0xFF;
        summary[7] = (rec->timestamp >> 8) & 0xFF;
        ble_send_notification(summary, 8);
    }
}

/* -------------------------------------------------------------------------
 * Clear all stored results
 * ------------------------------------------------------------------------- */
void storage_clear(void)
{
    flash_unlock();
    for (uint16_t p = 0; p < STORAGE_PAGE_COUNT; p++) {
        flash_erase_page(STORAGE_PAGE_START + p);
    }
    flash_lock();

    g_write_index = 0;
    g_result_count = 0;
}