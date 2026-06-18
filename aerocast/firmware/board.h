/*
 * board.h — AeroCast board pin map and constants
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 *
 * STM32H7A3VIT6 (UFBGA176) pin assignments for the AeroCast
 * bioaerosol identification station.
 */
#ifndef AEROCAST_BOARD_H
#define AEROCAST_BOARD_H

#include <stdint.h>
#include <stddef.h>
#include "registers.h"

/* ---- GPIO port base pointers (typed as volatile uint32_t* in registers.h) ---- */
#define PORTA       GPIOA_BASE
#define PORTB       GPIOB_BASE
#define PORTC       GPIOC_BASE
#define PORTD       GPIOD_BASE
#define PORTE       GPIOE_BASE
#define PORTH       GPIOH_BASE

/* ---- Clock tree ---- */
#define HSI_FREQ    64000000UL
#define PLL_VCO     400000000UL
#define SYSCLK      280000000UL
#define HCLK        280000000UL
#define APB1_CLK    140000000UL
#define APB2_CLK    140000000UL
#define APB4_CLK    140000000UL

/* ---- Pin map (port, pin, afn, label) ---- */
typedef struct {
    volatile uint32_t *port;
    uint8_t  pin;
    uint8_t  afn;
    const char *label;
} pin_t;

#define PIN(_p,_n,_a) { .port = PORT##_p, .pin = _n, .afn = _a, .label = #_p#_n }

/* SPI1: ADC (LTC2351-14) — 18 MHz, 4-channel simultaneous */
#define ADC_SPI          SPI1_BASE
#define PIN_ADC_SCK      PIN(A, 5, 5)
#define PIN_ADC_MISO     PIN(B, 4, 5)
#define PIN_ADC_MOSI     PIN(B, 5, 5)   /* unused by LTC2351 but wired for diagnostics */
#define PIN_ADC_CS       PIN(C, 8, 0)   /* manual CS */
#define PIN_ADC_CONV     PIN(C, 9, 0)   /* CONVST pulse */
#define PIN_ADC_BUSY     PIN(C, 10, 0)  /* BUSY IRQ input */

/* SPI2: eMMC / microSD (SDMMC1 actually, but share SDMMC1) */
#define SDMMC_DEV        SDMMC1_BASE
#define PIN_SD_CMD       PIN(C, 12, 12)
#define PIN_SD_CK        PIN(D, 2, 12)
#define PIN_SD_D0        PIN(C, 8, 12)  /* remapped on H7A3 */
#define PIN_SD_D1        PIN(C, 9, 12)
#define PIN_SD_D2        PIN(C, 10, 12)
#define PIN_SD_D3        PIN(C, 11, 12)
#define PIN_SD_CD        PIN(B, 15, 0)

/* SPI3: display ST7789V */
#define DISP_SPI         SPI3_BASE
#define PIN_DISP_SCK     PIN(C, 10, 6)
#define PIN_DISP_MOSI    PIN(C, 12, 6)
#define PIN_DISP_CS      PIN(C, 13, 0)
#define PIN_DISP_DC      PIN(C, 14, 0)
#define PIN_DISP_RST     PIN(C, 15, 0)
#define PIN_DISP_BL      PIN(D, 0, 0)

/* USART2: ESP32-C6 AT bridge (3V3 logic, 2 Mbps) */
#define ESP_UART         USART2_BASE
#define PIN_ESP_TX       PIN(A, 2, 7)
#define PIN_ESP_RX       PIN(A, 3, 7)
#define PIN_ESP_RST      PIN(A, 6, 0)
#define PIN_ESP_BOOT     PIN(A, 7, 0)

/* USART3: RS-485 anemometer (9600 baud) */
#define WIND_UART        USART3_BASE
#define PIN_WIND_TX      PIN(B, 10, 7)
#define PIN_WIND_RX      PIN(B, 11, 7)
#define PIN_WIND_DE      PIN(B, 12, 0)  /* driver-enable for RS-485 transceiver */

/* I2C1: SHT45 + BMP390 */
#define ENV_I2C          I2C1_BASE
#define PIN_I2C_SCL      PIN(B, 6, 4)
#define PIN_I2C_SDA      PIN(B, 7, 4)

/* I2C4: SDP810 flow sensor */
#define FLOW_I2C         I2C4_BASE
#define PIN_FLOW_SCL     PIN(B, 8, 2)
#define PIN_FLOW_SDA     PIN(B, 9, 2)

/* TIM2 CH1: blower PWM (20 kHz) */
#define BLOWER_TIM       TIM2_BASE
#define PIN_BLOWER_PWM   PIN(A, 15, 1)

/* TIM3 CH1: UV-LED pulse (340 nm) — gated by particle events */
#define UV_TIM           TIM3_BASE
#define PIN_UV_PWM       PIN(B, 0, 2)

/* DAC1 OUT1: PMT bias setpoint (0–3.3 V → 0–900 V via Cockcroft-Walton) */
#define PMT_DAC          DAC1_BASE
#define PIN_PMT_BIAS     PIN(A, 4, 0)   /* analog, no AF */

/* DAC1 OUT2: laser drive current setpoint */
#define LASER_DAC        DAC1_BASE      /* same DAC, channel 2 */
#define PIN_LASER_ISET   PIN(A, 5, 0)   /* analog */

/* ADC1 IN5: laser monitor photodiode (internal ADC, 12b) */
#define MON_ADC          ADC1_BASE
#define PIN_LASER_MON    PIN(A, 5, 0)   /* repurposed when DAC2 not in use */

/* Lid interlock switch (active-low) */
#define PIN_LID_SW       PIN(C, 0, 0)

/* User button (active-low, on rear) */
#define PIN_BTN          PIN(C, 1, 0)

/* Status LED (dual-color, green/red) */
#define PIN_LED_G        PIN(B, 1, 0)
#define PIN_LED_R        PIN(B, 2, 0)

/* ---- Sampling / flow constants ---- */
#define TARGET_FLOW_LPM      15.0f          /* target sample flow, L/min */
#define FLOW_KP              0.8f           /* PI proportional gain */
#define FLOW_KI              0.05f          /* PI integral gain */
#define FLOW_I_LIMIT         4095.0f        /* blower PWM clamp */
#define SAMPLE_PERIOD_MS     60000UL        /* 1-minute binning */
#define EVENT_BUF_SIZE       256            /* particle-event ring slots */

/* ---- Classifier constants ---- */
#define N_CLASSES            24
#define N_FEATURES           4
#define KNN_K                12
#define REF_TABLE_LEN        1968
#define COARSE_GATE_NODES    20

/* ---- Comms ---- */
#define MQTT_TOPIC_EVENTS    "aerocast/%s/events"
#define MQTT_TOPIC_FORECAST  "aerocast/%s/forecast"
#define MQTT_TOPIC_CMD       "aerocast/%s/cmd"
#define BLE_SERVICE_UUID     0xFE5A

/* ---- Storage ---- */
#define LOG_FILE_PREFIX      "/aerocast/events_"
#define CALIB_FILE           "/aerocast/calib.bin"

/* ---- Version ---- */
#define FW_VERSION_MAJOR     1
#define FW_VERSION_MINOR     0
#define FW_VERSION_PATCH     0
#define FW_AUTHOR            "jayis1"
#define FW_COPYRIGHT         "Copyright (C) 2026 jayis1"

#endif /* AEROCAST_BOARD_H */