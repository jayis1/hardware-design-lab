/*
 * display.h — StudGuard local UI abstraction
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#ifndef STUDGUARD_DISPLAY_H
#define STUDGUARD_DISPLAY_H

#include "../board.h"

void display_init(void);
void display_render_status(const sg_device_status_t *status, const sg_measurement_t *measurement);
void display_render_banner(const char *text);

#endif
