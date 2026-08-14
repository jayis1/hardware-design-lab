/*
 * drivers/mux.h — TMUX1108 analog multiplexer driver
 *
 * Drives the 3 address lines (A0/A1/A2) and active-low enable of the
 * TMUX1108 to select one of 8 Hall-effect sensor channels for the ADC.
 *
 * Author:  jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */
#ifndef MUSSEL_MUX_H
#define MUSSEL_MUX_H

#include <stdint.h>

void mux_init(void);
void mux_select(uint8_t channel);
void mux_disable(void);

#endif