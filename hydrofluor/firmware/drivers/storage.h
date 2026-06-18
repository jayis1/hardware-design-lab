/*
 * storage.h — HydroFluor microSD logging + QSPI calibration store
 * Author: jayis1
 * License: MIT
 */
#ifndef STORAGE_H
#define STORAGE_H

#include "board.h"
#include "fluorometry.h"
#include <stdint.h>

/* Initialize SDMMC1 (4-bit) + FATFS, and QSPI flash. */
int storage_init(void);

/* Log a sample record to the rotating CSV file on SD.
 * record is the 24-byte binary format from the BLE spec, expanded here. */
typedef struct {
    uint16_t seq;
    uint32_t timestamp;
    uint16_t depth_cm;
    int16_t  temp_c01;
    uint16_t battery_mv;
    uint16_t cdom_ppb;
    uint16_t chla_ugl;
    uint16_t phyc_ugl;
    uint16_t oil_ppb;
    uint16_t turb_ntu_x100;
    uint16_t flags;
} sample_record_t;

int storage_log_sample(const sample_record_t *rec);

/* Also write the raw 24-feature fluorescence vector (for post-hoc analysis). */
int storage_log_raw(const int16_t feat[PLS_FEATURES], uint16_t seq);

/* Save / load PLS model to QSPI flash (calibration persistence). */
int storage_save_model(analyte_t a, const pls_model_t *m);
int storage_load_model(analyte_t a, pls_model_t *m);

/* Get the next sample sequence number (monotonic counter). */
uint16_t storage_next_seq(void);

#endif /* STORAGE_H */