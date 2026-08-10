/**
 * @file    board.h
 * @brief   TideBand — Board pin assignments, clock configuration, and
 *          hardware-specific constants for the TideBand wrist pod.
 * @author  jayis1
 * @copyright © 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 */

#ifndef TIDEBAND_BOARD_H
#define TIDEBAND_BOARD_H

#include <stdint.h>
#include "registers.h"

/* ---- Pin assignments (STM32H733VGT6, 100-pin LQFP) ---- */
/* GPIO port and pin number for each board function.            */

/* Doppler TX — TIM1_CH1 PWM, 1 MHz square wave to LM48611 amp */
#define DOPPLER_TX_GPIO    GPIOA_BASE
#define DOPPLER_TX_PIN     8u
#define DOPPLER_TX_AF      1u       /* TIM1_CH1 on PA8 */
#define DOPPLER_TX_EN_GPIO GPIOA_BASE
#define DOPPLER_TX_EN_PIN  12u      /* TX amplifier enable (active high) */

/* Doppler ADC — AD9629 12-bit, SPI1 on CS via GPIO */
#define DOPPLER_ADC_SPI    SPI1_BASE
#define DOPPLER_ADC_CS_GPIO GPIOA_BASE
#define DOPPLER_ADC_CS_PIN  4u
#define DOPPLER_ADC_SCK_GPIO GPIOB_BASE
#define DOPPLER_ADC_SCK_PIN  3u
#define DOPPLER_ADC_SCK_AF   5u
#define DOPPLER_ADC_MISO_GPIO GPIOB_BASE
#define DOPPLER_ADC_MISO_PIN  4u
#define DOPPLER_ADC_MISO_AF   5u
#define DOPPLER_ADC_MOSI_GPIO GPIOB_BASE
#define DOPPLER_ADC_MOSI_PIN  5u
#define DOPPLER_ADC_MOSI_AF   5u
#define DOPPLER_ADC_BUSY_GPIO GPIOC_BASE
#define DOPPLER_ADC_BUSY_PIN  4u

/* IMU — ICM-42688-P via SPI4 (shared with LCD, separate CS) */
#define IMU_SPI            SPI4_BASE
#define IMU_CS_GPIO        GPIOE_BASE
#define IMU_CS_PIN         4u
#define IMU_SCK_GPIO       GPIOE_BASE
#define IMU_SCK_PIN        2u
#define IMU_SCK_AF         5u
#define IMU_MISO_GPIO      GPIOE_BASE
#define IMU_MISO_PIN       5u
#define IMU_MISO_AF        5u
#define IMU_MOSI_GPIO      GPIOE_BASE
#define IMU_MOSI_PIN       6u
#define IMU_MOSI_AF        5u
#define IMU_INT_GPIO       GPIOE_BASE
#define IMU_INT_PIN        3u

/* Magnetometer — MMC5983MA via I2C2 */
#define MAG_I2C            I2C2_BASE
#define MAG_I2C_ADDR       0x30u   /* MMC5983 I2C address (7-bit: 0x30) */
#define MAG_INT_GPIO       GPIOD_BASE
#define MAG_INT_PIN        0u

/* Pressure sensor — MS5837-30BA via I2C1 */
#define PRESS_I2C          I2C1_BASE
#define PRESS_I2C_ADDR     0x76u   /* MS5837 I2C address */
/* I2C1 pins: PB6 (SCL), PB7 (SDA), AF4 */

/* RTC — PCF8523 via I2C1 (shared with pressure sensor) */
#define RTC_I2C            I2C1_BASE
#define RTC_I2C_ADDR       0x68u

/* Fuel gauge — MAX17055 via I2C1 (shared) */
#define FUEL_I2C           I2C1_BASE
#define FUEL_I2C_ADDR      0x36u

/* NAND flash — W25N02G via SPI4 (shared with IMU, separate CS) */
#define NAND_SPI           SPI4_BASE
#define NAND_CS_GPIO       GPIOD_BASE
#define NAND_CS_PIN        6u
#define NAND_WP_GPIO       GPIOD_BASE
#define NAND_WP_PIN        7u      /* Write protect (active low) */
#define NAND_BUSY_GPIO     GPIOD_BASE
#define NAND_BUSY_PIN      5u

/* BLE module — nRF52840 via USART1 */
#define BLE_UART           USART1_BASE
#define BLE_TX_GPIO        GPIOA_BASE
#define BLE_TX_PIN         9u
#define BLE_TX_AF          7u
#define BLE_RX_GPIO        GPIOA_BASE
#define BLE_RX_PIN         10u
#define BLE_RX_AF          7u
#define BLE_CTS_GPIO       GPIOA_BASE
#define BLE_CTS_PIN        11u
#define BLE_CTS_AF         7u
#define BLE_RTS_GPIO       GPIOA_BASE
#define BLE_RTS_PIN        12u
#define BLE_RTS_AF         7u
#define BLE_RESET_GPIO     GPIOC_BASE
#define BLE_RESET_PIN      5u
#define BLE_INT_GPIO       GPIOC_BASE
#define BLE_INT_PIN        7u

/* Display — Sharp LS013B7DH03 via SPI4 (shared, separate CS) */
#define LCD_SPI            SPI4_BASE
#define LCD_CS_GPIO        GPIOD_BASE
#define LCD_CS_PIN         2u
#define LCD_SCK_GPIO       GPIOE_BASE
#define LCD_SCK_PIN        2u      /* Shared with IMU SCK */
#define LCD_MOSI_GPIO      GPIOE_BASE
#define LCD_MOSI_PIN       6u      /* Shared with IMU MOSI */
#define LCD_DISP_GPIO      GPIOD_BASE
#define LCD_DISP_PIN       3u      /* Display ON/OFF */
#define LCD_EXTMODE_GPIO   GPIOD_BASE
#define LCD_EXTMODE_PIN    4u      /* External COM inversion mode */

/* Haptic motor — TIM2_CH1 PWM */
#define HAPTIC_PWM_GPIO    GPIOA_BASE
#define HAPTIC_PWM_PIN     0u
#define HAPTIC_PWM_AF      1u      /* TIM2_CH1 on PA0 */
#define HAPTIC_EN_GPIO     GPIOA_BASE
#define HAPTIC_EN_PIN      1u      /* Motor driver enable */

/* USB-C charger interface (for firmware updates, not dive use) */
#define USB_DM_GPIO        GPIOA_BASE
#define USB_DM_PIN         11u
#define USB_DP_GPIO        GPIOA_BASE
#define USB_DP_PIN         12u

/* Status LED */
#define LED_GPIO           GPIOB_BASE
#define LED_PIN            0u      /* Blue status LED, active low */

/* ---- Clock configuration ---- */
/* HSE: 8 MHz crystal on PD0/PD1 (RCC_OSC_IN/OUT)                     */
/* PLL1: HSE / 1 (M=1) × 70 (N=70) / 2 (P=2) = 280 MHz SYSCLK         */
/* PLL1Q: / 5 = 56 MHz for USB                                        */
/* PLL1R: / 2 = 280 MHz for SYSCLK                                    */
/* AHB: SYSCLK / 1 = 280 MHz                                          */
/* APB1: SYSCLK / 4 = 70 MHz (TIM2 = 140 MHz)                         */
/* APB2: SYSCLK / 2 = 140 MHz (TIM1 = 280 MHz, SPI1 = 140 MHz)        */

#define BOARD_HSE_HZ           8000000u
#define BOARD_SYSCLK_HZ        280000000u
#define BOARD_AHB_HZ           280000000u
#define BOARD_APB1_HZ          70000000u
#define BOARD_APB1_TIM_HZ      140000000u
#define BOARD_APB2_HZ          140000000u
#define BOARD_APB2_TIM_HZ      280000000u

/* ---- SPI clock configurations ---- */
/* SPI1 (Doppler ADC): 70 MHz max for AD9629 serial */
#define DOPPLER_ADC_SPI_BAUD   70000000u
/* SPI4 (IMU/LCD/NAND): 24 MHz for IMU, 8 MHz for NAND, 8 MHz for LCD */
#define IMU_SPI_BAUD           24000000u
#define NAND_SPI_BAUD          80000000u  /* W25N02G supports up to 104 MHz */
#define LCD_SPI_BAUD           8000000u

/* ---- I2C timing register values ---- */
/* I2C1 at 70 MHz APB1, 400 kHz fast mode: 0x10C0EC7E (from STM32CubeMX) */
#define I2C1_TIMING_400K       0x10C0EC7Eu
/* I2C2 at 70 MHz APB1, 400 kHz fast mode */
#define I2C2_TIMING_400K       0x10C0EC7Eu

/* ---- System tick ---- */
#define SYSTICK_HZ             1000u   /* 1 ms tick */
#define SYSTICK_RELOAD         (BOARD_SYSCLK_HZ / SYSTICK_HZ - 1u)

/* ---- Display dimensions ---- */
#define LCD_WIDTH              128u
#define LCD_HEIGHT             128u
#define LCD_BUF_SIZE           (LCD_WIDTH * LCD_HEIGHT / 8u)  /* 1 bpp = 2048 bytes */

/* ---- Calibration geometry ---- */
/* Number of calibration points: 3 speeds × 6 headings = 18 */
#define CAL_NUM_POINTS         18u

/* ---- Dive log limits ---- */
#define MAX_CONCURRENT_DIVES   1u      /* One active dive at a time */
#define MAX_PROFILE_SAMPLES    0xFFFFFFFFu  /* Limited only by NAND */

/* ---- Helper macros ---- */
#define GPIO_BSRR_SET(port, pin)  ((1u << (pin)))
#define GPIO_BSRR_RESET(port, pin) ((1u << (pin + 16)))

/* Set a GPIO pin's mode (2 bits per pin in MODER) */
static inline void gpio_set_mode(uint32_t port, uint8_t pin, uint8_t mode)
{
    uint32_t moder = GPIO_MODER(port);
    moder &= ~(3u << (pin * 2u));
    moder |= ((uint32_t)mode << (pin * 2u));
    GPIO_MODER(port) = moder;
}

/* Set a GPIO pin's alternate function (4 bits per pin in AFR) */
static inline void gpio_set_af(uint32_t port, uint8_t pin, uint8_t af)
{
    if (pin < 8u) {
        uint32_t afrl = GPIO_AFRL(port);
        afrl &= ~(0xFu << (pin * 4u));
        afrl |= ((uint32_t)af << (pin * 4u));
        GPIO_AFRL(port) = afrl;
    } else {
        uint8_t hp = pin - 8u;
        uint32_t afrh = GPIO_AFRH(port);
        afrh &= ~(0xFu << (hp * 4u));
        afrh |= ((uint32_t)af << (hp * 4u));
        GPIO_AFRH(port) = afrh;
    }
}

/* Set GPIO output speed */
static inline void gpio_set_speed(uint32_t port, uint8_t pin, uint8_t speed)
{
    uint32_t ospeedr = GPIO_OSPEEDR(port);
    ospeedr &= ~(3u << (pin * 2u));
    ospeedr |= ((uint32_t)speed << (pin * 2u));
    GPIO_OSPEEDR(port) = ospeedr;
}

/* Set GPIO pull-up/pull-down */
static inline void gpio_set_pupd(uint32_t port, uint8_t pin, uint8_t pupd)
{
    uint32_t pupdr = GPIO_PUPDR(port);
    pupdr &= ~(3u << (pin * 2u));
    pupdr |= ((uint32_t)pupd << (pin * 2u));
    GPIO_PUPDR(port) = pupdr;
}

/* Write GPIO pin high */
static inline void gpio_set(uint32_t port, uint8_t pin)
{
    GPIO_BSRR(port) = GPIO_BSRR_SET(port, pin);
}

/* Write GPIO pin low */
static inline void gpio_clear(uint32_t port, uint8_t pin)
{
    GPIO_BSRR(port) = GPIO_BSRR_RESET(port, pin);
}

/* Read GPIO input */
static inline uint8_t gpio_read(uint32_t port, uint8_t pin)
{
    return (GPIO_IDR(port) >> pin) & 1u;
}

#endif /* TIDEBAND_BOARD_H */