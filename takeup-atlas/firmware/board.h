/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef TKA_BOARD_H
#define TKA_BOARD_H
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#define CORE_HZ 16000000u
#define SERIAL_BAUD 115200u
#define SERIAL_BUFFER 256u
/* NUCLEO-F401RE A0=PA0, A1=PA1, A2=PA4, A3=PB0.
 * J1 3V3/GND; J2 input A/B, output A/B.
 * J3..J6 NPN open-collector signals and GND; never supply voltage.
 * Encoders use a separate current-limited 5V supply with common ground. */
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
