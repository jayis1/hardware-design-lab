/*
 * sdlog.h — SD Card Logging (header)
 * RootTrace — Open-Source Electrical Impedance Tomography Root Imaging System
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 */

#ifndef ROOTTRACE_SDLOG_H
#define ROOTTRACE_SDLOG_H

#include <stdint.h>
#include "eit_acq.h"

int sdlog_init(void);
int sdlog_log_frame(const eit_frame_t *frame);
int sdlog_log_snapshot(const eit_frame_t *frame);
int sdlog_get_file_count(int *count);
int sdlog_read_file(int index, uint8_t *buffer, int max_size);
int sdlog_erase_all(void);
int sdlog_is_present(void);

#endif /* ROOTTRACE_SDLOG_H */