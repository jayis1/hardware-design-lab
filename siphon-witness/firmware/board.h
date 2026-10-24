/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef SW_BOARD_H
#define SW_BOARD_H
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#define CORE_HZ 16000000u
#define SERIAL_BAUD 115200u
#define SERIAL_BUFFER 256u
/* NUCLEO-F401RE A0=PA0,A1=PA1,A2=PA4,A3=PB0 to J2 pins 1..4.
 * J3 SOURCE_LOW, J4 SOURCE_HIGH, J5 RECEIVER_LOW, J6 RECEIVER_HIGH.
 * Each field connector: pin1 dry reed, pin2 GND. No external voltage.
 * PA2 AF7 USART2 TX to ST-LINK VCP. PA5 onboard LD2. */
void board_init(void);
uint32_t board_millis(void);
uint8_t board_closed(void);
void board_led(bool on);
bool board_watchdog_start(void);
void board_watchdog_feed(void);
bool serial_submit(const char *data, size_t size);
void serial_poll(void);
bool serial_busy(void);
bool serial_take_timeout(void);
#endif
