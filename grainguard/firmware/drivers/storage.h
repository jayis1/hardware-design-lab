/*
 * storage.h — W25R80 SPI flash data logging (header)
 * Author: jayis1  Copyright (C) 2026 jayis1  License: GPL-2.0
 */
#ifndef GRAINGUARD_STORAGE_H
#define GRAINGUARD_STORAGE_H

#include <stdint.h>
#include <stdbool.h>
#include "co2.h"
#include "temp.h"
#include "humid.h"
#include "acoustic.h"
#include "sri.h"

/* Log record (32 bytes) */
typedef struct __attribute__((packed)) {
    uint32_t timestamp_sec;   /* UTC epoch seconds */
    uint16_t co2_ppm;
    int16_t  tmax_x10;         /* ×10 C */
    int16_t  tmin_x10;
    int16_t  tdelta_x10;
    uint8_t  rh_pct;
    uint16_t emc_x1000;        /* ×1000 % MC */
    uint8_t  sri;
    uint8_t  ae_events_per_min;
    uint8_t  insect_id;
    uint8_t  grain_type;
    uint16_t battery_mv;
    uint8_t  reserved[10];
} log_record_t;

/* Initialize the flash (check JEDEC ID, erase if first boot). */
int  storage_init(void);

/* Append a log record to the ring buffer. Returns 0 on success. */
int  storage_append(const log_record_t *rec);

/* Read N records starting at index. */
int  storage_read(uint32_t index, log_record_t *out, uint16_t count);

/* Get total record count in the ring buffer. */
uint32_t storage_get_count(void);

/* Erase the entire log (sector erase). */
int  storage_erase_all(void);

#endif /* GRAINGUARD_STORAGE_H */