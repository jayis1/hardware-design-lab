/*
 * haptic.h — DRV2605L haptic driver for fingertip LRA actuators.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#ifndef SYNTHAND_HAPTIC_H
#define SYNTHAND_HAPTIC_H

#include <stdint.h>
#include "board.h"

/* Initialize I²C bus and all 5 DRV2605L chips.
 * Configures each chip for LRA mode, default library, and auto-calibration.
 * Returns 0 on success, nonzero on error. */
int haptic_init(void);

/* Enable/disable the haptic power gate (LRA supply).
 * When disabled, all DRV2605L chips enter standby. */
void haptic_enable(int enable);

/* Trigger a haptic waveform on a specific finger's LRA.
 * waveform_id: DRV2605L waveform library index (1-123).
 * finger: 0-4 (thumb, index, middle, ring, pinky).
 * Returns 0 on success, nonzero on I²C error. */
int haptic_trigger(uint8_t finger, uint8_t waveform_id);

/* Set the real-time playback mode (for continuous vibration).
 * amplitude: 0-127 (0 = stop, 127 = max).
 * Returns 0 on success. */
int haptic_set_rtp(uint8_t finger, uint8_t amplitude);

/* Stop any active vibration on all fingers. */
void haptic_stop_all(void);

/* DRV2605L register addresses */
#define DRV_REG_STATUS      0x00
#define DRV_REG_MODE        0x01
#define DRV_REG_RTP_INPUT   0x02
#define DRV_REG_LIBRARY     0x03
#define DRV_REG_WAVESEQ1    0x04
#define DRV_REG_WAVESEQ2    0x05
#define DRV_REG_GO          0x0C
#define DRV_REG_OVERDRIVE   0x0D
#define DRV_REG_SUSTAINPOS  0x0E
#define DRV_REG_SUSTAINNEG  0x0F
#define DRV_REG_BREAK       0x10
#define DRV_REG_AUDIOMAX    0x11
#define DRV_REG_RATEDVOLT   0x16
#define DRV_REG_OVERDRIVECLAMP 0x17
#define DRV_REG_FEEDBACK    0x1A
#define DRV_REG_CONTROL1    0x1B
#define DRV_REG_CONTROL2    0x1C
#define DRV_REG_CONTROL3    0x1D

/* Mode register values */
#define DRV_MODE_INT_TRIG   0x00  /* internal trigger */
#define DRV_MODE_DIAG       0x06  /* diagnostic mode */
#define DRV_MODE_CALIBRATE  0x07  /* auto-calibration */
#define DRV_MODE_RTP        0x05  /* real-time playback */

/* Feedback register: LRA mode, brake factor, loop gain */
#define DRV_FEEDBACK_LRA    (1U << 7)

/* Library selection: LRA library */
#define DRV_LIBRARY_LRA     0x06

#endif /* SYNTHAND_HAPTIC_H */