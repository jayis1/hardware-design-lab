/**
 * @file    board.h
 * @brief   CarbonFlux hardware pin mappings, clock configuration, and PCB revision.
 * @author  jayis1
 * @copyright Copyright © 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 *
 * This file defines all board-specific constants for the CarbonFlux soil
 * CO₂ flux monitor. Every pin assignment, clock rate, and physical constant
 * for the v1.0 PCB is centralized here.
 *
 * Target MCU: STM32U5A9ZG (UFBGA176+25)
 * PCB Rev: A (2026-06-17)
 */

#ifndef BOARD_H
#define BOARD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* ======================================================================== */
/*  PCB REVISION                                                            */
/* ======================================================================== */
#define PCB_REV_MAJOR           1
#define PCB_REV_MINOR           0
#define PCB_REV_STRING          "CarbonFlux v1.0"
#define BOARD_AUTHOR            "jayis1"
#define BOARD_DATE              "2026-06-17"

/* ======================================================================== */
/*  SYSTEM CLOCK CONFIGURATION                                              */
/* ======================================================================== */

/* External oscillators */
#define HSE_FREQ_HZ             16000000UL      /* 16 MHz main crystal     */
#define LSE_FREQ_HZ             32768UL         /* 32.768 kHz RTC crystal  */
#define HSII6_FREQ_HZ           16000000UL      /* Internal 16 MHz RC      */

/* Derived clock targets */
#define SYSCLK_FREQ_HZ          160000000UL     /* 160 MHz system clock    */
#define HCLK_FREQ_HZ            160000000UL     /* AHB bus clock           */
#define APB1_FREQ_HZ            160000000UL     /* APB1 (max 160 MHz)      */
#define APB2_FREQ_HZ            160000000UL     /* APB2 (max 160 MHz)      */
#define USB_FREQ_HZ             48000000UL      /* USB full-speed 48 MHz   */

/* PLL configuration (HSI16 → PLL → 160 MHz) */
#define PLL_M                    1              /* Divide by M = 1        */
#define PLL_N                   20              /* Multiply by N = 20      */
#define PLL_P                    2              /* Divide by P = 2         */
#define PLL_Q                    4              /* USB divider (160/4=48)  */

/* ======================================================================== */
/*  PERIPHERAL CLOCK ENABLE MASKS                                           */
/* ======================================================================== */

/* RCC AHB1ENR bits */
#define RCC_AHB1ENR_DMA1EN      (1U << 0)
#define RCC_AHB1ENR_FLASHEN     (1U << 3)
#define RCC_AHB1ENR_CRCEN       (1U << 6)

/* RCC APB1ENR1 bits */
#define RCC_APB1ENR1_TIM2EN     (1U << 0)
#define RCC_APB1ENR1_USART2EN   (1U << 17)
#define RCC_APB1ENR1_I2C1EN     (1U << 21)
#define RCC_APB1ENR1_SPI2EN     (1U << 28)

/* RCC APB2ENR bits */
#define RCC_APB2ENR_SPI1EN      (1U << 12)
#define RCC_APB2ENR_ADC1EN      (1U << 13)
#define RCC_APB2ENR_SYSCFGEN    (1U << 0)

/* ======================================================================== */
/*  GPIO PIN MAPPINGS — STM32U5A9ZG (UFBGA176+25)                          */
/* ======================================================================== */

/* --- SPI1: SX1262 LoRa modem --- */
#define PIN_LORA_NSS            GPIO_PB0       /* PB0 — Chip select       */
#define PIN_LORA_SCK            GPIO_PB3       /* PB3 — SPI clock 8 MHz   */
#define PIN_LORA_MISO           GPIO_PB4       /* PB4 — MISO              */
#define PIN_LORA_MOSI           GPIO_PB5       /* PB5 — MOSI              */
#define PIN_LORA_DIO1           GPIO_PB1       /* PB1 — DIO1 IRQ          */
#define PIN_LORA_RESET          GPIO_PB2       /* PB2 — Reset (active low)*/
#define PIN_LORA_BUSY           GPIO_PA1       /* PA1 — Busy indicator    */

/* --- I2C1: Sensor bus (SCD41, DPS310, TMP117, RV-8803) --- */
#define PIN_I2C1_SCL            GPIO_PB6       /* PB6 — SCL 400 kHz       */
#define PIN_I2C1_SDA            GPIO_PB7       /* PB7 — SDA               */

/* --- Sensor enable pins (individual power control) --- */
#define PIN_EN_SCD41            GPIO_PB8       /* PB8 — SCD41 power EN   */
#define PIN_EN_DPS310           GPIO_PB9       /* PB9 — DPS310 power EN  */
#define PIN_EN_TMP117           GPIO_PA0       /* PA0 — TMP117 power EN  */

/* --- SPI2 / QSPI: W25Q128 flash --- */
#define PIN_FLASH_CS            GPIO_PA8       /* PA8 — Chip select       */
#define PIN_FLASH_SCK           GPIO_PA5       /* PA5 — QSPI clock 40 MHz*/
#define PIN_FLASH_SI            GPIO_PA7       /* PA7 — MOSI / IO0        */
#define PIN_FLASH_SO            GPIO_PA6       /* PA6 — MISO / IO1        */
#define PIN_FLASH_IO2           GPIO_PC10      /* PC10 — Quad IO2         */
#define PIN_FLASH_IO3           GPIO_PC11      /* PC11 — Quad IO3         */

/* --- USB CDC (Virtual COM Port) --- */
#define PIN_USB_DP              GPIO_PA11      /* PA11 — USB D+            */
#define PIN_USB_DM              GPIO_PA12      /* PA12 — USB D-            */
#define PIN_USART2_TX           GPIO_PA2       /* PA2 — CDC TX (VCP)       */
#define PIN_USART2_RX           GPIO_PA3       /* PA3 — CDC RX (VCP)       */

/* --- ADC inputs --- */
#define PIN_ADC_PAR             GPIO_PC0       /* PC0 — ADC1_IN0 — PAR     */
#define PIN_ADC_VBAT            GPIO_PC1       /* PC1 — ADC1_IN1 — Battery */
#define PIN_ADC_SPARE           GPIO_PC2       /* PC2 — ADC1_IN2 — Spare   */

/* --- Servo (chamber lid actuator) --- */
#define PIN_SERVO_PWM           GPIO_PC4       /* PC4 — TIM2_CH1 — 50 Hz */
#define PIN_SERVO_STALL         GPIO_PC5       /* PC5 — Stall detect GPIO */

/* --- 1-Wire (DS18B20 soil temperature sensors) --- */
#define PIN_ONEWIRE_DATA        GPIO_PC6       /* PC6 — 1-Wire data bus   */

/* --- RTC interrupt --- */
#define PIN_RTC_INT             GPIO_PA4       /* PA4 — RV-8803 NINT      */

/* --- Status LEDs --- */
#define PIN_LED_GREEN           GPIO_PA10      /* PA10 — Power/status     */
#define PIN_LED_RED             GPIO_PA9       /* PA9 — Error indicator   */
#define PIN_LED_BLUE            GPIO_PC8       /* PC8 — LoRa TX indicator */

/* --- Configuration DIP switch --- */
#define PIN_DIP_SW0             GPIO_PA15      /* PA15 — DIP bit 0        */

/* --- Sensor stake connector (8-pin) --- */
#define PIN_STAKE_PWR           GPIO_PC9       /* PC9 — 12V power to stake*/
#define PIN_STAKE_SDI12         GPIO_PC7       /* PC7 — SDI-12 data line  */

/* --- SWD debug --- */
#define PIN_SWD_SWDIO           GPIO_PA13      /* PA13 — SWD data         */
#define PIN_SWD_SWCLK           GPIO_PA14      /* PA14 — SWD clock        */

/* ======================================================================== */
/*  I²C DEVICE ADDRESSES (7-bit)                                            */
/* ======================================================================== */
#define SCD41_I2C_ADDR          0x62           /* SCD41 default address   */
#define DPS310_I2C_ADDR         0x76           /* DPS310 default (SDO=1)  */
#define TMP117_I2C_ADDR         0x48           /* TMP117 default (A0=GND) */
#define RV8803_I2C_ADDR         0x32           /* RV-8803 default         */

/* ======================================================================== */
/*  SENSOR CHARACTERISTICS                                                  */
/* ======================================================================== */

/* SCD41 CO₂ sensor */
#define SCD41_ACCURACY_PPM      40             /* ±40 ppm typical         */
#define SCD41_RANGE_MIN_PPM     0              /* 0 ppm minimum           */
#define SCD41_RANGE_MAX_PPM     40000UL        /* 40,000 ppm maximum      */
#define SCD41_MEAS_PERIOD_MS    2000           /* 2-second measurement    */
#define SCD41_WARMUP_MS         1000           /* 1 second warmup         */
#define SCD41_I2C_TIMEOUT_MS    100            /* I²C transaction timeout */

/* DPS310 barometer */
#define DPS310_ACCURACY_HPA     0.002f         /* ±0.002 hPa precision    */
#define DPS310_RANGE_MIN_HPA    300.0f         /* 300 hPa min             */
#define DPS310_RANGE_MAX_HPA    1200.0f        /* 1200 hPa max            */

/* TMP117 air temperature */
#define TMP117_ACCURACY_C       0.1f           /* ±0.1°C                  */
#define TMP117_RANGE_MIN_C      -40.0f         /* -40°C min               */
#define TMP117_RANGE_MAX_C      85.0f          /* +85°C max               */

/* DS18B20 soil temperature */
#define DS18B20_ACCURACY_C      0.5f           /* ±0.5°C                  */
#define DS18B20_RESOLUTION_BITS 12             /* 12-bit resolution       */
#define DS18B20_CONV_MS_MAX     750            /* 750 ms at 12-bit       */
#define DS18B20_NUM_PROBES      3              /* Three sensors on stake  */

/* ======================================================================== */
/*  CHAMBER PHYSICAL CONSTANTS                                              */
/* ======================================================================== */

/* Standard 20 cm diameter collar, 14.3 cm headspace height (volume = 4.5 L) */
#define CHAMBER_DIAMETER_CM     20.0f          /* Collar inner diameter   */
#define CHAMBER_RADIUS_M        0.10f          /* 10 cm radius in meters  */
#define CHAMBER_AREA_M2         0.0314159f     /* π × r²                  */
#define CHAMBER_HEIGHT_M        0.143f         /* Headspace height (14.3 cm)*/
#define CHAMBER_VOLUME_M3       0.0045f        /* 4.5 L in m³             */
#define CHAMBER_VOLUME_L        4.5f           /* 4.5 liters              */

/* ======================================================================== */
/*  ACCUMULATION / MEASUREMENT PARAMETERS                                   */
/* ======================================================================== */

#define ACCUM_MIN_DURATION_S    120            /* Minimum accumulation    */
#define ACCUM_MAX_DURATION_S    300            /* Maximum accumulation    */
#define ACCUM_DEFAULT_S         180            /* Default: 3 minutes      */
#define EQUILIBRATION_S         60             /* Lid-open equilibration  */
#define CO2_SAMPLE_INTERVAL_S   2              /* Read SCD41 every 2 sec  */
#define MAX_CO2_SAMPLES         150            /* Max samples per cycle   */
#define MIN_VALID_SAMPLES       30             /* Minimum for regression  */
#define R2_MIN_THRESHOLD        0.95f          /* Minimum R² for valid    */

/* ======================================================================== */
/*  MEASUREMENT SCHEDULING                                                  */
/* ======================================================================== */

#define DEFAULT_INTERVAL_S      1800           /* 30 minutes default      */
#define MIN_INTERVAL_S          900            /* 15 minutes minimum      */
#define MAX_INTERVAL_S          21600          /* 6 hours maximum         */
#define BURST_DURATION_S        1800           /* 30 min burst mode       */
#define BURST_INTERVAL_S        1              /* 1 Hz in burst mode      */

/* ======================================================================== */
/*  POWER MANAGEMENT                                                        */
/* ======================================================================== */

#define VBAT_DIVIDER_R1         100000.0f      /* 100 kΩ top resistor     */
#define VBAT_DIVIDER_R2         10000.0f       /* 10 kΩ bottom resistor   */
#define VBAT_DIVIDER_RATIO      ((VBAT_DIVIDER_R1 + VBAT_DIVIDER_R2) / VBAT_DIVIDER_R2)
#define VBAT_FULL_LIFEPO4       14.6f          /* Full charge (4 cells)   */
#define VBAT_EMPTY_LIFEPO4      10.0f          /* Empty threshold          */
#define VBAT_LOW_WARN           11.5f          /* Low battery warning      */
#define VBAT_CRITICAL           10.5f          /* Critical — force sleep   */

/* TPS63070 buck-boost efficiency curve (typical at 3.3V out) */
#define TPS63070_EFF_3V3        0.85f          /* 85% typical efficiency   */
#define TPS63070_VIN_MIN        2.5f           /* Minimum input voltage    */
#define TPS63070_VIN_MAX        14.0f          /* Maximum input voltage    */

/* Solar MPPT (CN3791) */
#define MPPT_CHARGE_VOLTAGE     14.6f          /* LiFePO₄ 4S absorption   */
#define MPPT_FLOAT_VOLTAGE      13.8f          /* Float voltage            */
#define MPPT_MAX_CHARGE_A       1.0f           /* 1 A max charge current   */

/* Power states */
#define POWER_ACTIVE_MA         15.0f          /* ~15 mA active, all sens */
#define POWER_TX_MA             45.0f          /* ~45 mA during LoRa TX   */
#define POWER_SLEEP_UA          2.5f           /* ~2.5 µA Stop 2 mode     */

/* ======================================================================== */
/*  FLASH STORAGE (W25Q128JV, 16 MB)                                        */
/* ======================================================================== */

#define FLASH_TOTAL_SIZE        16777216UL     /* 16 MB = 128 Mb          */
#define FLASH_SECTOR_SIZE       4096UL         /* 4 kB erase sector       */
#define FLASH_PAGE_SIZE         256UL          /* 256 B program page      */
#define FLASH_LOG_START_ADDR    0x00100000UL   /* Start at 1 MB into flash*/
#define FLASH_LOG_SIZE          15728640UL     /* ~15 MB for ring buffer  */
#define FLASH_LOG_MAX_ENTRIES   (FLASH_LOG_SIZE / sizeof(flux_record_t))

/* Wear leveling constants */
#define FLASH_WEAR_SECTORS      (FLASH_LOG_SIZE / FLASH_SECTOR_SIZE)  /* ~3840 */
#define FLASH_MAX_ERASE_CYCLES  100000UL       /* W25Q128 rated endurance */

/* ======================================================================== */
/*  LORAWAN CONFIGURATION                                                   */
/* ======================================================================== */

#define LORA_FREQ_EU868_DEFAULT 868100000UL    /* 868.1 MHz EU default    */
#define LORA_FREQ_US915_DEFAULT 903900000UL    /* 903.9 MHz US default    */
#define LORA_TX_POWER_DBM       22             /* +22 dBm max             */
#define LORA_SF_DEFAULT         7              /* SF7 (fastest) default    */
#define LORA_BW_DEFAULT         125000         /* 125 kHz bandwidth        */
#define LORA_CR_DEFAULT         5              /* Coding rate 4/5         */
#define LORA_JOIN_RETRIES       3              /* Max OTAA join attempts   */
#define LORA_CONFIRMED_RETRIES  3              /* Confirmed uplink retries */
#define LORA_RX1_DELAY_MS       1000           /* RX1 window after TX     */
#define LORA_RX2_DELAY_MS       2000           /* RX2 window after TX     */
#define LORA_JOIN_TIMEOUT_MS    15000          /* 15 s join timeout       */

/* ======================================================================== */
/*  SERVO / ACTUATOR PARAMETERS                                             */
/* ======================================================================== */

#define SERVO_PWM_FREQ_HZ       50             /* 50 Hz standard servo    */
#define SERVO_PWM_PERIOD_US     20000          /* 20 ms period            */
#define SERVO_PULSE_OPEN_US     2000           /* 2 ms → 180° (open)     */
#define SERVO_PULSE_CLOSED_US   1000           /* 1 ms → 0° (closed)     */
#define SERVO_MOVE_TIME_MS      500            /* 500 ms for full travel  */
#define SERVO_STALL_TIMEOUT_MS  2000           /* 2 s stall = error       */
#define SERVO_HOLD_MA           5              /* ~5 mA holding current   */

/* ======================================================================== */
/*  BUFFER SIZES                                                            */
/* ======================================================================== */

#define USB_CDC_TX_BUF_SIZE     256            /* USB CDC TX buffer       */
#define USB_CDC_RX_BUF_SIZE     128            /* USB CDC RX buffer       */
#define LORA_TX_BUF_SIZE        64             /* Max LoRaWAN payload     */
#define LORA_RX_BUF_SIZE        64             /* Max downlink payload    */
#define CLI_CMD_BUF_SIZE        64             /* Command line buffer     */
#define CLI_MAX_ARGS            8              /* Max command arguments   */

/* ======================================================================== */
/*  TIMING CONSTANTS                                                        */
/* ======================================================================== */

#define SYSTICK_FREQ_HZ         1000           /* 1 kHz SysTick           */
#define WATCHDOG_TIMEOUT_MS     25600          /* 25.6 s IWDG timeout     */
#define LED_BLINK_MS            250            /* 250 ms LED blink period */
#define SENSOR_RETRY_DELAY_MS   10             /* 10 ms sensor retry delay*/
#define I2C_TIMEOUT_MS          50             /* 50 ms I²C timeout       */

/* ======================================================================== */
/*  ERROR CODES                                                             */
/* ======================================================================== */

#define ERR_NONE                0
#define ERR_I2C_TIMEOUT         -1
#define ERR_I2C_NACK            -2
#define ERR_SENSOR_NOT_FOUND    -3
#define ERR_SENSOR_CRC          -4
#define ERR_LORA_JOIN_FAILED    -5
#define ERR_LORA_TX_FAILED      -6
#define ERR_FLASH_WRITE_FAIL    -7
#define ERR_FLASH_FULL          -8
#define ERR_SERVO_STALL         -9
#define ERR_ADC_SATURATION      -10
#define ERR_LOW_BATTERY         -11
#define ERR_CONFIG_CRC          -12
#define ERR_FLUX_R2_LOW         -13
#define ERR_FLUX_NEGATIVE       -14
#define ERR_TIMEOUT             -15

/* ======================================================================== */
/*  STATE MACHINE ENUM                                                      */
/* ======================================================================== */

typedef enum {
    STATE_POWER_UP       = 0,
    STATE_INIT           = 1,
    STATE_EQUILIBRATE    = 2,
    STATE_CLOSE_LID      = 3,
    STATE_ACCUMULATE     = 4,
    STATE_COMPUTE_FLUX   = 5,
    STATE_LOG_DATA       = 6,
    STATE_TX_LORA        = 7,
    STATE_OPEN_LID       = 8,
    STATE_SLEEP          = 9,
    STATE_CAL_ZERO       = 10,
    STATE_CAL_SPAN       = 11,
    STATE_BURST_MODE     = 12,
    STATE_ERROR          = 15
} flux_state_t;

/* ======================================================================== */
/*  INLINE HELPER FUNCTIONS                                                 */
/* ======================================================================== */

/** @brief Convert millivolts to battery percentage (LiFePO₄ 4S). */
static inline uint8_t batt_mv_to_pct(float mv) {
    if (mv >= 14600.0f) return 100;
    if (mv <= 10000.0f) return 0;
    return (uint8_t)((mv - 10000.0f) / 46.0f);  /* ~46 mV per percent */
}

/** @brief Map servo microseconds to TIM2 CCR value (assuming 1 MHz timer clock). */
static inline uint32_t servo_us_to_ccr(uint16_t us) {
    return (uint32_t)us;  /* TIM2 prescaled to 1 MHz → 1 count = 1 µs */
}

/** @brief CRC-32 of config block (hardware CRC unit used). */
static inline uint32_t calc_crc32(const uint8_t *data, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif /* BOARD_H */