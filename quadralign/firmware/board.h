/* SPDX-License-Identifier: MIT
 * QuadraAlign board definition
 * Author: jayis1
 */
#ifndef QUADRALIGN_BOARD_H
#define QUADRALIGN_BOARD_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define QA_AUTHOR                 "jayis1"
#define QA_FIRMWARE_VERSION       "1.0.0"
#define QA_SAMPLE_HZ              200U
#define QA_REPORT_HZ              10U
#define QA_NUM_FEET               4U
#define QA_ADC_FULL_SCALE         8388607L
#define QA_LOAD_CELL_MV_V         2.0f
#define QA_EXCITATION_VOLTS       3.0f
#define QA_MAX_LOAD_KG            500.0f
#define QA_IMBALANCE_WARN_PERCENT 12.0f
#define QA_TILT_WARN_DEG          0.35f
#define QA_BATTERY_LOW_MV         3450U
#define QA_FLASH_CAL_ADDRESS      0x0803F800UL
#define QA_CAL_MAGIC              0x51414C31UL
#define QA_BUTTON_DEBOUNCE_MS     30U
#define QA_LONG_PRESS_MS          1500U
#define QA_BLE_MTU                185U
#define QA_NAME                   "QuadraAlign"

/* Logical pins; production maps these to STM32G0 GPIO registers. */
typedef enum {
    PIN_ADC_SCK = 0, PIN_ADC_DOUT_A, PIN_ADC_DOUT_B,
    PIN_IMU_INT, PIN_BLE_TX, PIN_BLE_RX, PIN_BUTTON,
    PIN_LED_R, PIN_LED_G, PIN_LED_B, PIN_PWR_HOLD
} board_pin_t;

typedef enum { LED_OFF, LED_BLUE, LED_GREEN, LED_AMBER, LED_RED, LED_PURPLE } led_color_t;

void board_init(void);
uint32_t board_millis(void);
void board_delay_ms(uint32_t ms);
void board_gpio_write(board_pin_t pin, bool value);
bool board_gpio_read(board_pin_t pin);
void board_led_set(led_color_t color, uint8_t brightness);
uint16_t board_battery_mv(void);
void board_watchdog_kick(void);
void board_power_off(void);
int board_uart_write(const uint8_t *data, size_t length);
int board_uart_read(uint8_t *data, size_t capacity);
int board_i2c_write(uint8_t address, uint8_t reg, const uint8_t *data, size_t length);
int board_i2c_read(uint8_t address, uint8_t reg, uint8_t *data, size_t length);
int board_flash_read(uint32_t address, void *data, size_t length);
int board_flash_write(uint32_t address, const void *data, size_t length);

#endif
