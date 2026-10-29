/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef RA_BOARD_H
#define RA_BOARD_H
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#define CORE_HZ 16000000u
#define SERIAL_BAUD 115200u
#define SERIAL_BUFFER 512u
/* J1.1: CN6.4 3V3; J1.2: CN6.6 GND.
 * J2.1..4: CN8.1..4, PA0/PA1/PA4/PB0.
 * J3=JAW_A J4=SHEET_A J5=JAW_B J6=SHEET_B; pin 2 always GND.
 * Voltage-free contacts only. USART2 PA2 AF7 to ST-LINK VCP. */
bool board_clock_reset(void);
bool board_inputs_configured(void);
void board_init(void);
uint32_t board_millis(void);
uint8_t board_closed(void);
bool board_watchdog_start(void);
void board_watchdog_feed(void);
bool serial_submit(const char *data, size_t size);
void serial_poll(void);
bool serial_busy(void);
bool serial_take_timeout(void);
#endif
