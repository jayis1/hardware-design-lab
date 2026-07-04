/*
 * board.h — GlyphFlow hardware pin map and board constants
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 *
 * STM32L432KC (UFQFPN32) pin assignments for the GlyphFlow
 * air-handwriting recognition wristband.
 */
#ifndef GLYPHFLOW_BOARD_H
#define GLYPHFLOW_BOARD_H

#include <stdint.h>

/* ---- MCU identity --------------------------------------------------- */
#define MCU_NAME        "STM32L432KC"
#define SYSCLK_HZ       80000000UL
#define LSE_HZ          32768UL
#define FLASH_SIZE_KB   256
#define SRAM_SIZE_KB    64

/* ---- Pin map -------------------------------------------------------- */
/*
 *  PA0  : ADC_IN5  — VBAT divider (1/3 LiPo via 100k/200k)
 *  PA1  : GPIO_OUT — IMU CS (chip select, active low)
 *  PA2  : GPIO_OUT — BLE module WAKE / reset (active low)
 *  PA3  : GPIO_IN  — BLE module IRQ (UART-CTS equivalent, EXTI3)
 *  PA4  : GPIO_OUT — LED DAT (APA3010 data line)
 *  PA5  : SPI1_SCK — IMU SPI clock (alt fn 5)
 *  PA6  : SPI1_MISO— IMU SPI MISO
 *  PA7  : SPI1_MOSI— IMU SPI MOSI
 *  PA8  : GPIO_IN  — TOUCH button (capacitive sensor OUT, EXTI8)
 *  PA9  : USART1_TX— BLE module UART TX
 *  PA10 : USART1_RX— BLE module UART RX
 *  PA11 : GPIO_OUT — Haptic EN (DRV2605L enable, active high)
 *  PA12 : GPIO_OUT — IMU CS2 (flash CS, unused on minimal build)
 *  PB0  : I2C1_SCL — MAX17048 + DRV2605L (alt fn 4)
 *  PB1  : I2C1_SDA — MAX17048 + DRV2605L (alt fn 4)
 *  PB4  : GPIO_IN  — IMU INT1 (data-ready / wake-on-motion, EXTI4)
 *  PB5  : GPIO_IN  — IMU INT2 (free-fall / tap, EXTI5, unused)
 *  PB6  : GPIO_OUT — CHG LED (TP4056 CHRG, open-drain sink)
 *  PB7  : GPIO_OUT — STBY LED (TP4056 STDBY, open-drain sink)
 *  PB8  : GPIO_OUT — nRST of BLE module (alt, also SWO if debugging)
 *  PC14 : OSC32_IN— 32.768 kHz LSE crystal (RTC)
 *  PC15 : OSC32_OUT
 *
 *  USB-C: PA11/PA12 are reused as USB DM/DP in DFU bootloader mode;
 *         in normal run mode they are GPIO + USART1 as above.
 */

#define PIN_IMU_CS_PORT     GPIOA
#define PIN_IMU_CS_BIT      1
#define PIN_BLE_WAKE_PORT   GPIOA
#define PIN_BLE_WAKE_BIT    2
#define PIN_BLE_IRQ_PORT    GPIOA
#define PIN_BLE_IRQ_BIT     3
#define PIN_LED_PORT        GPIOA
#define PIN_LED_BIT         4
#define PIN_TOUCH_PORT      GPIOA
#define PIN_TOUCH_BIT       8
#define PIN_HAPTIC_EN_PORT  GPIOA
#define PIN_HAPTIC_EN_BIT   11
#define PIN_IMU_INT1_PORT   GPIOB
#define PIN_IMU_INT1_BIT    4
#define PIN_CHG_LED_PORT    GPIOB
#define PIN_CHG_LED_BIT     6
#define PIN_STBY_LED_PORT   GPIOB
#define PIN_STBY_LED_BIT    7

/* ---- IMU (ICM-42688-P) register addresses --------------------------- */
#define IMU_WHO_AM_I        0x75
#define IMU_WHO_AM_I_VAL    0x47
#define IMU_PWR_MGMT0       0x4F
#define IMU_GYRO_CONFIG0    0x4F
#define IMU_ACCEL_CONFIG0   0x50
#define IMU_GYRO_CONFIG1    0x51
#define IMU_ACCEL_CONFIG1   0x53
#define IMU_INT_CONFIG      0x64
#define IMU_INT_CONFIG0     0x63
#define IMU_INT_SOURCE0     0x65
#define IMU_INT_SOURCE1     0x66
#define IMU_FIFO_CONFIG     0x5D
#define IMU_FIFO_COUNTH     0x5E
#define IMU_FIFO_COUNTL     0x5F
#define IMU_FIFO_DATA       0x60
#define IMU_INT_STATUS      0x2D
#define IMU_REG_BANK_SEL    0x76

#define IMU_READ_BIT        0x80
#define IMU_WRITE_BIT       0x00

/* ---- IMU sample rate codes ----------------------------------------- */
#define IMU_ODR_1KHZ        0x07  /* 1 kHz */
#define IMU_ODR_32KHZ       0x01
#define IMU_ODR_12_5HZ      0x0B  /* 12.5 Hz wake-on-motion */

/* ---- Acceleration full-scale ------------------------------------- */
#define IMU_ACCEL_FS_8G     0x02  /* ±8 g, 4096 LSB/g */

/* ---- Gyro full-scale ---------------------------------------------- */
#define IMU_GYRO_FS_2000DPS 0x00  /* ±2000 °/s, 16.4 LSB/°/s */

/* ---- DRV2605L haptic driver ---------------------------------------- */
#define DRV2605L_ADDR       0x5A  /* 7-bit I2C, left-justified 0xB4 */
#define DRV2605L_MODE       0x01
#define DRV2605L_RTP        0x02
#define DRV2605L_WAVESEQ1   0x04
#define DRV2605L_WAVESEQ2   0x05
#define DRV2605L_GO         0x0C

/* ---- MAX17048 fuel gauge ------------------------------------------ */
#define MAX17048_ADDR       0x36  /* 7-bit, left-justified 0x6C */
#define MAX17048_VCELL       0x02
#define MAX17048_SOC         0x04
#define MAX17048_VERSION    0x08

/* ---- Sampling parameters ------------------------------------------ */
#define SAMPLE_RATE_HZ      1000
#define SAMPLE_PERIOD_US    1000
#define IMU_FIFO_WATERMARK  31

/* ---- Stroke segmentation ------------------------------------------ */
#define STROKE_WINDOW_MS    32          /* segmentation window */
#define STROKE_WINDOW_SAMP  (STROKE_WINDOW_MS * SAMPLE_RATE_HZ / 1000)
#define PEN_UP_DWELL_MS     180         /* pen-up dwell */
#define PEN_UP_DWELL_SAMP   (PEN_UP_DWELL_MS * SAMPLE_RATE_HZ / 1000)
#define PEN_UP_VEL_MPS_X1K  180         /* 0.18 m/s in milli-m/s */
#define STROKE_MAX_SAMP     1500        /* 1.5 s cap */
#define STROKE_MIN_SAMP     16          /* reject flicks */

/* ---- Trajectory normalization ------------------------------------- */
#define TRAJ_NPOINTS        64          /* network input length */
#define TRAJ_CHANNELS        3           /* x, y, |v| */
#define N_CLASSES           58

/* ---- BLE HID ------------------------------------------------------ */
#define BLE_HID_REPORT_KEYS 8           /* max keys per HID report */
#define BLE_UART_BAUD       115200

/* ---- Power / sleep ------------------------------------------------ */
#define SLEEP_TIMEOUT_MS    2000         /* go to STOP2 after 2 s idle */
#define WAKEUP_SAMPLE_MIN   1            /* dummy, used by RTC in other build */

/* ---- Default sample interval (kept for API parity) --------------- */
#define DEFAULT_SAMPLE_INTERVAL_MIN  0   /* not used; GlyphFlow is event-driven */

/* ---- GPIO helper macros ------------------------------------------ */
#define GPIO_SET(port, bit)  ((port)->BSRR = (1U << (bit)))
#define GPIO_CLR(port, bit)  ((port)->BSRR = (1U << ((bit) + 16)))
#define GPIO_GET(port, bit)   (((port)->IDR >> (bit)) & 1U)

#endif /* GLYPHFLOW_BOARD_H */