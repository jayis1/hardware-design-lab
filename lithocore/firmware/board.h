/*
 * board.h — STM32G474RET6 pin map, clock configuration, and peripheral
 *            assignments for the LithoCore cell impedance spectrometer.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#ifndef LITHOCORE_BOARD_H
#define LITHOCORE_BOARD_H

#include <stdint.h>
#include <stddef.h>

/* -------------------------------------------------------------------------
 * MCU identity
 * ------------------------------------------------------------------------- */
#define MCU_NAME        "STM32G474RET6"
#define SYSCLK_HZ       170000000U     /* 170 MHz max for G474 */
#define HSE_FREQ_HZ     16384000U      /* 16.384 MHz TCXO — phase-locked DDS+ADC */
#define HCLK_HZ         SYSCLK_HZ
#define PCLK1_HZ        (SYSCLK_HZ / 1)  /* 170 MHz — timers need high rate */
#define PCLK2_HZ        (SYSCLK_HZ / 1)

#define FLASH_SIZE      (512 * 1024)   /* 512 KB */
#define SRAM_SIZE       (128 * 1024)   /* 128 KB */

/* -------------------------------------------------------------------------
 * GPIO pin assignments  (STM32G474RET6, LQFP64)
 *
 *  PA0  — AN_VCELL    (ADC1_IN1)   cell voltage DC monitor
 *  PA1  — AN_ISENSE   (ADC1_IN2)   current-sense TIA output
 *  PA2  — AN_VAC_HI   (ADC1_IN3)   AC voltage sense (high side, post-HPF)
 *  PA3  — AN_VAC_LO   (ADC1_IN4)   AC voltage sense (low side, diff ref)
 *  PA4  — DAC_OUT1                  DC bias DAC for offset trim
 *  PA5  — SPI1_NCS_ADC (GPIO)       ADS1256 chip select
 *  PA6  — SPI1_MISO                 ADS1256 MISO / MCU ADC SPI
 *  PA7  — SPI1_MOSI                 ADS1256 MOSI
 *  PA8  — TIM1_CH1                  DCIR pulse FET gate (2A, 100ms one-shot)
 *  PA9  — USART1_TX                 BLE co-proc UART
 *  PA10 — USART1_RX                 BLE co-proc UART
 *  PA11 — USB_DM                    USB-C data
 *  PA12 — USB_DP                    USB-C data
 *  PA13 — SWDIO                     debug
 *  PA14 — SWCLK                     debug
 *  PA15 — SPI3_NCS_DDS (GPIO)       AD9833 chip select
 *  PB0  — SPI3_SCK                  AD9833 clock
 *  PB1  — SPI3_MOSI                 AD9833 data
 *  PB2  — GPIO  BOOT1               (tied low)
 *  PB3  — GPIO  LED_STATUS1         white LED 1 (idle/ready)
 *  PB4  — GPIO  LED_STATUS2         white LED 2 (sweeping)
 *  PB5  — GPIO  LED_STATUS3         white LED 3 (done)
 *  PB6  — GPIO  LED_STATUS4         white LED 4 (fault)
 *  PB7  — GPIO  LED_GOOD            bi-color: green (good cell)
 *  PB8  — GPIO  LED_BAD             bi-color: red (bad cell)
 *  PB9  — GPIO  BTN_USER            user button (active low, pull-up)
 *  PB10 — USART3_TX                 debug UART (optional)
 *  PB11 — USART3_RX                 debug UART (optional)
 *  PB12 — GPIO  ANALOG_EN           load-switch enable (TPL7407) — analog rail
 *  PB13 — GPIO  DDS_RESET           AD9833 reset
 *  PB14 — GPIO  ADS_DRDY            ADS1256 data-ready interrupt
 *  PB15 — GPIO  SUPERCAP_OK         supercap voltage monitor (comparator out)
 *  PC0  — AN_NTC     (ADC1_IN6)     probe tip temperature
 *  PC1  — AN_VBUS    (ADC1_IN7)     USB VBUS detect
 *  PC2  — GPIO  OVP_FAULT           TLV3201 over-voltage comparator output
 *  PC3  — GPIO  REV_POL             LM74700 reverse-polarity detect
 *  PC4  — GPIO  BLE_CTS             BLE co-processor CTS (wake signal)
 *  PC5  — GPIO  BLE_RTS             BLE co-processor RTS
 *  PC6  — GPIO  BLE_WAKE            wake BLE co-processor
 *  PC7  — GPIO  CAL_SWITCH          calibration resistor relay
 *  PC8  — GPIO  USB_VBUS_EN         USB-C 5V output enable (DFU power)
 *  PC9  — GPIO  nRESET_BLE          BLE module reset
 *  PC10 — SPI3_SCK (alt)            (reserved)
 *  PC11 — GPIO  nCHARGE_EN          supercap charge enable
 *  PC12 — GPIO  FAULT_LATCH         hardware fault latch clear
 *  PC13 — GPIO  nBOOT0              (tied low)
 *  PC14 — GPIO  OSC32_IN            (32.768 kHz RTC crystal)
 *  PC15 — GPIO  OSC32_OUT
 *  PF0  — GPIO  nSS (SPI1 alt)
 *  PF1  — GPIO  (unused)
 * ------------------------------------------------------------------------- */

/* --- Port A --- */
#define PIN_AN_VCELL        0    /* PA0  ADC1_IN1 */
#define PIN_AN_ISENSE       1    /* PA1  ADC1_IN2 */
#define PIN_AN_VAC_HI       2    /* PA2  ADC1_IN3 */
#define PIN_AN_VAC_LO       3    /* PA3  ADC1_IN4 */
#define PIN_DAC_BIAS        4    /* PA4  DAC1_OUT1 */
#define PIN_SPI1_NCS_ADC    5    /* PA5  GPIO */
#define PIN_SPI1_MISO       6    /* PA6  SPI1_MISO */
#define PIN_SPI1_MOSI       7    /* PA7  SPI1_MOSI */
#define PIN_DCIR_GATE       8    /* PA8  TIM1_CH1 */
#define PIN_USART1_TX       9    /* PA9  USART1_TX */
#define PIN_USART1_RX       10   /* PA10 USART1_RX */
#define PIN_USB_DM          11   /* PA11 */
#define PIN_USB_DP          12   /* PA12 */
#define PIN_SWDIO           13   /* PA13 */
#define PIN_SWCLK           14   /* PA14 */
#define PIN_SPI3_NCS_DDS    15   /* PA15 GPIO */

/* --- Port B --- */
#define PIN_SPI3_SCK_DDS    0    /* PB0  GPIO (bit-bang SPI for DDS) */
#define PIN_SPI3_MOSI_DDS   1    /* PB1  GPIO */
#define PIN_LED_STATUS1     3    /* PB3  GPIO */
#define PIN_LED_STATUS2     4    /* PB4  GPIO */
#define PIN_LED_STATUS3     5    /* PB5  GPIO */
#define PIN_LED_STATUS4     6    /* PB6  GPIO */
#define PIN_LED_GOOD        7    /* PB7  GPIO */
#define PIN_LED_BAD         8    /* PB8  GPIO */
#define PIN_BTN_USER        9    /* PB9  GPIO (active low) */
#define PIN_ANALOG_EN       12   /* PB12 GPIO — analog rail power */
#define PIN_DDS_RESET       13   /* PB13 GPIO */
#define PIN_ADS_DRDY        14   /* PB14 GPIO (exti) */
#define PIN_SUPERCAP_OK     15   /* PB15 GPIO */

/* --- Port C --- */
#define PIN_AN_NTC          0    /* PC0  ADC1_IN6 */
#define PIN_AN_VBUS         1    /* PC1  ADC1_IN7 */
#define PIN_OVP_FAULT       2    /* PC2  GPIO (exti) */
#define PIN_REV_POL         3    /* PC3  GPIO */
#define PIN_BLE_CTS         4    /* PC4  GPIO */
#define PIN_BLE_RTS         5    /* PC5  GPIO */
#define PIN_BLE_WAKE        6    /* PC6  GPIO */
#define PIN_CAL_SWITCH      7    /* PC7  GPIO */
#define PIN_USB_VBUS_EN     8    /* PC8  GPIO */
#define PIN_nRESET_BLE      9    /* PC9  GPIO */
#define PIN_CHARGE_EN       11   /* PC11 GPIO */
#define PIN_FAULT_LATCH     12   /* PC12 GPIO */

/* -------------------------------------------------------------------------
 * GPIO mode constants (simplified — no HAL)
 * ------------------------------------------------------------------------- */
#define GPIO_MODE_INPUT      0x00
#define GPIO_MODE_OUTPUT_PP  0x01
#define GPIO_MODE_OUTPUT_OD  0x02
#define GPIO_MODE_ANALOG     0x03
#define GPIO_MODE_AF_PP      0x04

#define GPIO_PULL_NONE       0x00
#define GPIO_PULL_UP         0x01
#define GPIO_PULL_DOWN       0x02

#define GPIO_SPEED_LOW       0x00
#define GPIO_SPEED_HIGH      0x03

/* -------------------------------------------------------------------------
 * Peripheral instances (matching registers.h)
 * ------------------------------------------------------------------------- */
#define ADC1_BASE_ADDR       0x40012400U
#define SPI1_BASE_ADDR       0x40013000U
#define SPI3_BASE_ADDR       0x40003C00U
#define USART1_BASE_ADDR     0x40013800U
#define TIM1_BASE_ADDR       0x40012C00U
#define USB_BASE_ADDR        0x40006800U
#define CORDIC_BASE_ADDR     0x40020C00U
#define FMAC_BASE_ADDR       0x40021000U

/* -------------------------------------------------------------------------
 * Timing constants
 * ------------------------------------------------------------------------- */
#define TICKS_PER_SEC        1000U   /* 1 ms SysTick */
#define SWEEP_FAST_DURATION  20000U  /* 20 s fast sweep */
#define SWEEP_FULL_DURATION  720000U /* 12 min full sweep */
#define DCIR_PULSE_MS        100U    /* DCIR discharge pulse */
#define DCIR_CURRENT_MA      2000U   /* 2 A */
#define EIS_AC_CURRENT_MA    20U     /* 20 mA max AC perturbation */
#define OVP_THRESHOLD_MV     4500U   /* over-voltage cutoff */
#define UVP_THRESHOLD_MV     1500U   /* under-voltage cutoff */
#define TEMP_MAX_mC          60000U  /* 60 °C max cell temp */
#define DEBOUNCE_MS          50U     /* button debounce */
#define LONG_PRESS_MS        1500U   /* long press = full sweep */

/* -------------------------------------------------------------------------
 * System state machine
 * ------------------------------------------------------------------------- */
typedef enum {
    STATE_IDLE = 0,
    STATE_CONNECTING_BLE,
    STATE_SWEEP_FAST,
    STATE_SWEEP_FULL,
    STATE_DCIR_MEASURE,
    STATE_OCV_RELAX,
    STATE_CNLS_FIT,
    STATE_REPORT,
    STATE_FAULT,
    STATE_SLEEP,
} sys_state_t;

/* -------------------------------------------------------------------------
 * Configuration (stored in flash, mirror in RAM)
 * ------------------------------------------------------------------------- */
typedef struct {
    uint8_t  sweep_mode;        /* 0=fast, 1=full, 2=custom */
    uint8_t  chemistry;         /* 0=NMC18650, 1=NMC21700, 2=LFP26650, 3=NCA, 4=LCO */
    uint8_t  auto_chemistry;    /* 1 = auto-detect from OCV */
    uint8_t  led_brightness;   /* 0-3 */
    uint8_t  ble_enabled;
    uint8_t  reserved[3];
    uint32_t ble_baud;
    char     device_name[16];
} litho_config_t;

#define DEFAULT_CONFIG { \
    .sweep_mode = 0, \
    .chemistry = 0, \
    .auto_chemistry = 1, \
    .led_brightness = 2, \
    .ble_enabled = 1, \
    .reserved = {0,0,0}, \
    .ble_baud = 115200, \
    .device_name = "LithoCore-XXXX" \
}

/* -------------------------------------------------------------------------
 * Chemistry baseline table (indexed by litho_config_t.chemistry)
 * ------------------------------------------------------------------------- */
typedef struct {
    const char name[12];
    uint16_t nominal_mv;    /* nominal voltage mV */
    uint16_t capacity_mah;  /* nominal capacity mAh */
    uint16_t rs_mohm;       /* baseline Rs in mΩ */
    uint16_t rsei_mohm;     /* baseline Rsei in mΩ */
    uint16_t rct_mohm;      /* baseline Rct in mΩ */
    uint16_t cdl_mF;        /* baseline Cdl in mF */
} chemistry_baseline_t;

extern const chemistry_baseline_t chemistry_table[5];

/* -------------------------------------------------------------------------
 * LED helpers
 * ------------------------------------------------------------------------- */
#define LED_ON(port, pin)     ((volatile uint32_t *)(port))->BSRR = (1U << (pin))
#define LED_OFF(port, pin)    ((volatile uint32_t *)(port))->BSRR = (1U << ((pin) + 16))

/* -------------------------------------------------------------------------
 * Bit manipulation helpers
 * ------------------------------------------------------------------------- */
#define BIT(n)            (1U << (n))
#define ARRAY_LEN(a)      (sizeof(a) / sizeof((a)[0]))
#define MIN(a,b)          ((a) < (b) ? (a) : (b))
#define MAX(a,b)          ((a) > (b) ? (a) : (b))

#endif /* LITHOCORE_BOARD_H */