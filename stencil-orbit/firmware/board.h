/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef ATLAS_BOARD_H
#define ATLAS_BOARD_H
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#define CORE_HZ 16000000u
#define SERIAL_BAUD 115200u
#define SERIAL_BUFFER 256u
/* J1: VCC, GND. J2: SENSE0/PA0, SENSE1/PA1, SENSE2/PA4, SENSE3/PB0.
 * J3..J6: CONTACT0..CONTACT3, GND. NUCLEO-F401RE MB1136 A0..A3.
 * External 10k pulls to VCC, 1k series resistance to each dry contact. */
void board_init(void);
uint32_t board_reset_cause(void);
uint32_t board_millis(void);
uint8_t board_closed(void);
bool board_watchdog_start(void);
void board_watchdog_feed(void);
bool serial_submit(const char *data, size_t size);
void serial_poll(void);
bool serial_busy(void);
bool serial_take_timeout(void);
#endif
