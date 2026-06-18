/*
 * sd_fatfs.c - microSD FATFS glue for Pollen Scout (SDMMC1 4-bit)
 * Minimal block-write layer: appends CSV rows to /log/scout.csv and
 * writes raw ROI images to /img/IMG_xxxxxx.bin in a rotating ring.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 */
#include "sd_fatfs.h"
#include "board.h"
#include "registers.h"
#include <string.h>

/* FATFS types are stubbed here for compile-ability; in production these
 * come from FatFs R0.15+. We model the API the upper layers expect. */

static int s_mounted = 0;
static uint32_t s_img_counter = 0;
static uint32_t s_csv_offset  = 0;

int sd_fatfs_init(void)
{
    /* Power on SDMMC1 */
    SDMMC_POWER = 0x03;  /* ON */
    for (volatile int i = 0; i < 10000; i++) { }
    /* Clock 400 kHz for init */
    SDMMC_CLKCR = (120000000U / 400000U) | (0U << 0);
    /* Send CMD0 (GO_IDLE) + CMD8 + ACMD41 + CMD2 + CMD3 ... (simplified) */
    SDMMC_CMD = (0U << 0) | (1U << 31);   /* CMD0 with CPSMEN */
    for (volatile int i = 0; i < 100000; i++) { }
    /* We assume the card is present and FAT32; in a real build the
     * FatFs f_mount() is called here. */
    s_mounted = 1;
    return 0;
}

int sd_fatfs_append_csv(const char *line, int len)
{
    if (!s_mounted) return -1;
    /* In a production build: open /log/scout.csv in append mode, write
     * `len` bytes from `line`, close. Here we model the call surface. */
    (void)line;
    s_csv_offset += len;
    return 0;
}

int sd_fatfs_write_image(const uint8_t *pixels, int len, const char *tag)
{
    if (!s_mounted) return -1;
    /* Filename: /img/IMG_xxxxxx_<tag>.bin */
    (void)pixels; (void)len; (void)tag;
    s_img_counter++;
    return 0;
}

int sd_fatfs_sync(void)
{
    /* Flush any pending FATFS buffers */
    return 0;
}

uint32_t sd_fatfs_image_count(void)  { return s_img_counter; }
uint32_t sd_fatfs_csv_bytes(void)    { return s_csv_offset; }