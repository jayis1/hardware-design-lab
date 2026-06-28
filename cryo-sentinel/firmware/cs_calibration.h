/*
 * cs_calibration.h — 4-point capacitive level calibration.
 *
 * Author: jayis1
 * License: MIT
 */
#ifndef CRYO_SENTINEL_CS_CALIBRATION_H
#define CRYO_SENTINEL_CS_CALIBRATION_H

#include <stdint.h>
#include <stdbool.h>

#define CS_CAL_POINTS    4
#define CS_CAL_SERIAL_LEN 24

typedef struct {
    uint32_t magic;                 /* CS_FRAM_MAGIC */
    uint32_t dewar_serial;          /* numeric, or hash of string */
    char     dewar_label[CS_CAL_SERIAL_LEN];
    uint32_t installed_epoch_min;
    uint32_t reserved;
    /* Four (raw, pct) anchor pairs. pct = {0, 25, 75, 100} by convention. */
    uint32_t raw[CS_CAL_POINTS];
    uint8_t  pct[CS_CAL_POINTS];
    uint16_t crc;
} cs_calibration_t;

/* Load calibration from FRAM. Returns 0 if valid, nonzero if absent/corrupt. */
int cs_calibration_load(cs_calibration_t *out);

/* Save a new calibration to FRAM (overwrites). */
int cs_calibration_save(const cs_calibration_t *c);

/* Capture a single anchor point during commissioning.
 * point_idx is 0..3; reads the current raw FDC code and stores it. */
int cs_calibration_capture(int point_idx, uint32_t now_min);

/* Convert a raw FDC code to a level percentage using the stored map. */
float cs_calibration_to_pct(uint32_t raw);

/* True if a valid calibration is present. */
bool cs_calibration_present(void);

#endif /* CRYO_SENTINEL_CS_CALIBRATION_H */