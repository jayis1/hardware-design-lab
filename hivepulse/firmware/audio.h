/*
 * audio.h — Audio capture API for HivePulse CS42448 codec
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>
#include <stdbool.h>
#include "board.h"

/* Number of microphone channels captured (4 mics, 2 interior + 2 entrance) */
#define AUDIO_CHANNELS       4

/* Samples per channel in one FFT window */
#define AUDIO_FFT_SIZE       4096

/* DMA double-buffer: two buffers of FFT_SIZE * CHANNELS samples */
#define AUDIO_BUF_SAMPLES    (AUDIO_FFT_SIZE * AUDIO_CHANNELS)
#define AUDIO_BUF_BYTES      (AUDIO_BUF_SAMPLES * sizeof(int16_t))

/* Audio snapshot: contains raw samples from the latest window */
typedef struct {
    int16_t samples[AUDIO_CHANNELS][AUDIO_FFT_SIZE]; /* De-interleaved */
    uint32_t timestamp_ms;
    uint32_t seq_number;
    bool valid;
} audio_snapshot_t;

/* Initialize audio subsystem: codec config, I2S, DMA double buffer */
int audio_init(void);

/* Start a capture session (begins DMA, codec runs in continuous mode) */
int audio_start_capture(void);

/* Stop capture (DMA halted, codec powered down to save current) */
int audio_stop_capture(void);

/* Get the latest completed audio snapshot (non-blocking)
 * Returns true if a new snapshot is available since last call */
bool audio_get_snapshot(audio_snapshot_t *snap);

/* Get current audio level (RMS) per channel — for app display */
void audio_get_levels(float levels[AUDIO_CHANNELS]);

/* DMA interrupt handler — called on half-transfer and full-transfer */
void audio_dma_irq_handler(void);

/* Codec register access (I2C) */
int codec_write_reg(uint8_t reg, uint8_t value);
int codec_read_reg(uint8_t reg, uint8_t *value);

/* Set microphone gain (0-100%) */
void audio_set_gain(uint8_t gain_pct);

#endif /* AUDIO_H */