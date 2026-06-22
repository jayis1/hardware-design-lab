/*
 * board.h — Pin assignments, peripheral mappings, and clock configuration
 * BreathPrint — Portable Breath VOC Analyzer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */

#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>
#include <stddef.h>
#include "registers.h"

/* ========================================================================
 * System Clock Configuration
 * STM32H733 @ 312 MHz from 16 MHz HSE × PLL
 *   PLL: M=4, N=156, P=2, Q=13, R=13
 *   SYSCLK = 16/4 * 156 / 2 = 312 MHz
 *   AHB = 312 MHz, APB1 = 156 MHz, APB2 = 156 MHz
 * ======================================================================== */

#define SYSTEM_CLOCK_HZ     312000000UL
#define APB1_CLOCK_HZ       156000000UL
#define APB2_CLOCK_HZ       156000000UL
#define AHB_CLOCK_HZ        312000000UL

#define HSE_FREQ_HZ         16000000UL
#define PLL_M               4
#define PLL_N               156
#define PLL_P               2
#define PLL_Q               13
#define PLL_R               13

#define FLASH_LATENCY       2   /* VOS0 @ 312 MHz needs 2 WS */

/* ========================================================================
 * GPIO Pin Assignments
 * ======================================================================== */

/* --- I2C1 Bus (Sensor Array) --- */
/* PB6  -> I2C1_SCL (AF4)    PB7  -> I2C1_SDA (AF4) */
#define I2C1_SCL_PORT       GPIOB
#define I2C1_SCL_PIN        6
#define I2C1_SCL_AF         AF4_I2C
#define I2C1_SDA_PORT       GPIOB
#define I2C1_SDA_PIN        7
#define I2C1_SDA_AF         AF4_I2C

/* --- I2C2 Bus (Auxiliary) --- */
/* PB10 -> I2C2_SCL (AF4)    PB11 -> I2C2_SDA (AF4) */
#define I2C2_SCL_PORT       GPIOB
#define I2C2_SCL_PIN        10
#define I2C2_SCL_AF         AF4_I2C
#define I2C2_SDA_PORT       GPIOB
#define I2C2_SDA_PIN        11
#define I2C2_SDA_AF         AF4_I2C

/* --- SPI1 (microSD Card) --- */
/* PA5  -> SPI1_SCK (AF5)    PA6  -> SPI1_MISO (AF5)    PA7  -> SPI1_MOSI (AF5) */
#define SD_SPI              SPI1
#define SD_SCK_PORT         GPIOA
#define SD_SCK_PIN          5
#define SD_MISO_PORT        GPIOA
#define SD_MISO_PIN         6
#define SD_MOSI_PORT        GPIOA
#define SD_MOSI_PIN         7
#define SD_CS_PORT          GPIOA
#define SD_CS_PIN           4
#define SD_SCK_AF           AF5_SPI
#define SD_MISO_AF          AF5_SPI
#define SD_MOSI_AF          AF5_SPI

/* --- SPI2 (OLED Display - SSD1306) --- */
/* PB13 -> SPI2_SCK (AF5)    PB14 -> SPI2_MISO (AF5, unused)   PB15 -> SPI2_MOSI (AF5) */
#define DISP_SPI            SPI2
#define DISP_SCK_PORT       GPIOB
#define DISP_SCK_PIN        13
#define DISP_MOSI_PORT      GPIOB
#define DISP_MOSI_PIN       15
#define DISP_CS_PORT        GPIOB
#define DISP_CS_PIN         12
#define DISP_DC_PORT        GPIOB
#define DISP_DC_PIN         5
#define DISP_RST_PORT       GPIOB
#define DISP_RST_PIN         4
#define DISP_SCK_AF         AF5_SPI
#define DISP_MOSI_AF        AF5_SPI

/* --- SPI4 (External Flash W25Q64) --- */
/* PE2 -> SPI4_SCK (AF5)    PE5 -> SPI4_MISO (AF5)    PE6 -> SPI4_MOSI (AF5) */
#define FLASH_SPI           SPI4
#define FLASH_SCK_PORT      GPIOE
#define FLASH_SCK_PIN       2
#define FLASH_MISO_PORT     GPIOE
#define FLASH_MISO_PIN      5
#define FLASH_MOSI_PORT     GPIOE
#define FLASH_MOSI_PIN      6
#define FLASH_CS_PORT       GPIOE
#define FLASH_CS_PIN        4
#define FLASH_SCK_AF        AF5_SPI
#define FLASH_MISO_AF       AF5_SPI
#define FLASH_MOSI_AF       AF5_SPI

/* --- USART2 (BLE Module SPBTLE-RC) --- */
/* PA2 -> USART2_TX (AF7)    PA3 -> USART2_RX (AF7) */
#define BLE_USART           USART2
#define BLE_TX_PORT         GPIOA
#define BLE_TX_PIN          2
#define BLE_RX_PORT         GPIOA
#define BLE_RX_PIN          3
#define BLE_TX_AF           AF7_USART
#define BLE_RX_AF           AF7_USART
#define BLE_RST_PORT        GPIOA
#define BLE_RST_PIN         8
#define BLE_CS_PORT         GPIOA
#define BLE_CS_PIN          9   /* SPI CS if SPI mode used */

/* --- USART3 (SenseAir S8 CH4 NDIR) --- */
/* PC10 -> USART3_TX (AF7)    PC11 -> USART3_RX (AF7) */
#define CH4_USART           USART3
#define CH4_TX_PORT         GPIOC
#define CH4_TX_PIN          10
#define CH4_RX_PORT         GPIOC
#define CH4_RX_PIN          11
#define CH4_TX_AF           AF7_USART
#define CH4_RX_AF           AF7_USART

/* --- USART1 (USB-C serial / debug console) --- */
/* PA9  -> USART1_TX (AF7)    PA10 -> USART1_RX (AF7) */
#define DBG_USART           USART1
#define DBG_TX_PORT         GPIOA
#define DBG_TX_PIN          9
#define DBG_RX_PORT         GPIOA
#define DBG_RX_PIN          10
#define DBG_TX_AF           AF7_USART
#define DBG_RX_AF           AF7_USART

/* --- ADC1 Channel for H2 Sensor (Membrapor H2-500) --- */
/* PC0 -> ADC1_INP0 / INN0 (differential or single-ended) */
#define H2_ADC              ADC1
#define H2_ADC_PORT         GPIOC
#define H2_ADC_PIN          0
#define H2_ADC_CHANNEL      0

/* --- ADC1 Channel for Battery Voltage --- */
/* PC1 -> ADC1_INP1 */
#define BAT_ADC_CHANNEL     1
#define BAT_ADC_PORT        GPIOC
#define BAT_ADC_PIN         1

/* --- ADC1 Channel for Pressure Sensor analog (backup) --- */
/* PC2 -> ADC1_INP2 */
#define AUX_ADC_CHANNEL     2

/* --- User Button (capacitive touch) --- */
/* PA0 -> TIM2_CH1 / EXTI0 (capacitive touch button) */
#define BTN_PORT            GPIOA
#define BTN_PIN             0
#define BTN_EXTI_IRQ        IRQn_EXTI0

/* --- Power/Reset Button --- */
/* PC13 -> EXTI13 (tactile switch) */
#define PWR_BTN_PORT        GPIOC
#define PWR_BTN_PIN         13

/* --- Status LED (NeoPixel / WS2812B) --- */
/* PB1 -> TIM3_CH4 (PWM for WS2812B data) */
#define LED_PORT            GPIOB
#define LED_PIN             1
#define LED_TIM             TIM3
#define LED_TIM_CH          4

/* --- Sample-in-Progress White LED --- */
/* PB2 -> GPIO output */
#define SAMPLE_LED_PORT     GPIOB
#define SAMPLE_LED_PIN      2

/* --- Sensor Power Load Switch --- */
/* PB8 -> GPIO output (enable sensor power domain VSENS) */
#define SENS_PWR_PORT       GPIOB
#define SENS_PWR_PIN        8

/* --- Exhaust Pump Control --- */
/* PB9 -> GPIO output (enable micro-pump for chamber flush) */
#define PUMP_PORT           GPIOB
#define PUMP_PWR_PIN        9

/* --- Charger Status --- */
/* PA15 -> GPIO input (MCP73831 STAT pin) */
#define CHRG_STAT_PORT      GPIOA
#define CHRG_STAT_PIN       15

/* --- USB-C --- */
/* PA11 -> USB_DM    PA12 -> USB_DP */
#define USB_DM_PORT        GPIOA
#define USB_DM_PIN         11
#define USB_DP_PORT        GPIOA
#define USB_DP_PIN         12

/* ========================================================================
 * Timing Constants
 * ======================================================================== */

#define SAMPLE_RATE_HZ      20      /* Sensor sampling rate */
#define SAMPLE_PERIOD_MS    (1000 / SAMPLE_RATE_HZ)
#define WARMUP_DURATION_MS  5000    /* Sensor heater warmup */
#define SAMPLE_DURATION_MS  15000   /* Total sample window */
#define ANALYZE_TIMEOUT_MS  100     /* ML inference timeout */
#define RESULT_DISPLAY_MS   15000   /* Show result for 15s */
#define IDLE_AMBIENT_MS     1800000 /* 30 min between ambient baselines */

#define SAMPLE_COUNT        (SAMPLE_DURATION_MS / SAMPLE_PERIOD_MS) /* 300 */

/* ========================================================================
 * Breath Analysis Constants
 * ======================================================================== */

#define NUM_SENSORS         8       /* Total sensor channels */
#define NUM_FEATURES        40      /* 5 features × 8 sensors */
#define NUM_CLASSES         6       /* Metabolic states + unknown */

/* Metabolic state classes */
#define STATE_BASELINE      0
#define STATE_KETOTIC       1
#define STATE_POSTMEAL      2
#define STATE_POSTEXERCISE  3
#define STATE_GUT_FERMENT   4
#define STATE_UNKNOWN       5

/* Breath quality flags */
#define BREATH_VALID        0
#define BREATH_DEAD_SPACE   1
#define BREATH_CONTAMINATED 2
#define BREATH_RETRY        3

/* CO2 threshold for alveolar breath (ppm) */
#define CO2_ALVEOLAR_MIN    35000   /* 3.5% = 35000 ppm */
#define CO2_AMBIENT         420     /* Ambient ~420 ppm */

/* ========================================================================
 * BLE GATT Profile Constants
 * ======================================================================== */

#define BLE_SERVICE_UUID0   0x1B
#define BLE_SERVICE_UUID1   0x7F

/* Characteristic UUID suffixes */
#define BLE_CHAR_CMD        0x0001  /* Write: commands from phone */
#define BLE_CHAR_SAMPLE     0x0002  /* Notify: real-time sensor data */
#define BLE_CHAR_RESULT     0x0003  /* Notify: final result */
#define BLE_CHAR_STATUS     0x0004  /* Notify: device status */
#define BLE_CHAR_LOG        0x0005  /* Read: log download */
#define BLE_CHAR_OTA        0x0006  /* Write: OTA control */

/* Command opcodes */
#define BLE_CMD_START_SAMPLE    0x01
#define BLE_CMD_CALIBRATE       0x02
#define BLE_CMD_SET_CONFIG      0x03
#define BLE_CMD_GET_STATUS      0x04
#define BLE_CMD_DOWNLOAD_LOG    0x05
#define BLE_CMD_OTA_BEGIN       0x06
#define BLE_CMD_OTA_DATA        0x07
#define BLE_CMD_OTA_END         0x08
#define BLE_CMD_SET_TIME        0x09
#define BLE_CMD_FACTORY_RESET   0x0A

/* ========================================================================
 * Data Structures
 * ======================================================================== */

/* Raw sensor reading (one time step) */
typedef struct {
    uint32_t timestamp_ms;
    uint16_t bme688_1_gas;      /* BME688 #1 gas resistance (ohms) */
    uint16_t bme688_2_gas;      /* BME688 #2 gas resistance (ohms) */
    uint16_t scd41_co2;         /* SCD41 CO2 (ppm) */
    uint16_t s8_ch4;            /* SenseAir S8 CH4 (ppm) */
    uint16_t dgs_ethanol;       /* Ethanol (ppb) */
    uint16_t dgs_nh3;           /* Ammonia (ppb) */
    uint16_t dgs_h2s;           /* H2S (ppb) */
    uint16_t h2_analog;         /* H2 sensor ADC reading */
    uint16_t temperature;       /* °C × 100 */
    uint16_t humidity;          /* %RH × 100 */
    uint16_t pressure;          /* hPa */
} sensor_raw_t;

/* Extracted features per sensor channel */
typedef struct {
    float peak;         /* Baseline-corrected peak amplitude */
    float rise_time;    /* Time from 10% to 90% of peak (seconds) */
    float plateau;      /* Steady-state value at end of sample */
    float decay_tau;    /* Exponential decay time constant (seconds) */
    float auc;           /* Area under curve (trapezoidal) */
} sensor_features_t;

/* Full feature vector */
typedef struct {
    sensor_features_t sensors[NUM_SENSORS];
} feature_vector_t;

/* Breath analysis result (64 bytes — matches BLE protocol) */
typedef struct {
    uint32_t timestamp;           /* Unix time */
    uint8_t  metabolic_state;     /* 0-5 */
    uint8_t  breath_quality;      /* 0=valid, 1=dead-space, 2=contaminated */
    uint16_t acetone_ppm;         /* ppm × 100 */
    uint16_t h2_ppm;              /* ppm × 10 */
    uint16_t ch4_ppm;             /* ppm × 10 */
    uint16_t ethanol_ppm;         /* ppm × 100 */
    uint16_t isoprene_ppb;        /* ppb */
    uint16_t nh3_ppm;             /* ppm × 10 */
    uint16_t h2s_ppb;             /* ppb */
    uint16_t voc_index;           /* BME688 IAQ index 0-500 */
    uint16_t co2_ppm;             /* CO2 reference (ppm) */
    int16_t  temperature;         /* °C × 10 */
    uint16_t humidity;            /* %RH × 10 */
    uint16_t pressure;            /* hPa */
    uint8_t  battery_pct;         /* 0-100 */
    uint8_t  sensor_health;       /* Bitmask: bit n = sensor n OK */
    uint8_t  reserved[8];         /* Future expansion */
} __attribute__((packed)) breath_result_t;

/* Calibration coefficients */
typedef struct {
    float zero_offset[NUM_SENSORS];    /* Baseline readings */
    float span_gain[NUM_SENSORS];      /* Span calibration gains */
    float cross_sens[8][8];            /* Cross-sensitivity matrix */
    float temp_coeff[NUM_SENSORS];     /* Temperature compensation */
    float hum_coeff[NUM_SENSORS];      /* Humidity compensation */
    uint32_t calibration_time;         /* Unix time of last cal */
    uint16_t ambient_count;            /* Number of ambient recalibrations */
    uint8_t  valid;                    /* 1 if calibration valid */
    uint8_t  reserved[3];
    uint32_t crc32;                    /* Integrity check */
} calib_data_t;

/* Device state machine */
typedef enum {
    STATE_IDLE = 0,
    STATE_WARMUP,
    STATE_SAMPLE,
    STATE_EXHALE_WAIT,
    STATE_ANALYZE,
    STATE_RESULT,
    STATE_CALIBRATION,
    STATE_SLEEP,
    STATE_ERROR
} device_state_t;

/* Global device context */
typedef struct {
    device_state_t state;
    device_state_t prev_state;
    uint32_t state_timer_ms;       /* Time in current state */
    uint32_t sample_index;         /* Current sample within window */
    sensor_raw_t samples[SAMPLE_COUNT];
    uint32_t last_ambient_ms;      /* Last ambient baseline time */
    calib_data_t calib;
    breath_result_t last_result;
    uint8_t battery_pct;
    uint8_t charging;
    uint8_t ble_connected;
    uint8_t sd_present;
    uint8_t error_code;
} device_ctx_t;

/* Error codes */
#define ERR_NONE            0
#define ERR_I2C_FAIL        1
#define ERR_SENSOR_TIMEOUT  2
#define ERR_SD_FAIL         3
#define ERR_BLE_FAIL        4
#define ERR_CALIB_INVALID   5
#define ERR_FLASH_FAIL      6
#define ERR_LOW_BATTERY     7
#define ERR_BREATH_INVALID  8

/* ========================================================================
 * Macro Helpers
 * ======================================================================== */

#define MIN(a, b)           ((a) < (b) ? (a) : (b))
#define MAX(a, b)           ((a) > (b) ? (a) : (b))
#define CLAMP(x, lo, hi)    (MIN(MAX((x), (lo)), (hi)))
#define ARRAY_SIZE(a)       (sizeof(a) / sizeof((a)[0]))

#define UNUSED(x)           ((void)(x))

/* Delay helpers */
#define DELAY_MS(ms)        delay_ms(ms)
#define DELAY_US(us)        delay_us(us)

/* ========================================================================
 * Function Prototypes (forward declarations for board-level init)
 * ======================================================================== */

void board_init(void);
void clock_init(void);
void gpio_init(void);
void nvic_init(void);

void delay_ms(uint32_t ms);
void delay_us(uint32_t us);

uint32_t millis(void);
uint32_t micros(void);

void enter_sleep(void);
void enter_stop(void);

/* GPIO helpers */
static inline void gpio_set(gpio_reg_t *port, uint8_t pin) {
    port->BSRR = GPIO_BSRR_SET(pin);
}
static inline void gpio_clear(gpio_reg_t *port, uint8_t pin) {
    port->BSRR = GPIO_BSRR_RESET(pin);
}
static inline uint8_t gpio_read(gpio_reg_t *port, uint8_t pin) {
    return (port->IDR >> pin) & 1;
}

static inline void gpio_config(gpio_reg_t *port, uint8_t pin,
                               uint8_t mode, uint8_t otype,
                               uint8_t speed, uint8_t pupd) {
    uint32_t pos = pin * 2;
    port->MODER = (port->MODER & ~(0x3UL << pos)) | ((uint32_t)mode << pos);
    port->OTYPER = (port->OTYPER & ~(0x1UL << pin)) | ((uint32_t)otype << pin);
    port->OSPEEDR = (port->OSPEEDR & ~(0x3UL << pos)) | ((uint32_t)speed << pos);
    port->PUPDR = (port->PUPDR & ~(0x3UL << pos)) | ((uint32_t)pupd << pos);
}

static inline void gpio_config_af(gpio_reg_t *port, uint8_t pin,
                                  uint8_t af, uint8_t speed) {
    gpio_config(port, pin, GPIO_MODE_AF, GPIO_OTYPE_PP, speed, GPIO_PUPD_NONE);
    if (pin < 8) {
        port->AFRL = (port->AFRL & ~(0xFUL << (pin * 4))) | ((uint32_t)af << (pin * 4));
    } else {
        port->AFRH = (port->AFRH & ~(0xFUL << ((pin - 8) * 4))) |
                     ((uint32_t)af << ((pin - 8) * 4));
    }
}

#endif /* BOARD_H */