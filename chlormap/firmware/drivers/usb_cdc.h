/*
 * usb_cdc.h — USB CDC virtual serial port interface
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#ifndef DRIVERS_USB_CDC_H
#define DRIVERS_USB_CDC_H

#include <stdint.h>
#include <stdbool.h>

/* Initialize USB CDC */
bool usb_cdc_init(void);

/* Send string over CDC */
void usb_cdc_send(const char *str);

/* Check if a complete command line has been received */
bool usb_cdc_has_command(void);

/* Get the received command line */
void usb_cdc_get_command(char *buf, uint16_t max_len);

/* Check if CDC host is connected (DTR active) */
bool usb_cdc_is_connected(void);

/* USB CDC poll (call from main loop for IRQ-driven RX) */
void usb_cdc_poll(void);

#endif /* DRIVERS_USB_CDC_H */