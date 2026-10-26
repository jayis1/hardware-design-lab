/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef CREEP_BOARD_H
#define CREEP_BOARD_H
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#define BOARD_MCU "STM32F401RET6"
#define BOARD_ASSEMBLY "NUCLEO-F401RE"
#define CORE_HZ 16000000u
#define SERIAL_BAUD 115200u
#define SERIAL_BUFFER 512u
/* J1.1=CN6.4 3V3; J1.2=CN6.6 GND. J2.1..4=CN8.1..4.
 * ADC1 channels 0,1,4,8: PA0,PA1,PA4,PB0, specimen/reference/load A/load B.
 * Each 100k pullup is on the ADC side of a 1k sensor series resistor.
 * Nucleo USART2 PA2 AF7 reaches ST-LINK virtual COM. TX only. */
bool board_clock_reset(void);
void board_init(void);
uint32_t board_millis(void);
bool board_watchdog_start(void);
void board_watchdog_feed(void);
bool adc_init(void);
bool adc_read(uint16_t output[4]);
bool serial_submit(const char *data,size_t size);
void serial_poll(void);
bool serial_busy(void);
bool serial_take_timeout(void);
#endif
