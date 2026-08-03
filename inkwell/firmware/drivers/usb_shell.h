/*
 * usb_shell.h — USB CDC-ACM shell for calibration and DFU
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */
#ifndef INKWELL_DRIVERS_USB_SHELL_H
#define INKWELL_DRIVERS_USB_SHELL_H

#include <stdint.h>

void usb_shell_init(void);
void usb_shell_poll(void);

/* Exposed for main.c / calibration routines. */
void usb_shell_set_thresholds(uint16_t down_mN, uint16_t up_mN);

#endif