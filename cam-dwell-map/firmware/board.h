/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef CAM_BOARD_H
#define CAM_BOARD_H
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#define BOARD_MCU "STM32F401RET6"
#define BOARD_ASSEMBLY "NUCLEO-F401RE"
#define CORE_HZ 16000000u
#define SERIAL_BAUD 115200u
#define SERIAL_BUFFER 256u
/* J1: 1=CN6.4 3V3, 2=CN6.6 GND. J2.1..4=CN8.1..4.
 * J3=ENC_A/PA0, J4=ENC_B/PA1, J5=INDEX/PA4, J6=FOLLOWER/PB0.
 * Each sensor connector pin 1=signal, pin 2=common ground.
 * Only NPN open collectors or voltage-free contacts; never external voltage.
 * PA2 AF7 -> USART2 TX -> ST-LINK virtual COM. No command receiver. */
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
