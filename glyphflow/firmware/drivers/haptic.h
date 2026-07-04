/*
 * haptic.h — DRV2605L haptic driver
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 */
#ifndef GLYPHFLOW_HAPTIC_H
#define GLYPHFLOW_HAPTIC_H

#include <stdint.h>

int haptic_init(void);
int    haptic_play(uint8_t waveform_id);   /* DRV2605L library waveform 0-123 */
int haptic_tick(void);                    /* short click */
int haptic_confirm(void);                 /* longer buzz */
int haptic_error(void);                   /* double buzz */

#endif /* GLYPHFLOW_HAPTIC_H */