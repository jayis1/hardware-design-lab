/**
 * @file    board.h
 * @brief   Terramesh geotechnical node — board-level pin definitions and constants
 * @author  jayis1
 * @copyright Copyright © 2026 jayis1
 * @license GPL-2.0
 *
 * This file defines all pin mappings, peripheral base addresses, and
 * board-specific constants for the Terramesh subterranean sensor node.
 * Target MCU: STM32U5A5ZJT6Q (Cortex-M33, LQFP144)
 */

#ifndef BOARD_H
#define BOARD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* ======================================================================== *
 *  MCU Identification                                                      *
 * ======================================================================== */
#define MCU_STM32U5A5ZJT6Q
#define MCU_FLASH_SIZE_KB      4096
#define MCU_SRAM_SIZE_KB       2560
#define MCU_CORE_FREQ_HZ       160000000UL
#define MCU_HSI16_FREQ_HZ      16000000UL
#define MCU_LSE_FREQ_HZ        32768UL

/* ======================================================================== *
 *  Application Flash Layout                                                 *
 * ======================================================================== */
#define FLASH_BASE              0x08000000UL
#define BOOTLOADER_SIZE_KB      32
#define BOOTLOADER_START        FLASH_BASE
#define BOOTLOADER_END          (FLASH_BASE + (BOOTLOADER_SIZE_KB * 1024))
#define APP_START               BOOTLOADER_END
#define APP_MAX_SIZE_KB         (FLASH_SIZE_KB - BOOTLOADER_SIZE_KB - LOG_RESERVED_KB)
#define LOG_RESERVED_KB         128
#define LOG_START               (FLASH_BASE + ((FLASH_SIZE_KB - LOG_RESERVED_KB) * 1024))
#define LOG_SECTOR_SIZE_KB      2
#define LOG_SECTOR_COUNT        (LOG_RESERVED_KB / LOG_SECTOR_SIZE_KB)

/* ======================================================================== *
 *  Pin Mappings — STM32U5A5ZJT6Q LQFP144                                    *
 * ======================================================================== */

/* --- SPI1: LoRa (SX1262) + Flash (MX25R6435F) --- */
#define PIN_SPI1_SCK           GPIO_PIN_0   /* PA0  */
#define PIN_SPI1_MISO          GPIO_PIN_1   /* PA1  */
#define PIN_SPI1_MOSI          GPIO_PIN_2   /* PA2  */
#define PIN_SX1262_NSS         GPIO_PIN_3   /* PA3  */
#define PIN_FLASH_CS           GPIO_PIN_4   /* PA4  */
#define PIN_SX1262_DIO1        GPIO_PIN_5   /* PA5  */
#define PIN_SX1262_BUSY        GPIO_PIN_6   /* PA6  */
#define PIN_SX1262_RESET       GPIO_PIN_7   /* PA7  */
#define GPIO_PORT_SPI1         GPIOA
#define GPIO_PORT_SX1262_CTRL  GPIOA
#define GPIO_PORT_FLASH_CTRL   GPIOA

/* --- SPI2: Inclinometer (SCL3300) + Accelerometer (ADXL372) --- */
#define PIN_SPI2_SCK           GPIO_PIN_0   /* PB0  */
#define PIN_SPI2_MISO          GPIO_PIN_1   /* PB1  */
#define PIN_SPI2_MOSI          GPIO_PIN_2   /* PB2  */
#define PIN_SCL3300_CS         GPIO_PIN_3   /* PB3  */
#define PIN_ADXL372_CS         GPIO_PIN_4   /* PB4  */
#define PIN_ADXL372_INT1       GPIO_PIN_5   /* PB5  */
#define GPIO_PORT_SPI2         GPIOB
#define GPIO_PORT_SPI2_CTRL    GPIOB

/* --- I2C1: Environmental (BME688), RTC (RV-3028), NFC (ST25DV) --- */
#define PIN_I2C1_SCL           GPIO_PIN_6   /* PB6  */
#define PIN_I2C1_SDA           GPIO_PIN_7   /* PB7  */
#define GPIO_PORT_I2C1         GPIOB

/* --- I2C2: Pore pressure ADCs (ADS1120 ×2) --- */
#define PIN_I2C2_SCL           GPIO_PIN_8   /* PB8  */
#define PIN_I2C2_SDA           GPIO_PIN_9   /* PB9  */
#define GPIO_PORT_I2C2         GPIOB

/* --- LPUART1: LTE-M (BG95-M3) --- */
#define PIN_LPUART1_TX         GPIO_PIN_0   /* PC0  */
#define PIN_LPUART1_RX         GPIO_PIN_1   /* PC1  */
#define PIN_BG95_PWRKEY        GPIO_PIN_2   /* PC2  */
#define PIN_BG95_STATUS        GPIO_PIN_3   /* PC3  */
#define GPIO_PORT_LPUART1      GPIOC
#define GPIO_PORT_BG95_CTRL    GPIOC

/* --- USART2: USB CDC-ACM debug --- */
#define PIN_USART2_TX          GPIO_PIN_4   /* PC4  */
#define PIN_USART2_RX          GPIO_PIN_5   /* PC5  */
#define GPIO_PORT_USART2       GPIOC

/* --- Interrupt inputs --- */
#define PIN_RTC_INT            GPIO_PIN_6   /* PC6  — RV-3028 alarm */
#define PIN_SCL3300_DRDY       GPIO_PIN_7   /* PC7  — inclinometer data ready */
#define PIN_ADS1120_1_DRDY     GPIO_PIN_8   /* PC8  — ADC #1 data ready */
#define PIN_ADS1120_2_DRDY     GPIO_PIN_9   /* PC9  — ADC #2 data ready */
#define GPIO_PORT_INT_IN       GPIOC

/* --- Analog / Misc --- */
#define PIN_MOISTURE_EN        GPIO_PIN_0   /* PD0  — moisture probe FET gate */
#define PIN_BATTERY_ADC        GPIO_PIN_1   /* PD1  — battery voltage sense */
#define PIN_MOISTURE_ADC       GPIO_PIN_2   /* PD2  — moisture probe analog */
#define PIN_LED_RED            GPIO_PIN_3   /* PD3  */
#define PIN_LED_GREEN          GPIO_PIN_4   /* PD4  */
#define PIN_PMIC_EN            GPIO_PIN_5   /* PD5  — enable 3.3 V rail */
#define PIN_LTE_EN             GPIO_PIN_6   /* PD6  — enable LTE rail */
#define GPIO_PORT_MISC         GPIOD

/* ======================================================================== *
 *  Peripheral Base Addresses                                                 *
 * ======================================================================== */
#define SPI1_BASE              0x40013000UL
#define SPI2_BASE              0x40003800UL
#define I2C1_BASE              0x40005400UL
#define I2C2_BASE              0x40005800UL
#define USART2_BASE            0x40004400UL
#define LPUART1_BASE           0x40008000UL
#define ADC1_BASE              0x50020000UL
#define TIM2_BASE              0x40000000UL
#define TIM6_BASE              0x40001000UL
#define RTC_BASE               0x46000000UL
#define IWDG_BASE              0x40002800UL
#define EXTI_BASE              0x46001000UL
#define DMA1_BASE              0x40020000UL
#define DMA2_BASE              0x40020400UL

/* ======================================================================== *
 *  I²C Device Addresses (7-bit)                                              *
 * ======================================================================== */
#define I2C_ADDR_BME688        0x76
#define I2C_ADDR_RV3028        0x52
#define I2C_ADDR_ST25DV        0x53
#define I2C_ADDR_ADS1120_1     0x48
#define I2C_ADDR_ADS1120_2     0x49
#define I2C_ADDR_MAX20361      0x28

/* ======================================================================== *
 *  SPI Device Configuration                                                  *
 * ======================================================================== */
#define SPI1_BAUDRATE_DIV      2           /* 80 MHz / 2 = 40 MHz (max 10 MHz for SX1262) */
#define SPI1_CPOL              0           /* CPOL=0, CPHA=0 (mode 0) */
#define SPI1_CPHA              0
#define SPI1_FIRST_BIT         SPI_FIRST_BIT_MSB

#define SPI2_BAUDRATE_DIV      10          /* 80 MHz / 10 = 8 MHz */
#define SPI2_CPOL              1           /* CPOL=1, CPHA=1 (mode 3 for SCL3300) */
#define SPI2_CPHA              1
#define SPI2_FIRST_BIT         SPI_FIRST_BIT_MSB

/* ======================================================================== *
 *  Timing Constants                                                           *
 * ======================================================================== */
#define MSEC_PER_SEC           1000UL
#define USEC_PER_MSEC          1000UL
#define TICK_FREQ_HZ           1000UL      /* SysTick at 1 kHz */
#define NORMAL_SAMPLE_INTERVAL_S   600     /* 10 minutes between samples */
#define FAST_SAMPLE_INTERVAL_S     10      /* 10 seconds during anomaly */
#define LORA_TX_INTERVAL_S         1800    /* 30 minutes between LoRa TX (normal) */
#define LORA_FAST_TX_INTERVAL_S    60      /* 1 minute during anomaly */
#define LTE_UPLINK_INTERVAL_S      21600   /* 6 hours between LTE uplinks */
#define HELLO_INTERVAL_S           1800    /* 30 min mesh hello */
#define ROUTE_TIMEOUT_S            5400    /* 90 min route expiry */
#define WATCHDOG_TIMEOUT_MS        10000   /* 10 s IWDG timeout */

/* ======================================================================== *
 *  Sensor Constants                                                           *
 * ======================================================================== */
#define PORE_PRESSURE_MAX_KPA      500.0f
#define PORE_PRESSURE_ADC_RANGE    65535U  /* 16-bit */
#define MOISTURE_ADC_RANGE        4095U   /* 12-bit */
#define TILT_RANGE_DEG             90.0f
#define ACCEL_RANGE_G              200.0f
#define BME688_TEMP_OFFSET_C       0.0f
#define BME688_PRESS_OFFSET_PA     0.0f

/* Classification thresholds */
#define THRESH_PORE_DEEP_KPA       15.0f
#define THRESH_PORE_SHALLOW_KPA    10.0f
#define THRESH_MOISTURE_PCT        10.0f
#define THRESH_TILT_DEG            0.1f
#define THRESH_TILT_RATE_DEG_PER_HR 0.5f
#define THRESH_TILT_RATE_CRITICAL   1.0f

/* ======================================================================== *
 *  LoRa Configuration                                                        *
 * ======================================================================== */
#define LORA_FREQ_HZ              868500000UL  /* 868.5 MHz (EU 868 band) */
#define LORA_SF                   10           /* Spreading factor */
#define LORA_BW_KHZ               125          /* Bandwidth */
#define LORA_CR                    5           /* Coding rate 4/5 */
#define LORA_TX_POWER_DBM         14           /* +14 dBm */
#define LORA_SYNC_WORD            0x2B         /* Private network */
#define LORA_PREAMBLE_LEN          8           /* Symbols */
#define LORA_RX_TIMEOUT_MS         5000        /* 5 s RX window */

/* ======================================================================== *
 *  Mesh Protocol Constants                                                    *
 * ======================================================================== */
#define MESH_MAX_HOPS             8
#define MESH_MAX_NODES            256
#define MESH_TDMA_SLOTS           8
#define MESH_TDMA_FRAME_S         60
#define MESH_BROADCAST_ADDR       0xFFFF
#define MESH_GATEWAY_ADDR         0x0000
#define MESH_MAX_PAYLOAD          51          /* Bytes after header */
#define MESH_RREQ_RETRIES         3
#define MESH_RREQ_TIMEOUT_S       30

/* Packet types */
#define MSG_TYPE_DATA             0x01
#define MSG_TYPE_ACK              0x02
#define MSG_TYPE_CMD              0x03
#define MSG_TYPE_BEACON           0x04
#define MSG_TYPE_REGISTER         0x05
#define MSG_TYPE_RREQ             0x06
#define MSG_TYPE_RREP             0x07

/* ======================================================================== *
 *  Error Codes                                                                *
 * ======================================================================== */
typedef enum {
    ERR_OK              = 0,
    ERR_TIMEOUT         = -1,
    ERR_BUSY            = -2,
    ERR_INVALID_PARAM   = -3,
    ERR_CRC_MISMATCH    = -4,
    ERR_SPI             = -5,
    ERR_I2C             = -6,
    ERR_LORA_TX         = -7,
    ERR_LORA_RX         = -8,
    ERR_LTE_AT          = -9,
    ERR_FLASH_WRITE     = -10,
    ERR_FLASH_READ      = -11,
    ERR_FLASH_ERASE     = -12,
    ERR_SENSOR_TIMEOUT  = -13,
    ERR_SENSOR_OVERRANGE = -14,
    ERR_NO_ROUTE        = -15,
    ERR_QUEUE_FULL      = -16,
} error_t;

/* ======================================================================== *
 *  Classification States                                                      *
 * ======================================================================== */
typedef enum {
    CLASS_NORMAL    = 0,
    CLASS_WARNING   = 1,
    CLASS_CRITICAL  = 2,
} classification_t;

/* ======================================================================== *
 *  Sensor Data Record (32 bytes, packed)                                      *
 * ======================================================================== */
typedef struct __attribute__((packed)) {
    uint32_t timestamp;             /* Unix epoch seconds */
    uint16_t pore_press_shallow;    /* 0.01 kPa units */
    uint16_t pore_press_deep;       /* 0.01 kPa units */
    uint16_t moisture;              /* 0.01% VWC */
    int16_t  tilt_x;                /* 0.001° units */
    int16_t  tilt_y;                /* 0.001° units */
    int16_t  accel_x;               /* mg units */
    int16_t  accel_y;               /* mg units */
    int16_t  accel_z;               /* mg units */
    int16_t  temperature;           /* 0.01°C units */
    uint16_t pressure;              /* Pa units */
    uint8_t  classification;        /* 0=NORMAL, 1=WARNING, 2=CRITICAL */
    uint8_t  battery_mv;            /* mV / 20 */
} sensor_record_t;

_Static_assert(sizeof(sensor_record_t) == 32, "sensor_record_t must be 32 bytes");

/* ======================================================================== *
 *  Mesh Packet Header (12 bytes, packed)                                      *
 * ======================================================================== */
typedef struct __attribute__((packed)) {
    uint8_t  preamble[8];           /* Sync word */
    uint16_t dest_addr;            /* Destination node ID */
    uint16_t src_addr;             /* Source node ID */
    uint16_t seq_num;              /* Monotonic sequence number */
    uint8_t  hop_count;            /* TTL */
    uint8_t  msg_type;             /* Message type */
    uint8_t  payload[0];           /* Flexible array member */
    /* CRC32 appended after payload */
} mesh_packet_header_t;

#define MESH_HEADER_SIZE           (sizeof(mesh_packet_header_t) + 4) /* +4 for CRC32 */
#define MESH_PACKET_MAX_SIZE       (MESH_HEADER_SIZE + MESH_MAX_PAYLOAD)

/* ======================================================================== *
 *  Routing Table Entry                                                        *
 * ======================================================================== */
typedef struct {
    uint16_t dest_addr;
    uint16_t next_hop;
    uint8_t  hop_count;
    uint32_t last_seen_s;           /* Last HELLO timestamp (seconds) */
    uint16_t seq_num;
    bool     valid;
} route_entry_t;

#define ROUTE_TABLE_SIZE          32

/* ======================================================================== *
 *  System State                                                               *
 * ======================================================================== */
typedef enum {
    STATE_SLEEP,
    STATE_SENSE,
    STATE_CLASSIFY,
    STATE_TX_QUEUE,
    STATE_TX_MESH,
    STATE_TX_LTE,
    STATE_RX_WINDOW,
    STATE_DFU,
    STATE_ERROR,
} system_state_t;

/* ======================================================================== *
 *  Inline Helpers                                                             *
 * ======================================================================== */

/** Convert milliseconds to SysTick ticks */
static inline uint32_t ms_to_ticks(uint32_t ms) {
    return ms;
}

/** Convert seconds to milliseconds */
static inline uint32_t s_to_ms(uint32_t s) {
    return s * MSEC_PER_SEC;
}

/** Check if a classification indicates an anomaly */
static inline bool is_anomaly(classification_t c) {
    return (c == CLASS_WARNING) || (c == CLASS_CRITICAL);
}

#ifdef __cplusplus
}
#endif

#endif /* BOARD_H */
