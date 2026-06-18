/*
 * sd_fatfs.h - microSD FATFS glue header
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 */
#ifndef SD_FATFS_H
#define SD_FATFS_H
#include <stdint.h>

int      sd_fatfs_init(void);
int      sd_fatfs_append_csv(const char *line, int len);
int      sd_fatfs_write_image(const uint8_t *pixels, int len, const char *tag);
int      sd_fatfs_sync(void);
uint32_t sd_fatfs_image_count(void);
uint32_t sd_fatfs_csv_bytes(void);

#endif /* SD_FATFS_H */