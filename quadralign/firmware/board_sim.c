/* SPDX-License-Identifier: MIT
 * QuadraAlign host simulation board support
 * Author: jayis1
 */
#include "board.h"
#include "registers.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

static uint32_t tick_ms;
static uint8_t flash_page[512];
static bool flash_valid;

void board_init(void) { tick_ms = 0U; memset(flash_page, 0xFF, sizeof flash_page); }
uint32_t board_millis(void) { return tick_ms++; }
void board_delay_ms(uint32_t ms) { tick_ms += ms; }
void board_gpio_write(board_pin_t pin, bool value) { (void)pin; (void)value; }
bool board_gpio_read(board_pin_t pin) { (void)pin; return false; }
void board_led_set(led_color_t color, uint8_t brightness) { (void)color; (void)brightness; }
uint16_t board_battery_mv(void) { return 4012U; }
void board_watchdog_kick(void) { }
void board_power_off(void) { }
int board_uart_write(const uint8_t *data, size_t length) {
    if (!data || length < 5U || data[0] != BLE_SOF) return -1;
    return 0;
}
int board_uart_read(uint8_t *data, size_t capacity) { (void)data; (void)capacity; return 0; }
int board_i2c_write(uint8_t address, uint8_t reg, const uint8_t *data, size_t length) {
    (void)reg; return address == LIS2DW12_ADDRESS && data && length ? 0 : -1;
}
int board_i2c_read(uint8_t address, uint8_t reg, uint8_t *data, size_t length) {
    if (address != LIS2DW12_ADDRESS || !data) return -1;
    memset(data, 0, length);
    if (reg == LIS2DW12_WHO_AM_I && length) data[0] = LIS2DW12_WHO_AM_I_VALUE;
    return 0;
}
int board_flash_read(uint32_t address, void *data, size_t length) {
    (void)address;
    if (!flash_valid || !data || length > sizeof flash_page) return -1;
    memcpy(data, flash_page, length); return 0;
}
int board_flash_write(uint32_t address, const void *data, size_t length) {
    (void)address;
    if (!data || length > sizeof flash_page) return -1;
    memcpy(flash_page, data, length); flash_valid = true; return 0;
}
