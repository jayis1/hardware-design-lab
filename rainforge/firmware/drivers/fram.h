/*
 * fram.h — CY15B104Q 4 Mbit FRAM driver (SPI)
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 */
#ifndef RAINFORGE_FRAM_H
#define RAINFORGE_FRAM_H

#include <stdint.h>
#include "piezo.h"
#include "disdrometer.h"

/* ---- API ---- */
void  fram_init(void);
int   fram_read_id(uint8_t *id_out);   /* 9-byte RDID check */
void  fram_sleep(void);
void  fram_wake(void);

/* Byte-level read/write (with WREN) */
int   fram_read(uint32_t addr, uint8_t *buf, uint16_t len);
int   fram_write(uint32_t addr, const uint8_t *buf, uint16_t len);

/* High-level: calibration storage */
int   fram_load_calibration(calibration_t *cal);
int   fram_save_calibration(const calibration_t *cal);

/* High-level: statistics block */
int   fram_load_stats(dsd_t *dsd);
int   fram_save_stats(const dsd_t *dsd);

/* High-level: event ring buffer */
int   fram_log_event(const droplet_feature_t *feat);
int   fram_read_events(uint32_t start_idx, droplet_feature_t *out, uint16_t max);
uint32_t fram_event_count(void);
void  fram_clear_events(void);

/* High-level: config block */
int   fram_load_config(uint8_t *buf, uint16_t len);
int   fram_save_config(const uint8_t *buf, uint16_t len);

#endif /* RAINFORGE_FRAM_H */