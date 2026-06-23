/*
 * dsp.h — DSP pipeline header for MycoMesh
 * MycoMesh — Open-Source Fungal Mycelium Electrophysiology & Chemical Sensing Network
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef MYCOMESH_DSP_H
#define MYCOMESH_DSP_H

#include "board.h"
#include <stdint.h>

/* Initialize the DSP pipeline. */
void dsp_init(uint8_t mains_freq_hz, uint16_t sample_rate_hz);

/* Process one block of samples for a single channel:
 *   - High-pass filter (0.5 Hz)
 *   - Mains notch filter (50/60 Hz)
 *   - Low-pass filter (5 kHz)
 * Output goes to filtered_buf. */
void dsp_process_channel(uint8_t channel,
                         const int16_t *input,
                         int16_t *filtered,
                         uint16_t n_samples,
                         uint8_t mains_freq_hz);

/* Detect spikes in a filtered channel buffer using adaptive threshold.
 * Detected spikes are written to the spikes array (max max_spikes).
 * Returns the number of spikes detected. */
uint16_t dsp_detect_spikes(uint8_t channel,
                           const int16_t *filtered,
                           uint16_t n_samples,
                           spike_event_t *spikes,
                           uint16_t max_spikes,
                           uint32_t timestamp_ms);

/* Compute cross-channel propagation events.
 * Returns the number of propagation events detected. */
uint16_t dsp_compute_propagation(int16_t filtered[ADS1298_TOTAL_CHANS][DSP_BLOCK_SIZE],
                                 uint8_t n_channels,
                                 uint16_t n_samples,
                                 uint16_t window_ms,
                                 float corr_threshold);

/* Update the notch filter frequency (50 or 60 Hz). */
void dsp_set_notch_freq(uint8_t mains_hz);

/* Get the current noise floor (MAD) estimate for a channel. */
float dsp_get_noise_mad(uint8_t channel);

#endif /* MYCOMESH_DSP_H */