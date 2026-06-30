/*
 * drivers/storage.h — FRAM and SPI flash storage for event logging
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 */

#ifndef STORAGE_H
#define STORAGE_H

#include <stdint.h>
#include "../board.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FRAM_SIZE_BYTES   (256 * 1024)  /* 2 Mbit = 256 KB */
#define FLASH_SIZE_BYTES  (16 * 1024 * 1024) /* 128 Mbit = 16 MB */
#define FLASH_PAGE_SIZE   256

#define FRAM_EVENT_MAGIC  0x53504554  /* "SPET" */

int  storage_init(void);

/* FRAM event log (circular buffer) */
int  fram_write_event(const detection_event_t *event);
int  fram_read_event(uint32_t index, detection_event_t *out);
uint32_t fram_get_event_count(void);
void fram_clear_events(void);

/* SPI flash image storage */
int  flash_write_image(uint32_t addr, const uint8_t *data, uint32_t len);
int  flash_read_image(uint32_t addr, uint8_t *data, uint32_t len);
int  flash_erase_sector(uint32_t addr);

/* Low-level SPI operations */
int  fram_write(uint32_t addr, const uint8_t *data, uint32_t len);
int  fram_read(uint32_t addr, uint8_t *data, uint32_t len);
int  flash_write_enable(void);
int  flash_wait_busy(void);

#ifdef __cplusplus
}
#endif

#endif /* STORAGE_H */