/*
 * dsp.h — DSP pipeline public interface
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 */
#ifndef SOUNDLOOM_DSP_H
#define SOUNDLOOM_DSP_H

#include <stdint.h>
#include "board.h"

typedef struct {
    uint8_t  channel;
    uint32_t timestamp;
    float    rms;
    float    features[CLS_FRAMES * CLS_MEL_BINS];  /* flattened mel spectrogram */
} dsp_event_t;

void   dsp_init(void);
void   dsp_process(const int32_t *samples, uint32_t n_ch);
int    dsp_get_event(dsp_event_t *out);
void   dsp_get_mel_spectrogram(uint8_t ch, float out[CLS_FRAMES][CLS_MEL_BINS]);
float  dsp_spectral_entropy(uint8_t ch);
void   dsp_record_class(int class_id);
void   dsp_get_event_rates(float rates[CLS_NUM_CLASSES], uint32_t window_minutes);
void   dsp_reset_counters(void);
uint32_t dsp_get_total_events(void);
void   dsp_set_sensitivity(float factor);

#endif /* SOUNDLOOM_DSP_H */