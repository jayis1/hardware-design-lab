/*
 * mux.h — 16-Channel TX/RX Multiplexer Driver
 *
 * LignoScan — Portable Acoustic Tomography Scanner
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#ifndef LIGNOSCAN_MUX_H
#define LIGNOSCAN_MUX_H

#include <stdint.h>

#define MUX_CHANNELS 16

void mux_init(void);
void mux_select_tx(int channel);
void mux_select_rx(int channel);
void mux_disable_tx(void);
void mux_disable_rx(void);
int mux_detect_channels(void);

#endif /* LIGNOSCAN_MUX_H */