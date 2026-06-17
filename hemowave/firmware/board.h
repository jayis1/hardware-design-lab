/**
 * @file    board.h
 * @brief   HemoWave — Board-specific pin definitions and peripheral mapping
 * @author  jayis1
 * @copyright  Copyright © 2026 jayis1. All rights reserved.
 * @license  CERN-OHL-S v2 (hardware), GPL-2.0 (firmware), MIT (app)
 *
 * Target:  STM32U585AII6Q (UFBGA169 package)
 * Board:   HemoWave v1.0 — Wearable MW-PPG + BIS Monitor
 *
 * This file maps every external pin, peripheral instance, and alternate
 * function used on the HemoWave PCB. Any porting to a different board or
 * MCU package should start by modifying the definitions in this file.
 */

#ifndef HEMOWAVE_BOARD_H
#define HEMOWAVE_BOARD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* =========================================================================
 *  MCU Identification
 *  ========================================================================= */
#define MCU_PART           "STM32U585AII6Q"
#define MCU_CORE           "ARM Cortex-M33"
#define MCU_SPEED_HZ       160000000UL
#define FLASH_SIZE         0x00200000UL   /* 2 MB */
#define SRAM_TOTAL         0x000C6000UL   /* 786 KB (SRAM1+2+3) */

/* =========================================================================
 *  Clock Configuration
 *  ========================================================================= */
#define HSE_HZ              16000000UL    /* 16 MHz external crystal */
#define HSI_HZ              16000000UL    /* 16 MHz internal RC */
#define LSE_HZ              32768UL       /* 32.768 kHz RTC crystal */
#define SYSTEM_CLOCK_HZ     160000000UL
#define APB1_CLOCK_HZ       160000000UL
#define APB2_CLOCK_HZ       160000000UL

/* =========================================================================
 *  SPI1 — AFE4900 PPG Analog Front-End
 *  ========================================================================= */
#define SPI1_INSTANCE       SPI1
#define SPI1_SCK_PORT       GPIOA
#define SPI1_SCK_PIN        GPIO_PIN_5
#define SPI1_SCK_AF         GPIO_AF5_SPI1
#define SPI1_MISO_PORT      GPIOA
#define SPI1_MISO_PIN       GPIO_PIN_6
#define SPI1_MISO_AF        GPIO_AF5_SPI1
#define SPI1_MOSI_PORT      GPIOA
#define SPI1_MOSI_PIN       GPIO_PIN_7
#define SPI1_MOSI_AF        GPIO_AF5_SPI1
#define SPI1_CS_PORT        GPIOB
#define SPI1_CS_PIN         GPIO_PIN_0
#define SPI1_FREQ_HZ        8000000UL     /* 8 MHz */
#define SPI1_MODE           SPI_MODE_0   /* CPOL=0, CPHA=0 */
#define SPI1_DIR            SPI_DIR_2LINES

/* AFE4900 interrupt (data ready) */
#define AFE4900_DRDY_PORT   GPIOB
#define AFE4900_DRDY_PIN    GPIO_PIN_1
#define AFE4900_DRDY_EXTI   EXTI1_IRQn
#define AFE4900_RESET_PORT  GPIOB
#define AFE4900_RESET_PIN   GPIO_PIN_2

/* =========================================================================
 *  SPI2 — AD5941 Bioimpedance AFE
 *  ========================================================================= */
#define SPI2_INSTANCE       SPI2
#define SPI2_SCK_PORT       GPIOB
#define SPI2_SCK_PIN        GPIO_PIN_10
#define SPI2_SCK_AF         GPIO_AF5_SPI2
#define SPI2_MISO_PORT      GPIOB
#define SPI2_MISO_PIN       GPIO_PIN_14
#define SPI2_MISO_AF        GPIO_AF5_SPI2
#define SPI2_MOSI_PORT      GPIOB
#define SPI2_MOSI_PIN       GPIO_PIN_15
#define SPI2_MOSI_AF        GPIO_AF5_SPI2
#define SPI2_CS_PORT        GPIOC
#define SPI2_CS_PIN         GPIO_PIN_4
#define SPI2_FREQ_HZ        12000000UL    /* 12 MHz */
#define SPI2_MODE           SPI_MODE_1   /* CPOL=0, CPHA=1 */
#define SPI2_DIR            SPI_DIR_2LINES

/* AD5941 control signals */
#define AD5941_DRDY_PORT    GPIOC
#define AD5941_DRDY_PIN     GPIO_PIN_5
#define AD5941_DRDY_EXTI    EXTI5_IRQn
#define AD5941_RESET_PORT   GPIOC
#define AD5941_RESET_PIN    GPIO_PIN_6
#define AD5941_GPIO0_PORT   GPIOC
#define AD5941_GPIO0_PIN    GPIO_PIN_7

/* =========================================================================
 *  SPI3 / QSPI — IS66WVS4M8J PSRAM (Data Buffer)
 *  ========================================================================= */
#define QSPI_PSRAM_INST     OCTOSPI1
#define QSPI_PSRAM_CS_PORT  GPIOE
#define QSPI_PSRAM_CS_PIN   GPIO_PIN_11
#define QSPI_PSRAM_CLK_PORT GPIOE
#define QSPI_PSRAM_CLK_PIN  GPIO_PIN_12
#define QSPI_PSRAM_DQ0_PORT GPIOE
#define QSPI_PSRAM_DQ0_PIN  GPIO_PIN_13
#define QSPI_PSRAM_DQ1_PORT GPIOE
#define QSPI_PSRAM_DQ1_PIN  GPIO_PIN_14
#define QSPI_PSRAM_DQ2_PORT GPIOE
#define QSPI_PSRAM_DQ2_PIN  GPIO_PIN_15
#define QSPI_PSRAM_DQ3_PORT GPIOB
#define QSPI_PSRAM_DQ3_PIN  GPIO_PIN_3
#define QSPI_PSRAM_FREQ_HZ  166000000UL  /* 166 MHz DDR */
#define QSPI_PSRAM_SIZE     (8 * 1024 * 1024) /* 8 MB */
#define QSPI_PSRAM_ADDR     0x90000000UL

/* =========================================================================
 *  QUADSPI — W25Q64JV Flash (Model + Config)
 *  ========================================================================= */
#define QSPI_FLASH_INST     QUADSPI
#define QSPI_FLASH_CS_PORT  GPIOB
#define QSPI_FLASH_CS_PIN   GPIO_PIN_6
#define QSPI_FLASH_CLK_PORT GPIOB
#define QSPI_FLASH_CLK_PIN  GPIO_PIN_3
#define QSPI_FLASH_DQ0_PORT GPIOD
#define QSPI_FLASH_DQ0_PIN  GPIO_PIN_12
#define QSPI_FLASH_DQ1_PORT GPIOD
#define QSPI_FLASH_DQ1_PIN  GPIO_PIN_13
#define QSPI_FLASH_DQ2_PORT GPIOD
#define QSPI_FLASH_DQ2_PIN  GPIO_PIN_14
#define QSPI_FLASH_DQ3_PORT GPIOD
#define QSPI_FLASH_DQ3_PIN  GPIO_PIN_15
#define QSPI_FLASH_FREQ_HZ  80000000UL   /* 80 MHz */
#define QSPI_FLASH_SIZE     (8 * 1024 * 1024) /* 8 MB */
#define QSPI_FLASH_ADDR     0x70000000UL

/* =========================================================================
 *  I2C1 — BMI270 IMU + TMP117 Temperature
 *  ========================================================================= */
#define I2C1_INSTANCE       I2C1
#define I2C1_SCL_PORT       GPIOB
#define I2C1_SCL_PIN        GPIO_PIN_8
#define I2C1_SCL_AF         GPIO_AF4_I2C1
#define I2C1_SDA_PORT       GPIOB
#define I2C1_SDA_PIN        GPIO_PIN_9
#define I2C1_SDA_AF         GPIO_AF4_I2C1
#define I2C1_FREQ_HZ        400000UL     /* Fast Mode 400 kHz */
#define I2C1_TIMING         0x00707DBC   /* CubeMX-generated timing */

/* BMI270 IMU */
#define BMI270_ADDR         0x68         /* 7-bit I²C address (ADDR=0) */
#define BMI270_INT1_PORT    GPIOA
#define BMI270_INT1_PIN     GPIO_PIN_2
#define BMI270_INT1_EXTI    EXTI2_IRQn

/* TMP117 Temperature Sensor */
#define TMP117_ADDR         0x48         /* 7-bit I²C address */
#define TMP117_DRDY_PORT    GPIOA
#define TMP117_DRDY_PIN     GPIO_PIN_3
#define TMP117_DRDY_EXTI    EXTI3_IRQn

/* =========================================================================
 *  I2C2 — MAX77734 PMIC
 *  ========================================================================= */
#define I2C2_INSTANCE       I2C2
#define I2C2_SCL_PORT       GPIOH
#define I2C2_SCL_PIN        GPIO_PIN_4
#define I2C2_SCL_AF         GPIO_AF4_I2C2
#define I2C2_SDA_PORT       GPIOH
#define I2C2_SDA_PIN        GPIO_PIN_5
#define I2C2_SDA_AF         GPIO_AF4_I2C2
#define I2C2_FREQ_HZ        100000UL     /* Standard Mode 100 kHz */
#define I2C2_TIMING         0x00B03CFB

#define MAX77734_ADDR       0x38         /* 7-bit I²C address */
#define MAX77734_IRQ_PORT   GPIOA
#define MAX77734_IRQ_PIN    GPIO_PIN_0
#define MAX77734_IRQ_EXTI   EXTI0_IRQn

/* =========================================================================
 *  LPUART1 — DA14531MOD BLE Module
 *  ========================================================================= */
#define LPUART1_INSTANCE    LPUART1
#define LPUART1_TX_PORT     GPIOA
#define LPUART1_TX_PIN      GPIO_PIN_9
#define LPUART1_TX_AF       GPIO_AF8_LPUART1
#define LPUART1_RX_PORT     GPIOA
#define LPUART1_RX_PIN      GPIO_PIN_10
#define LPUART1_RX_AF       GPIO_AF8_LPUART1
#define LPUART1_RTS_PORT    GPIOA
#define LPUART1_RTS_PIN     GPIO_PIN_12
#define LPUART1_CTS_PORT    GPIOA
#define LPUART1_CTS_PIN     GPIO_PIN_11
#define LPUART1_BAUD        115200
#define LPUART1_DATA_BITS   8
#define LPUART1_STOP_BITS   1
#define LPUART1_PARITY      UART_PARITY_NONE

/* BLE module control */
#define BLE_RST_PORT        GPIOA
#define BLE_RST_PIN         GPIO_PIN_15
#define BLE_CTS_PORT        GPIOB
#define BLE_CTS_PIN         GPIO_PIN_4
#define BLE_WAKE_PORT       GPIOB
#define BLE_WAKE_PIN        GPIO_PIN_5

/* =========================================================================
 *  USB — STUSB4500 USB-C PD Controller (via I²C3)
 *  ========================================================================= */
#define I2C3_INSTANCE       I2C3
#define I2C3_SCL_PORT       GPIOA
#define I2C3_SCL_PIN        GPIO_PIN_8
#define I2C3_SCL_AF         GPIO_AF4_I2C3
#define I2C3_SDA_PORT       GPIOC
#define I2C3_SDA_PIN        GPIO_PIN_9
#define I2C3_SDA_AF         GPIO_AF4_I2C3
#define I2C3_FREQ_HZ        100000UL

#define STUSB4500_ADDR      0x28

/* =========================================================================
 *  USB — STM32 native USB 2.0 FS for CDC + DFU
 *  ========================================================================= */
#define USB_DM_PORT         GPIOA
#define USB_DM_PIN          GPIO_PIN_11
#define USB_DP_PORT         GPIOA
#define USB_DP_PIN          GPIO_PIN_12
#define USB_VBUS_EN_PORT    GPIOA
#define USB_VBUS_EN_PIN     GPIO_PIN_13
#define USB_OVERCUR_PORT    GPIOA
#define USB_OVERCUR_PIN     GPIO_PIN_14
#define USB_IRQ             OTG_FS_IRQn

/* =========================================================================
 *  LED Multiplexer Control — 4× TPS22965 Load Switches
 *  ========================================================================= */
#define LED_MUX_EN_PORT     GPIOB
#define LED_MUX_EN_PIN      GPIO_PIN_12   /* Master enable for all LEDs */
#define LED_SEL0_PORT       GPIOB
#define LED_SEL0_PIN        GPIO_PIN_13
#define LED_SEL1_PORT       GPIOD
#define LED_SEL1_PIN        GPIO_PIN_8
#define LED_SEL2_PORT       GPIOD
#define LED_SEL2_PIN        GPIO_PIN_9
#define LED_SEL_A0_PORT     GPIOD
#define LED_SEL_A0_PIN      GPIO_PIN_10  /* Address bit 0 for mux */
#define LED_SEL_A1_PORT     GPIOD
#define LED_SEL_A1_PIN      GPIO_PIN_11  /* Address bit 1 for mux */
#define LED_SEL_A2_PORT     GPIOE
#define LED_SEL_A2_PIN      GPIO_PIN_0   /* Address bit 2 for mux */
#define LED_SEL_A3_PORT     GPIOE
#define LED_SEL_A3_PIN      GPIO_PIN_1   /* Address bit 3 for mux */

/* =========================================================================
 *  BIS Electrode Control
 *  ========================================================================= */
#define BIS_DRV_EN_PORT     GPIOE
#define BIS_DRV_EN_PIN      GPIO_PIN_2   /* Enable BIS current source */
#define BIS_SENSE_EN_PORT   GPIOE
#define BIS_SENSE_EN_PIN    GPIO_PIN_3   /* Enable BIS sense input */

/* =========================================================================
 *  GPIO Expander (if fitted — TCA6408A over I²C1)
 *  ========================================================================= */
#define TCA6408A_ADDR       0x20
#define TCA6408A_INT_PORT   GPIOA
#define TCA6408A_INT_PIN    GPIO_PIN_1
#define TCA6408A_INT_EXTI   EXTI1_IRQn

/* =========================================================================
 *  On-Board Peripherals
 *  ========================================================================= */
/* Status LED (RGB NeoPixel or simple tricolor) */
#define STATUS_LED_R_PORT   GPIOE
#define STATUS_LED_R_PIN    GPIO_PIN_4
#define STATUS_LED_G_PORT   GPIOE
#define STATUS_LED_G_PIN    GPIO_PIN_5
#define STATUS_LED_B_PORT   GPIOE
#define STATUS_LED_B_PIN    GPIO_PIN_6

/* Button */
#define BTN_USER_PORT       GPIOC
#define BTN_USER_PIN        GPIO_PIN_13
#define BTN_USER_EXTI       EXTI13_IRQn

/* Vibration motor */
#define VIBRO_PORT          GPIOE
#define VIBRO_PIN           GPIO_PIN_7

/* =========================================================================
 *  ADC Channels (internal MCU ADC1)
 *  ========================================================================= */
#define ADC1_BATTERY_CH     ADC_CHANNEL_1  /* Battery voltage divider */
#define ADC1_SKIN_TEMP_CH   ADC_CHANNEL_2  /* External thermistor */
#define ADC1_PD_CURRENT_CH  ADC_CHANNEL_3  /* Photodiode bias monitor */

/* =========================================================================
 *  DMA Streams
 *  ========================================================================= */
#define DMA_PPG_RX          DMA1_Channel1   /* SPI1 RX → PPG buffer */
#define DMA_BIS_RX          DMA1_Channel2   /* SPI2 RX → BIS buffer */
#define DMA_BLE_TX          DMA1_Channel3   /* LPUART1 TX → BLE FIFO */
#define DMA_PSRAM_TX        DMA2_Channel1   /* QSPI PSRAM TX */

/* =========================================================================
 *  Timer / PWM
 *  ========================================================================= */
#define TIM_PPG_SAMPLE      TIM1            /* 100 kHz sample clock */
#define TIM_IMU_SAMPLE      TIM2            /* 1.6 kHz IMU trigger */
#define TIM_LED_PWM         TIM3            /* LED brightness PWM */
#define TIM_VIBRO_PWM       TIM4            /* Vibration motor PWM */

/* =========================================================================
 *  Pin Initialization Helper Macro
 *  ========================================================================= */
#define PIN_CFG_OUTPUT(port, pin, init_state)                       \
    do {                                                            \
        LL_GPIO_SetPinMode(port, pin, LL_GPIO_MODE_OUTPUT);        \
        LL_GPIO_SetPinOutputType(port, pin, LL_GPIO_OUTPUT_PUSHPULL); \
        LL_GPIO_SetPinSpeed(port, pin, LL_GPIO_SPEED_FREQ_LOW);    \
        LL_GPIO_SetPinPull(port, pin, LL_GPIO_PULL_NO);            \
        LL_GPIO_WriteOutputPin(port, pin, (init_state));           \
    } while (0)

#define PIN_CFG_INPUT(port, pin, pull)                              \
    do {                                                            \
        LL_GPIO_SetPinMode(port, pin, LL_GPIO_MODE_INPUT);         \
        LL_GPIO_SetPinPull(port, pin, (pull));                      \
    } while (0)

#define PIN_CFG_AF(port, pin, af, speed)                            \
    do {                                                            \
        LL_GPIO_SetPinMode(port, pin, LL_GPIO_MODE_ALTERNATE);     \
        LL_GPIO_SetAFPin_0_7(port, pin, (af));                     \
        LL_GPIO_SetPinSpeed(port, pin, (speed));                   \
        LL_GPIO_SetPinOutputType(port, pin, LL_GPIO_OUTPUT_PUSHPULL); \
        LL_GPIO_SetPinPull(port, pin, LL_GPIO_PULL_NO);            \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif /* HEMOWAVE_BOARD_H */