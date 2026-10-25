/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef CROSSOVER_BOARD_H
#define CROSSOVER_BOARD_H
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#define CORE_HZ 16000000u
#define SERIAL_BAUD 115200u
#define SERIAL_BUFFER 512u
/* NUCLEO-F401RE A0/PA0, A1/PA1, A2/PA4, A3/PB0; active low.
 * J3 left start, J4 left finish, J5 right start, J6 right finish. */
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
