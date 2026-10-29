/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef PIPETTE_BOARD_H
#define PIPETTE_BOARD_H
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#define CORE_HZ 16000000u
#define SERIAL_BAUD 115200u
#define SERIAL_BUFFER 512u
/* External NUCLEO-F401RE MB1136, not on the passive carrier:
 * J1.1 CN6.4 3V3; J1.2 CN6.6 GND.
 * J2 SENSE0..3 to CN8.1..4 PA0/PA1/PA4/PB0.
 * J3 HOME, J4 FIRST, J5 SECOND, J6 SOURCE, all pin 2 GND.
 * FIRST is a threshold contact, closed also at SECOND.
 * SOURCE is an operator pedal declaration, NOT a liquid sensor.
 * Voltage-free contacts only; USART2 TX PA2 AF7 to ST-LINK VCP. */
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
