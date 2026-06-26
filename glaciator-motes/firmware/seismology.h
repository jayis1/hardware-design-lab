/*
 * seismology.h — STA/LTA trigger, one-bit cross-correlation, event classifier
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#ifndef SEISMOLOGY_H
#define SEISMOLOGY_H

#include <stdint.h>
#include <stdbool.h>
#include "adc_driver.h"
#include "mems_driver.h"

/* Event types detected by the template library */
typedef enum {
    EVT_NONE = 0,
    EVT_BASAL_SLIDE,
    EVT_CREVASSE_SNAP,
    EVT_CALVING_THUMP,
    EVT_SUBGLACIAL_TREMOR,
    EVT_ICEQUAKE_LOCAL,
    EVT_NOISE,
    EVT_UNKNOWN
} event_type_t;

typedef struct {
    uint32_t ts_utc_ms;       /* UTC time of trigger (disciplined) */
    event_type_t type;
    float    sta_lta_ratio[3];/* per-component ratio */
    float    magnitude_est;   /* rough Mv estimate */
    float    correl_score;    /* best template match score [0..1] */
    uint8_t  template_id;     /* which template matched best */
    int32_t  sample_count;    /* total samples in the sealed window */
} event_meta_t;

/* One-bit template (sign bits, packed 8-per-byte) */
typedef struct {
    int8_t   sign[EVENT_TEMPLATE_LEN]; /* +1 or -1 */
    event_type_t type;
    const char *name;
} template_t;

void     seismo_init(void);
bool     seismo_process_frame(adc_frame_t *f);  /* returns true if triggered */
event_meta_t *seismo_last_event(void);
void     seismo_add_template(const template_t *t);
void     seismo_set_threshold(float sta_lta);
float    seismo_get_threshold(void);
void     seismo_seal_event(uint8_t *out_buf, uint32_t *out_len, uint32_t max_len);
uint8_t  seismo_classify(const int8_t *sign_stream, uint16_t n);

/* Helper: convert a frame of triaxial ADC samples to one-bit sign stream */
void     seismo_frame_to_signbits(adc_frame_t *f, int8_t *out_z,
                                  int8_t *out_ns, int8_t *out_ew);

#endif /* SEISMOLOGY_H */