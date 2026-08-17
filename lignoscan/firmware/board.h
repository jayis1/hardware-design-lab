/*
 * board.h — LignoScan Board Pin Assignments and Configuration
 *
 * LignoScan — Portable Acoustic Tomography Scanner
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#ifndef LIGNOSCAN_BOARD_H
#define LIGNOSCAN_BOARD_H

#include "registers.h"

/* ---- Board identity ---- */
#define BOARD_NAME      "LignoScan"
#define BOARD_VERSION   "1.0"
#define BOARD_AUTHOR    "jayis1"

/* ---- System clock configuration ---- */
#define HSE_VALUE       8000000UL   /* 8 MHz external crystal */
#define SYSCLK_FREQ     280000000UL /* 280 MHz core */
#define APB1_FREQ       140000000UL /* APB1 = SYSCLK/2 */
#define APB2_FREQ       140000000UL /* APB2 = SYSCLK/2 */

/* ---- SPI1: TDC-GP22 (high-speed, up to 20 MHz) ---- */
#define TDC_SPI         SPI1
#define TDC_SPI_SCK     GPIOA   /* PA5 */
#define TDC_SPI_SCK_PIN 5
#define TDC_SPI_MISO    GPIOA   /* PA6 */
#define TDC_SPI_MISO_PIN 6
#define TDC_SPI_MOSI    GPIOB   /* PB5 */
#define TDC_SPI_MOSI_PIN 5
#define TDC_SPI_CS      GPIOA   /* PA4 */
#define TDC_SPI_CS_PIN  4
#define TDC_SPI_AF      5       /* AF5 for SPI1 */

/* ---- SPI2: SD card (FAT32, SPI mode) ---- */
#define SD_SPI          SPI2
#define SD_SPI_SCK      GPIOB   /* PB10 */
#define SD_SPI_SCK_PIN  10
#define SD_SPI_MISO     GPIOC   /* PC2 */
#define SD_SPI_MISO_PIN 2
#define SD_SPI_MOSI     GPIOC   /* PC3 */
#define SD_SPI_MOSI_PIN 3
#define SD_CS           GPIOB   /* PB12 */
#define SD_CS_PIN       12
#define SD_SPI_AF       5

/* ---- SPI3: OLED display (SSD1306) ---- */
#define OLED_SPI        SPI3
#define OLED_SCK        GPIOC   /* PC10 */
#define OLED_SCK_PIN    10
#define OLED_MOSI       GPIOC   /* PC12 */
#define OLED_MOSI_PIN   12
#define OLED_CS         GPIOA   /* PA15 */
#define OLED_CS_PIN     15
#define OLED_DC         GPIOB   /* PB4 */
#define OLED_DC_PIN     4
#define OLED_RST        GPIOB   /* PB3 */
#define OLED_RST_PIN    3
#define OLED_SPI_AF     6

/* ---- USART1: nRF52833 BLE module (1 Mbps) ---- */
#define BLE_UART        USART1
#define BLE_TX          GPIOB   /* PB6 */
#define BLE_TX_PIN      6
#define BLE_RX          GPIOB   /* PB7 */
#define BLE_RX_PIN      7
#define BLE_UART_AF     7
#define BLE_BAUD        1000000UL

/* ---- USART3: u-blox NEO-M9N GPS ---- */
#define GPS_UART        USART3
#define GPS_TX          GPIOB   /* PB10 (alt function, mux'd) */
#define GPS_TX_PIN      10
#define GPS_RX          GPIOB   /* PB11 */
#define GPS_RX_PIN      11
#define GPS_UART_AF     7
#define GPS_BAUD        38400UL

/* ---- UART4: Debug console ---- */
#define DBG_UART        UART4
#define DBG_TX          GPIOA   /* PA0 */
#define DBG_TX_PIN      0
#define DBG_RX          GPIOA   /* PA1 */
#define DBG_RX_PIN      1
#define DBG_UART_AF     6
#define DBG_BAUD        115200UL

/* ---- I2C1: MAX17048 fuel gauge ---- */
#define FUEL_I2C        I2C1
#define FUEL_SCL        GPIOB   /* PB8 */
#define FUEL_SCL_PIN    8
#define FUEL_SDA        GPIOB   /* PB9 */
#define FUEL_SDA_PIN    9
#define FUEL_I2C_AF     4
#define FUEL_ADDR       0x36

/* ---- GPIO: HV H-bridge control ---- */
#define HV_EN           GPIOC   /* PC0 — HV supply enable */
#define HV_EN_PIN       0
#define HV_H_IN1        GPIOC   /* PC1 — H-bridge high-side */
#define HV_H_IN1_PIN    1
#define HV_H_IN2        GPIOC   /* PC2 — H-bridge high-side */
#define HV_H_IN2_PIN    2
#define HV_L_IN1        GPIOC   /* PC3 — H-bridge low-side */
#define HV_L_IN1_PIN    3
#define HV_L_IN2        GPIOC   /* PC4 — H-bridge low-side */
#define HV_L_IN2_PIN    4
#define HV_PULSE_WIDTH_US  5    /* Default pulse width in microseconds */

/* ---- GPIO: TX/RX MUX control (via 74HC4067) ---- */
#define TX_MUX_A0       GPIOD   /* PD0 */
#define TX_MUX_A0_PIN   0
#define TX_MUX_A1       GPIOD   /* PD1 */
#define TX_MUX_A1_PIN   1
#define TX_MUX_A2       GPIOD   /* PD2 */
#define TX_MUX_A2_PIN   2
#define TX_MUX_A3       GPIOD   /* PD3 */
#define TX_MUX_A3_PIN   3
#define TX_MUX_EN       GPIOD   /* PD4 — active low enable */
#define TX_MUX_EN_PIN   4

#define RX_MUX_A0       GPIOD   /* PD5 */
#define RX_MUX_A0_PIN   5
#define RX_MUX_A1       GPIOD   /* PD6 */
#define RX_MUX_A1_PIN   6
#define RX_MUX_A2       GPIOD   /* PD7 */
#define RX_MUX_A2_PIN   7
#define RX_MUX_A3       GPIOD   /* PD8 */
#define RX_MUX_A3_PIN   8
#define RX_MUX_EN       GPIOD   /* PD9 — active low enable */
#define RX_MUX_EN_PIN   9

/* ---- GPIO: Sensor cable ID ADC (0-15 via resistor divider) ---- */
#define CABLE_ID_ADC    GPIOA   /* PA3 — ADC1_IN3 */
#define CABLE_ID_PIN    3

/* ---- GPIO: VGA gain control (DAC via SPI) ---- */
#define VGA_DAC_CS      GPIOB   /* PB2 */
#define VGA_DAC_CS_PIN  2

/* ---- GPIO: Comparator threshold (DAC via SPI, shared bus) ---- */
#define CMP_DAC_CS      GPIOB   /* PB1 */
#define CMP_DAC_CS_PIN  1

/* ---- GPIO: User controls ---- */
#define BTN_SCAN        GPIOE   /* PE0 — Scan button (active low) */
#define BTN_SCAN_PIN    0
#define BTN_MODE        GPIOE   /* PE1 — Mode button */
#define BTN_MODE_PIN    1
#define BTN_POWER       GPIOE   /* PE2 — Power button */
#define BTN_POWER_PIN   2
#define ROTARY_A        GPIOE   /* PE3 — Rotary encoder A */
#define ROTARY_A_PIN    3
#define ROTARY_B        GPIOE   /* PE4 — Rotary encoder B */
#define ROTARY_B_PIN    4
#define ROTARY_BTN      GPIOE   /* PE5 — Rotary encoder push */
#define ROTARY_BTN_PIN  5

/* ---- GPIO: Status LEDs ---- */
#define LED_STATUS      GPIOB   /* PB0 — Green status LED */
#define LED_STATUS_PIN  0
#define LED_ERROR       GPIOB   /* PB1 — Red error LED (shared with DAC CS — mux'd) */
#define LED_SCAN        GPIOE   /* PE6 — Blue scan-in-progress LED */
#define LED_SCAN_PIN    6

/* ---- GPIO: TDC interrupt ---- */
#define TDC_INT         GPIOC   /* PC6 — TDC-GP22 interrupt (ALU busy) */
#define TDC_INT_PIN     6

/* ---- GPIO: USB-C VBUS detect ---- */
#define VBUS_DETECT     GPIOA   /* PA9 */
#define VBUS_DETECT_PIN 9

/* ---- GPIO: Charger status ---- */
#define CHG_STAT1       GPIOC   /* PC13 — MCP73871 STAT1 */
#define CHG_STAT1_PIN   13
#define CHG_STAT2       GPIOC   /* PC14 — MCP73871 STAT2 */
#define CHG_STAT2_PIN   14

/* ---- Scan configuration defaults ---- */
#define MAX_SENSORS         16
#define MIN_SENSORS         8
#define SHOTS_PER_PAIR      16      /* Number of pulse averages per TX-RX pair */
#define TOMO_ITERATIONS     50      /* SART iterations */
#define TOMO_RADIAL_CELLS   8       /* Polar grid radial divisions */
#define TOMO_ANGULAR_CELLS  16      /* Polar grid angular divisions */
#define SOUND_WOOD_VMIN     2500.0f /* m/s — above this = sound wood */
#define MOD_DECAY_VMIN      1500.0f /* m/s — above this = moderate decay */

/* ---- Timing ---- */
#define HV_RECHARGE_MS      2       /* HV cap recharge time between shots */
#define MUX_SETTLE_US       50      /* MUX switch settling time */
#define TDC_TIMEOUT_US      10000   /* TDC measurement timeout */

/* ---- BLE protocol ---- */
#define BLE_SERVICE_UUID    "0000LIGN-0000-1000-8000-00805F9B34FB"
#define BLE_MTU             247
#define BLE_PACKET_MAX      240

/* ---- SD card ---- */
#define SD_BLOCK_SIZE       512
#define SD_MAX_RETRIES      3

/* ---- Function prototypes ---- */
void board_init(void);
void clock_init(void);
void gpio_init_all(void);
void delay_ms(uint32_t ms);
void delay_us(uint32_t us);
uint32_t millis(void);

#endif /* LIGNOSCAN_BOARD_H */