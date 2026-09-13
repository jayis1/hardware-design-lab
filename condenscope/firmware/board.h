/* CondenScope board support definitions
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef CONDENSCOPE_BOARD_H
#define CONDENSCOPE_BOARD_H

#include <stdint.h>
#include <stdbool.h>

#define CPU_CLOCK_HZ           160000000UL
#define APB1_CLOCK_HZ           80000000UL
#define SENSOR_SAMPLE_HZ               8U
#define THERMAL_WIDTH                  32U
#define THERMAL_HEIGHT                 24U
#define THERMAL_PIXELS                (THERMAL_WIDTH * THERMAL_HEIGHT)
#define MAP_MAX_CELLS                 256U
#define USB_FRAME_MTU                1024U
#define BLE_FRAME_MTU                 240U

/* STM32U585 package pin encoding: port nibble, then pin. */
#define PIN(port, pin) (((port) << 4U) | (pin))
#define PORT_A 0U
#define PORT_B 1U
#define PORT_C 2U
#define PORT_D 3U

#define PIN_I2C1_SCL       PIN(PORT_B, 8)
#define PIN_I2C1_SDA       PIN(PORT_B, 9)
#define PIN_I2C2_SCL       PIN(PORT_B, 10)
#define PIN_I2C2_SDA       PIN(PORT_B, 11)
#define PIN_TOF_XSHUT      PIN(PORT_C, 4)
#define PIN_SENSOR_INT     PIN(PORT_C, 5)
#define PIN_TRIGGER        PIN(PORT_A, 0)
#define PIN_STATUS_LED_R   PIN(PORT_A, 8)
#define PIN_STATUS_LED_G   PIN(PORT_A, 9)
#define PIN_STATUS_LED_B   PIN(PORT_A, 10)
#define PIN_BUZZER         PIN(PORT_B, 4)
#define PIN_BATTERY_ADC    PIN(PORT_C, 0)
#define PIN_USB_DM         PIN(PORT_A, 11)
#define PIN_USB_DP         PIN(PORT_A, 12)
#define PIN_SWDIO          PIN(PORT_A, 13)
#define PIN_SWCLK          PIN(PORT_A, 14)

#define MLX90640_ADDRESS       0x33U
#define SHT45_ADDRESS          0x44U
#define VL53L4CD_ADDRESS       0x29U
#define FRAM_ADDRESS           0x50U
#define MAX17048_ADDRESS       0x36U

#define MLX_FRAME_WORDS        834U
#define MLX_EEPROM_WORDS       832U
#define FRAM_SIZE_BYTES      32768U
#define FRAM_MAP_BASE       0x1000U

#define VBAT_DIVIDER_NUM       2000U
#define VBAT_DIVIDER_DEN       1000U
#define ADC_REFERENCE_MV       3300U
#define ADC_FULL_SCALE         4095U

#define DEW_WARNING_CENTIC       250
#define DEW_DANGER_CENTIC         75
#define DISTANCE_MIN_MM           80U
#define DISTANCE_MAX_MM         2500U
#define EMISSIVITY_DEFAULT_Q15  31130U /* 0.95 */

#define ARRAY_LEN(x) (sizeof(x) / sizeof((x)[0]))
#define CLAMP(x, lo, hi) ((x) < (lo) ? (lo) : ((x) > (hi) ? (hi) : (x)))

/* Platform functions implemented by startup/HAL layer. */
void board_clock_init(void);
void board_gpio_init(void);
void board_i2c_init(void);
void board_usb_init(void);
void board_ble_uart_init(void);
void board_adc_init(void);
void board_watchdog_init(void);
void board_watchdog_feed(void);
uint32_t board_millis(void);
void board_delay_ms(uint32_t milliseconds);
void board_led_set(uint8_t red, uint8_t green, uint8_t blue);
void board_buzzer_set(uint16_t frequency_hz, uint8_t duty_percent);
bool board_trigger_pressed(void);
uint16_t board_adc_read(uint8_t channel);
int board_i2c_read16(uint8_t bus, uint8_t address, uint16_t reg,
                     uint16_t *data, uint16_t words);
int board_i2c_write16(uint8_t bus, uint8_t address, uint16_t reg,
                      const uint16_t *data, uint16_t words);
int board_i2c_read8(uint8_t bus, uint8_t address, uint16_t reg,
                    uint8_t *data, uint16_t length);
int board_i2c_write8(uint8_t bus, uint8_t address, uint16_t reg,
                     const uint8_t *data, uint16_t length);
uint16_t board_usb_write(const uint8_t *data, uint16_t length);
uint16_t board_ble_write(const uint8_t *data, uint16_t length);
uint16_t board_transport_read(uint8_t *data, uint16_t capacity);
void board_enter_stop2(void);
void board_system_reset(void);

#endif
