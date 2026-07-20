/*
 * eit_acq.h — EIT Frame Acquisition (header)
 * RootTrace — Open-Source Electrical Impedance Tomography Root Imaging System
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 */

#ifndef ROOTTRACE_EIT_ACQ_H
#define ROOTTRACE_EIT_ACQ_H

#include <stdint.h>
#include "ad5940.h"
#include "mux.h"

/* EIT frame: 208 measurements (16 stim × 13 meas per stim) */
typedef struct {
    uint32_t timestamp_ms;          /* Frame timestamp */
    uint16_t frame_seq;            /* Frame sequence number */
    uint8_t  freq_index;           /* Excitation frequency index */
    ad5940_meas_t meas[EIT_FRAME_SIZE]; /* All 208 measurements */
    float    conductivity[EIT_MESH_NODES]; /* Reconstructed conductivity */
    uint8_t  status;               /* 0=ok, nonzero=error */
    uint8_t  electrode_contacts;   /* bitmask of good contacts (16 bits -> 2 bytes) */
    uint16_t electrode_contact_mask;
} eit_frame_t;

void eit_acq_init(void);
int  eit_acq_capture_frame(eit_frame_t *frame, uint8_t freq_index);
int  eit_acq_check_all_contacts(uint16_t *contact_mask);
void eit_acq_calibrate(void);

/* Adjacent stimulation/measurement pattern generation */
void eit_acq_get_stim_pattern(uint8_t stim_index, mux_config_t *cfg);
void eit_acq_get_meas_pattern(uint8_t stim_index, uint8_t meas_index,
                                mux_config_t *cfg);

#endif /* ROOTTRACE_EIT_ACQ_H */