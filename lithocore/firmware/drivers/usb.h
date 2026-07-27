/*
 * usb.h — USB-C CDC serial + DFU support
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#ifndef LITHOCORE_USB_H
#define LITHOCORE_USB_H

#include <stdint.h>
#include "soh.h"

int  usb_init(void);
void usb_service(void);
int  usb_send_result(const soh_result_t *result);
int  usb_send_csv(const soh_result_t *result);
void usb_send_string(const char *str);

/* DFU jump — enters the STM32 system bootloader for firmware updates */
void usb_enter_dfu(void);

#endif /* LITHOCORE_USB_H */