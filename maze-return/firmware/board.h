/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef MAZE_BOARD_H
#define MAZE_BOARD_H
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#define CORE_HZ 16000000u
#define SERIAL_BAUD 115200u
#define SERIAL_BUFFER 768u
/* External NUCLEO-F401RE MB1136. J1: CN6.4 3V3 / CN6.6 GND.
 * J2: CN8.1..4 PA0/PA1/PA4/PB0. J3..6: checkpoint A, B, C, D.
 * Every contact connector: pin1 signal, pin2 GND; dry contacts only.
 * USART2 TX PA2 AF7 to ST-LINK VCP. No command receiver. */
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
