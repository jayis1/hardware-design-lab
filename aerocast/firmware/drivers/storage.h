/*
 * storage.h — AeroCast storage API
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 *
 * The minute_bin_t struct is defined here (mirrors main.c's private
 * bin) so the storage driver can write CSV rows without depending on
 * main.c internals.
 */
#ifndef AEROCAST_STORAGE_H
#define AEROCAST_STORAGE_H

#include <stdint.h>
#include "board.h"
#include "classify.h"   /* ref_row_t */

typedef struct {
    uint32_t epoch_min;
    uint32_t counts[N_CLASSES];
    float    fsc_p50, ssc_p50, flb_p50, fla_p50;
    float    temperature, humidity, pressure;
    float    flow_lpm, wind_dir, wind_speed;
} minute_bin_t;

void     storage_init(void);
void     storage_append_log(const char *line);
void     storage_write_bin_csv(const minute_bin_t *b);
int      storage_save_calib(const uint8_t *buf, uint16_t len);
int      storage_load_calib(ref_row_t *buf, uint16_t max_n);
uint32_t storage_dump(uint32_t offset, uint32_t max_bytes, uint8_t *out);
uint32_t storage_size(void);

#endif /* AEROCAST_STORAGE_H */