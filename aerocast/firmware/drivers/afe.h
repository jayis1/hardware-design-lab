/*
 * afe.h — AeroCast analog front-end & event extractor API
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 */
#ifndef AEROCAST_AFE_H
#define AEROCAST_AFE_H

#include <stdint.h>

typedef struct {
    uint32_t timestamp_us;
    uint32_t fsc_area;     /* forward-scatter integrated area */
    uint32_t ssc_area;     /* side-scatter integrated area */
    uint32_t fsc_peak;
    uint32_t ssc_peak;
    uint32_t fl_blue;      /* 420–470 nm fluorescence peak */
    uint32_t fl_amber;     /* 520–580 nm fluorescence peak */
} particle_event_t;

void     afe_init(void);
void     afe_pump(void);
int      afe_pop_event(particle_event_t *out);
void     afe_run_blank(uint32_t seconds);

float    afe_baseline(int ch);
float    afe_noise(int ch);
float    afe_threshold(void);
uint32_t afe_events_pending(void);

#endif /* AEROCAST_AFE_H */