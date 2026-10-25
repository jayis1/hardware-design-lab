/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef RC_BOARD_H
#define RC_BOARD_H
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#define CORE_HZ 16000000u
#define SERIAL_BAUD 115200u
#define SERIAL_BUFFER 640u
/* NUCLEO-F401RE: J2 pins 1..4 = A0/PA0, A1/PA1, A2/PA4, A3/PB0.
 * J3 inlet, J4 left, J5 middle, J6 right; pin 1 signal, pin 2 ground.
 * Adafruit 2167 open-collector receivers: low means blocked.
 * Sensor power supplied externally at 5V; share ground, not the 5V rail. */
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
