/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef NS_BOARD_H
#define NS_BOARD_H
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#define BOARD_MCU "STM32F401RET6"
#define BOARD_ASSEMBLY "NUCLEO-F401RE"
#define CORE_HZ 16000000u
#define SERIAL_BAUD 115200u
#define SERIAL_BUFFER 256u
/* J1: pin 1 Nucleo CN6.4 3V3, pin 2 CN6.6 GND.
 * J2.1..4 to CN8.1..4: PA0, PA1, PA4, PB0.
 * J3..J6: SEAT_A..SEAT_D pin 1; GND pin 2. Dry contacts only.
 * PA2 AF7 -> USART2 TX -> onboard ST-LINK VCP. No RX commands. */
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
