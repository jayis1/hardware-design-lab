/*
 * board.h — FloraPulse Board Configuration & Pin Assignments
 * FloraPulse — Plant Electrophysiology & Sap-Flow Stress Monitor
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Pin assignments, clock configuration, and physical constants for the
 * FloraPulse PCB (80mm × 50mm).  All pin definitions correspond to the
 * KiCad schematic in kicad/device.kicad_sch.
 */

#ifndef FLORAPULSE_BOARD_H
#define FLORAPULSE_BOARD_H

#include <stdint.h>
#include "registers.h"

/* ===================================================================== */
/*  Clock configuration                                                   */
/* ===================================================================== */

/* STM32L476: HSI 16 MHz → PLL → 80 MHz SYSCLK
 * PLL: source = HSI16, M=1, N=10, R=2 → VCO=160 MHz, SYSCLK=80 MHz
 * APB1 = 80 MHz, APB2 = 80 MHz (no prescaling on L4 for max peripheral speed)
 */
#define BOARD_HSI_FREQ_HZ       16000000ULL
#define BOARD_SYSCLK_FREQ_HZ     80000000ULL
#define BOARD_HCLK_FREQ_HZ       80000000ULL
#define BOARD_APB1_FREQ_HZ       80000000ULL
#define BOARD_APB2_FREQ_HZ       80000000ULL

/* PLL configuration */
#define BOARD_PLL_M             1
#define BOARD_PLL_N             10
#define BOARD_PLL_R             2

/* LSE 32.768 kHz for RTC */
#define BOARD_LSE_FREQ_HZ       32768ULL

/* TIM1 clock = APB2 × 2 (if APB2 prescaler != 1) → 160 MHz
 * With APB2 prescaler = 1 (no division), TIM1CLK = APB2 = 80 MHz */
#define BOARD_TIM1_CLK_HZ       80000000ULL
#define BOARD_TIM2_CLK_HZ       80000000ULL
#define BOARD_TIM3_CLK_HZ       80000000ULL

/* TIM1 — Sap-flow heater pulse
 * 1 Hz repetition, 4 s pulse → one-shot mode
 * ARR = 80M / 1 = 80,000,000 (period 1 s)
 * CCR1 = 4 × (80M/1) = 320,000,000? No — we use OPM with ARR = pulse_width
 */
#define BOARD_HEATER_PERIOD_MS  60000UL   /* 1-minute cycle */
#define BOARD_HEATER_PULSE_MS   4000UL    /* 4-second heat pulse */
#define BOARD_HEATER_ARR         (BOARD_TIM1_CLK_HZ / 1000U) * BOARD_HEATER_PULSE_MS

/* TIM2 — System tick (1 Hz wakeup) */
#define BOARD_SAMPLE_PERIOD_S   1U       /* Default: 1 sample/second */
#define BOARD_TIM2_ARR          (BOARD_TIM2_CLK_HZ - 1U)

/* ===================================================================== */
/*  Pin assignments (STM32L476RGT6, 64-pin LQFP)                        */
/* ===================================================================== */

/* SPI1 — ADS1292 24-bit biopotential ADC (plant action potentials)
 *   PA5  → SPI1_SCK   (AF5)
 *   PA6  → SPI1_MISO  (AF5)
 *   PA7  → SPI1_MOSI  (AF5)
 *   PB0  → ADS1292_CS (GPIO output, active low)
 *   PB1  → ADS1292_DRDY (EXTI1, falling edge, GPIO input)
 *   PB2  → ADS1292_START (GPIO output)
 *   PB10 → ADS1292_PWDN (GPIO output)
 */
#define ADS_CS_PORT       GPIOB_BASE
#define ADS_CS_PIN        0
#define ADS_DRDY_PORT     GPIOB_BASE
#define ADS_DRDY_PIN      1
#define ADS_START_PORT    GPIOB_BASE
#define ADS_START_PIN     2
#define ADS_PWDN_PORT     GPIOB_BASE
#define ADS_PWDN_PIN      10

/* SPI2 — ADS1220 thermistor ADC + microSD (chip-select-multiplexed)
 *   PB13 → SPI2_SCK   (AF5)
 *   PB14 → SPI2_MISO  (AF5)
 *   PB15 → SPI2_MOSI  (AF5)
 *   PB4  → ADS1220_CS (GPIO output, active low)
 *   PB5  → ADS1220_DRDY (EXTI5, falling edge)
 *   PB6  → SD_CS      (GPIO output, active low)
 */
#define ADS1220_CS_PORT    GPIOB_BASE
#define ADS1220_CS_PIN     4
#define ADS1220_DRDY_PORT  GPIOB_BASE
#define ADS1220_DRDY_PIN   5
#define SD_CS_PORT         GPIOB_BASE
#define SD_CS_PIN          6

/* I2C1 — BME280 + APDS-9301 + MAX17048
 *   PB8  → I2C1_SCL (AF4)
 *   PB9  → I2C1_SDA (AF4)
 */
#define I2C1_SCL_PORT     GPIOB_BASE
#define I2C1_SCL_PIN      8
#define I2C1_SDA_PORT     GPIOB_BASE
#define I2C1_SDA_PIN      9
/* I2C1 TIMINGR: 100 kHz at 80 MHz PCLK
 * PRESC=1, SCLL=0x13, SCLH=0x0F, SDADEL=0x2, SCLDEL=0x4
 */
#define I2C1_TIMINGR_VAL  0x00201D2BU

/* USART1 — nRF52840 BLE module (115200 baud)
 *   PA9  → USART1_TX (AF7)
 *   PA10 → USART1_RX (AF7)
 *   PA11 → nRF52840_CTS (AF7)
 *   PA12 → nRF52840_RTS (AF7)
 */
#define BLE_USART_BASE    USART1_BASE
#define BLE_BAUD          115200U
#define BLE_BRR_VAL       (BOARD_APB2_FREQ_HZ / BLE_BAUD)

/* TIM1_CH1 — Heater PWM output (PC0 → heater MOSFET gate driver)
 *   PC0  → TIM1_CH1 (AF2 on L4)
 *   PC1  → HEATER_ENABLE (GPIO output, enables MOSFET gate driver)
 */
#define HEATER_PWM_PORT   GPIOC_BASE
#define HEATER_PWM_PIN    0
#define HEATER_EN_PORT    GPIOC_BASE
#define HEATER_EN_PIN     1

/* TIM3_CH1 — Leaf-wetness capacitive sensor (frequency counter via PA6)
 *   PA6  → TIM3_CH1 (AF2, input capture for frequency measurement)
 *   Note: PA6 is shared with SPI1_MISO on L4; use TIM3_ETR on PD2 instead.
 *   PD2  → TIM3_ETR (AF2, external trigger input for frequency counting)
 *   PC2  → LW_SENSOR_DRIVE (GPIO output, toggles 555 oscillator enable)
 */
#define LW_ETR_PORT       GPIOD_BASE
#define LW_ETR_PIN        2
#define LW_DRIVE_PORT     GPIOC_BASE
#define LW_DRIVE_PIN      2

/* SX1276 LoRa DIO pins (interrupts)
 *   PC3  → SX1276_DIO0 (EXTI3, rising edge, RX/TX done)
 *   PC4  → SX1276_DIO1 (EXTI4, rising edge, RX timeout)
 *   PC5  → SX1276_RST (GPIO output)
 */
#define LORA_DIO0_PORT    GPIOC_BASE
#define LORA_DIO0_PIN     3
#define LORA_DIO1_PORT    GPIOC_BASE
#define LORA_DIO1_PIN     4
#define LORA_RST_PORT     GPIOC_BASE
#define LORA_RST_PIN      5

/* Status indicators
 *   PA8  → LED_STATUS (GPIO output, active low, green)
 *   PA15 → LED_ALERT  (GPIO output, active low, red)
 *   PC10 → LED_TX     (GPIO output, active low, blue, BLE activity)
 *   PC11 → LED_LORA   (GPIO output, active low, yellow, LoRa activity)
 */
#define LED_STATUS_PORT   GPIOA_BASE
#define LED_STATUS_PIN    8
#define LED_ALERT_PORT    GPIOA_BASE
#define LED_ALERT_PIN     15
#define LED_TX_PORT       GPIOC_BASE
#define LED_TX_PIN        10
#define LED_LORA_PORT     GPIOC_BASE
#define LED_LORA_PIN      11

/* User button (capacitive touch — but also a physical button for boot)
 *   PC12 → BUTTON_USER (GPIO input, pull-up, EXTI12 falling edge)
 */
#define BTN_USER_PORT     GPIOC_BASE
#define BTN_USER_PIN      12

/* USB-C (J1) — for DFU / serial console
 *   PA11 → USB_DM (AF10)
 *   PA12 → USB_DP (AF10)
 *   (These conflict with USART1 RTS/CTS; we use BLE without HW flow control.)
 */
#define USB_DM_PIN        11
#define USB_DP_PIN        12

/* Battery sense (internal ADC)
 *   PC0_ALT → actually use ADC1_IN1 on PA1 for battery voltage divider
 *   PA1  → BATT_SENSE (ADC analog input)
 */
#define BATT_SENSE_PORT   GPIOA_BASE
#define BATT_SENSE_PIN    1
#define BATT_DIVIDER_RATIO 2.0f  /* R1=R2=100k → 2:1 */

/* ===================================================================== */
/*  Physical / sensor constants                                           */
/* ===================================================================== */

/* ADS1292: plant action potentials range ±2.5 mV typical
 * Internal reference 2.42 V, PGA gain 12 → LSB = 2.42 / (12 × 2^24)
 *                     = 12.0 nV per count
 * At gain 12, full-scale = ±2.42V/12 ≈ ±201 mV → plenty of headroom
 */
#define BOARD_ADS1292_VREF_MV    2420.0f
#define BOARD_ADS1292_GAIN       12
#define BOARD_ADS1292_LSB_NV     (BOARD_ADS1292_VREF_MV * 1e6f / \
                                  ((float)BOARD_ADS1292_GAIN * 8388608.0f))

/* Sap-flow calibration: heat pulse velocity (cm/h)
 * Heat pulse velocity vh = (k / (ρ * c)) * ln(t2/t1) / (x / 2)
 * where:
 *   k = thermal conductivity of wood (0.542 W/m·K)
 *   t1, t2 = temperatures at probes x1 and x2 (cm above heater)
 *   x = probe spacing (0.6 cm typical)
 * We compute the dimensionless ratio in firmware; app does the final
 * conversion to g/h using the stem cross-sectional area.
 */
#define BOARD_HEATER_PROBE_SPACING_CM  0.6f
#define BOARD_HEATER_UP_DIST_CM        0.6f
#define BOARD_HEATER_DOWN_DIST_CM      0.6f
#define BOARD_WOOD_THERMAL_DIFFUSIVITY 2.5e-7f  /* m²/s, typical */

/* Leaf-wetness sensor: capacitive divider
 * 555 oscillator frequency = 1 / (0.693 * (R + 2*R2) * C)
 * C = 100 pF dry → ~10 kHz
 * C = 200 pF wet → ~5 kHz
 */
#define BOARD_LW_FREQ_DRY_HZ    10000U
#define BOARD_LW_FREQ_WET_HZ     5000U
#define BOARD_LW_GATE_MS         100U   /* Measurement gate time */

/* Battery: Li-ion 3.7V nominal, 18650 2200 mAh */
#define BOARD_BATT_NOMINAL_MV    3700
#define BOARD_BATT_FULL_MV       4200
#define BOARD_BATT_EMPTY_MV      3300
#define BOARD_BATT_CAPACITY_MAH  2200

/* ===================================================================== */
/*  Data logging format                                                    */
/* ===================================================================== */

/* Binary log record: 48 bytes per sample (every 1 s)
 * Structure (packed, little-endian):
 *  [0:3]    uint32  timestamp_s (Unix time from RTC)
 *  [4:7]    int32   ap_ch1_uv  (plant action potential, channel 1, µV)
 *  [8:11]   int32   ap_ch2_uv  (plant action potential, channel 2, µV)
 *  [12:15]  int32   t_upper_c  (upper sap-flow thermistor, °C × 100)
 *  [16:19]  int32   t_lower_c  (lower sap-flow thermistor, °C × 100)
 *  [20:23]  int16   t_ambient_c (BME280 temperature, °C × 100)
 *  [24:25]  uint16  rh_pct      (BME280 relative humidity, % × 100)
 *  [26:29]  uint32  pressure_pa (BME280 pressure, Pa)
 *  [30:33]  uint32  lux          (APDS-9301 illuminance, lux)
 *  [34:35]  uint16  leaf_wet_raw (leaf-wetness frequency, Hz)
 *  [36:37]  uint16  sap_flow_vh  (heat pulse velocity, cm/h × 10)
 *  [38:39]  uint16  ap_rms_uv   (AP RMS amplitude over last second, µV)
 *  [40:41]  uint16  ap_rate_hz   (AP firing rate, events/s × 10)
 *  [42]     uint8   batt_pct     (battery state of charge, %)
 *  [43]     uint8   flags        (bit0: heater pulse, bit1: BLE connected,
 *                                 bit2: LoRa TX, bit3: anomaly detected,
 *                                 bit4: GPS fix (unused), bit5: SD full)
 *  [44:47]  uint32  reserved    (alignment / future use)
 */
#define LOG_RECORD_SIZE   48U

/* ===================================================================== */
/*  BLE protocol framing                                                  */
/* ===================================================================== */

#define BLE_FRAME_SOF     0xAAU
#define BLE_FRAME_EOF     0x55U
#define BLE_MAX_PAYLOAD    128U
#define BLE_MTU           247U

/* BLE command opcodes */
#define BLE_CMD_PING           0x01U
#define BLE_CMD_GET_STATUS     0x02U
#define BLE_CMD_GET_SAMPLE     0x03U
#define BLE_CMD_START_STREAM   0x04U
#define BLE_CMD_STOP_STREAM    0x05U
#define BLE_CMD_START_CALIB    0x06U
#define BLE_CMD_GET_CALIB      0x07U
#define BLE_CMD_SET_RATE       0x08U
#define BLE_CMD_GET_RATE       0x09U
#define BLE_CMD_SET_THRESH     0x0AU
#define BLE_CMD_GET_THRESH     0x0BU
#define BLE_CMD_DOWNLOAD_LOG   0x0CU
#define BLE_CMD_ERASE_LOG      0x0DU
#define BLE_CMD_SET_TIME       0x0EU
#define BLE_CMD_GET_VERSION    0x0FU
#define BLE_CMD_GET_WAVEFORM   0x10U
#define BLE_CMD_TRIGGER_HEATER 0x11U

/* BLE response codes */
#define BLE_RSP_OK             0x80U
#define BLE_RSP_ERROR           0x81U
#define BLE_RSP_DATA            0x82U
#define BLE_RSP_STREAM          0x83U
#define BLE_RSP_LOG_CHUNK      0x84U
#define BLE_RSP_CALIB_DATA     0x85U
#define BLE_RSP_STATUS         0x86U
#define BLE_RSP_VERSION         0x87U
#define BLE_RSP_WAVEFORM       0x88U

/* ===================================================================== */
/*  LoRa protocol (uplink packets to gateway)                            */
/* ===================================================================== */

#define LORA_FREQ_HZ           868000000ULL  /* EU 868 MHz ISM band */
#define LORA_BW_KHZ            125
#define LORA_SF                12   /* Spreading factor 12 (max range) */
#define LORA_CR                5    /* Coding rate 4/5 */
#define LORA_TX_POWER_DBM      14
#define LORA_PREAMBLE_LEN       8
#define LORA_SYNC_WORD         0x34U  /* Private network */

/* Uplink packet structure: 24 bytes
 *  [0]     uint8  device_id
 *  [1:4]   uint32 timestamp_s
 *  [5:6]   int16  ap_uv_avg
 *  [7:8]   uint16 sap_flow_vh
 *  [9:10]  int16  temp_c
 *  [11:12] uint16 rh_pct
 *  [13:14] uint16 pressure_pa_lo
 *  [15:16] uint16 lux
 *  [17]    uint8  leaf_wet_pct
 *  [18]    uint8  batt_pct
 *  [19]    uint8  flags
 *  [20:21] uint16 ap_rate_hz
 *  [22:23] uint16 crc16
 */
#define LORA_PKT_SIZE         24U
#define LORA_TX_INTERVAL_S    300U   /* 5-minute uplink interval */

/* ===================================================================== */
/*  Firmware version                                                      */
/* ===================================================================== */

#define FW_VERSION_MAJOR   1
#define FW_VERSION_MINOR   0
#define FW_VERSION_PATCH   0
#define FW_VERSION_STRING  "1.0.0"
#define FW_BUILD_DATE      "2026-06-18"
#define FW_AUTHOR          "jayis1"

/* ===================================================================== */
/*  Operating modes (state machine)                                      */
/* ===================================================================== */

typedef enum {
    MODE_BOOT = 0,
    MODE_IDLE,
    MODE_MONITOR,     /* Real-time streaming via BLE */
    MODE_LOGGING,     /* SD card logging only */
    MODE_CALIB,       /* Sensor calibration */
    MODE_STREAM,      /* BLE waveform streaming */
    MODE_LORA,        /* LoRa uplink mode (low-power) */
    MODE_SLEEP,       /* Deep sleep between samples */
    MODE_SHUTDOWN,
} flora_mode_t;

/* ===================================================================== */
/*  Inline helpers: atomic GPIO set/reset                                */
/* ===================================================================== */

static inline void gpio_set(uint32_t port, uint8_t pin)
{
    GPIO_REG(port, GPIO_BSRR) = (1U << pin);
}

static inline void gpio_reset(uint32_t port, uint8_t pin)
{
    GPIO_REG(port, GPIO_BSRR) = (1U << (pin + 16));
}

static inline uint8_t gpio_read(uint32_t port, uint8_t pin)
{
    return (GPIO_REG(port, GPIO_IDR) >> pin) & 1U;
}

#endif /* FLORAPULSE_BOARD_H */