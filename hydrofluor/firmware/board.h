/*
 * board.h — HydroFluor pin & peripheral map
 * Author: jayis1
 * License: MIT
 *
 * Target: STM32L4R9VIT6 (LQFP100)
 * Board:  HydroFluor water-quality fluorescence sonde
 */
#ifndef HYDROFLUOR_BOARD_H
#define HYDROFLUOR_BOARD_H

#include <stdint.h>
#include <stddef.h>

/* ---- Clock tree (after SystemInit, HSI16 PLL → 120 MHz SYSCLK) ---- */
#define BOARD_SYSCLK_HZ        120000000UL
#define BOARD_HCLK_HZ           120000000UL
#define BOARD_PCLK1_HZ          40000000UL   /* APB1 (TIM2-7, I2C, USART2/3) */
#define BOARD_PCLK2_HZ          120000000UL  /* APB2 (TIM1, USART1, SPI1)    */

#define BOARD_APB1_PRESCALER    3            /* /3 → 40 MHz                  */
#define BOARD_APB2_PRESCALER    1            /* /1 → 120 MHz                 */

/* ---- LED excitation array (TIM1 advanced-control timer) ---- *
 * Six excitation LEDs driven through constant-current MOSFET stages.
 * TIM1 channels 1-6 (complementary outputs NOT used; single-ended).
 *
 *  CH1 -> PA8  -> LED_DRV_EN_255  (255 nm deep-UV,  AlGaN)
 *  CH2 -> PA9  -> LED_DRV_EN_280  (280 nm deep-UV,  AlGaN)
 *  CH3 -> PA10 -> LED_DRV_EN_365  (365 nm,          InGaN)
 *  CH4 -> PA11 -> LED_DRV_EN_470  (470 nm,          InGaN)
 *  CH5 -> PC6  -> LED_DRV_EN_590  (590 nm,          AlInGaP)
 *  CH6 -> PC7  -> LED_DRV_EN_660  (660 nm,          AlInGaP)
 */
#define LED_DRV_PORT            GPIOA
#define LED_DRV_255_PIN         8     /* TIM1_CH1 */
#define LED_DRV_280_PIN         9     /* TIM1_CH2 */
#define LED_DRV_365_PIN         10    /* TIM1_CH3 */
#define LED_DRV_470_PIN         11    /* TIM1_CH4 */
#define LED_DRV_590_PIN_PORT    GPIOC
#define LED_DRV_590_PIN         6     /* TIM1_CH1 alt (remap) */
#define LED_DRV_660_PIN         7     /* TIM1_CH2 alt (remap) */

/* Excitation wavelength enum (index into led_excitation table) */
typedef enum {
    EX_255NM = 0,   /* deep-UV, hydrocarbons + A254 absorbance            */
    EX_280NM = 1,   /* deep-UV, protein/aromatics                         */
    EX_365NM = 2,   /* near-UV, CDOM/DOM (humic substances)              */
    EX_470NM = 3,   /* blue, chlorophyll-a excitation                    */
    EX_590NM = 4,   /* amber, phycocyanin (cyanobacteria) excitation     */
    EX_660NM = 5,   /* deep-red, turbidity 90° side scatter              */
    EX_CHANNEL_COUNT
} ex_wavelength_t;

/* Per-LED pulse & current config (default; overridable from app) */
typedef struct {
    ex_wavelength_t id;
    uint16_t pulse_us;     /* on-duration                       */
    uint16_t current_ma;  /* constant-current setpoint          */
    uint8_t  averages;    /* pulse-repeats per sample          */
} led_config_t;

#define DEFAULT_PULSE_US        120
#define DEFAULT_CURRENT_MA      300
#define DEFAULT_AVERAGES        8

/* ---- Photodiode / ADC chain ---- *
 * 4 fluorescence PDs + 1 transmission/reference PD → 8:1 analog mux → ADS1256
 * ADS1256 on SPI1 (PA5/PA6/PA7 + PB2 CS).  Data-ready line on PB1 EXTI.
 * Mux select lines: PD2, PC4, PC5 (3-bit, 74HC4051 control).
 */
#define ADC_SPI                 SPI1
#define ADC_SPI_SCK_PIN         5     /* PA5  */
#define ADC_SPI_MISO_PIN        6     /* PA6  */
#define ADC_SPI_MOSI_PIN        7     /* PA7  */
#define ADC_CS_PIN              2     /* PB2  */
#define ADC_DRDY_PIN            1     /* PB1, EXTI falling */
#define ADC_DRDY_PORT           GPIOB
#define ADC_CS_PORT             GPIOB

/* Analog mux select lines (74HC4051) */
#define MUX_SEL0_PIN            2     /* PD2 */
#define MUX_SEL1_PIN            4     /* PC4 */
#define MUX_SEL2_PIN            5     /* PC5 */

typedef enum {
    PD_FLUOR_360 = 0,    /* oil emission @ 360 nm, excited 255 */
    PD_FLUOR_430 = 1,    /* CDOM emission @ 430 nm, excited 365 */
    PD_FLUOR_650 = 2,    /* phycocyanin @ 650 nm, excited 590   */
    PD_FLUOR_685 = 3,    /* chlorophyll-a @ 685 nm, excited 470 */
    PD_REF_255   = 4,    /* transmission ref for 255 nm          */
    PD_REF_660   = 5,    /* transmission ref for 660 nm          */
    PD_SCATTER_660 = 6,  /* 90° side scatter for turbidity        */
    PD_GND_BIAS  = 7,    /* grounded input for dark/bias meas.   */
    PD_CHANNEL_COUNT
} pd_channel_t;

/* ---- Peristaltic pump (TIM2 PWM + enable) ---- */
#define PUMP_PWM_TIMER          TIM2
#define PUMP_PWM_PIN            3     /* PB3, TIM2_CH2 */
#define PUMP_PWM_PORT           GPIOB
#define PUMP_EN_PIN             4     /* PB4 */
#define PUMP_EN_PORT            GPIOB
#define PUMP_PWM_HZ             20000
#define PUMP_DEFAULT_DUTY_PCT   60

/* ---- BLE 5 (BM71) on USART1 ---- */
#define BLE_UART                USART1
#define BLE_UART_TX_PIN         9     /* PA9 alt — shared with LED; muxed */
#define BLE_UART_RX_PIN         10    /* PA10 alt */
#define BLE_UART_BAUD           115200
#define BLE_RST_PIN             0     /* PA0 */
#define BLE_RST_PORT            GPIOA

/* ---- LoRa (RFM95W) on SPI2 ---- */
#define LORA_SPI                SPI2
#define LORA_SCK_PIN             13    /* PB13 */
#define LORA_MISO_PIN            14    /* PB14 */
#define LORA_MOSI_PIN            15    /* PB15 */
#define LORA_CS_PIN              12    /* PB12 */
#define LORA_CS_PORT             GPIOB
#define LORA_RST_PIN             8     /* PA8 alt */
#define LORA_RST_PORT            GPIOA
#define LORA_DIO0_PIN            0     /* PB0, EXTI rising */
#define LORA_DIO0_PORT           GPIOB
#define LORA_DIO1_PIN            5     /* PB5 */
#define LORA_DIO1_PORT           GPIOB
#define LORA_FREQ_HZ             915000000UL   /* US band; config in lora_link */

/* ---- microSD on SDIO (PC8-12, PD2 CMD) ---- */
#define SD_SDMMC                SDMMC1
#define SD_CLK_PIN              12    /* PC12 */
#define SD_CMD_PIN              2     /* PD2  */
#define SD_D0_PIN                8     /* PC8  */
#define SD_D1_PIN                9     /* PC9  */
#define SD_D2_PIN                10    /* PC10 */
#define SD_D3_PIN                11    /* PC11 */
#define SD_CD_PIN                15    /* PC15 (card detect) */

/* ---- QSPI flash (W25Q128) for calibration + PLS model ---- */
#define QSPI_CS_PIN             10    /* PB10 */
#define QSPI_CLK_PIN             2     /* PB2 alt */
#define QSPI_D0_PIN             11    /* PB11 */
#define QSPI_D1_PIN             12    /* PB12 */
#define QSPI_D2_PIN             13    /* PB13 */
#define QSPI_D3_PIN             14    /* PB14 */
#define QSPI_FLASH_SIZE_BYTES   (128UL * 1024UL * 1024UL / 8UL)  /* 128 Mbit */

/* ---- I²C peripherals (I2C1, PB6 SCL / PB7 SDA) ---- */
#define I2C1_SCL_PIN            6     /* PB6 */
#define I2C1_SDA_PIN            7     /* PB7 */
#define I2C1_HZ                 100000

/* MS5837-02BA depth/pressure sensor (I2C addr 0x76) */
#define DEPTH_I2C_ADDR          0x76
#define DEPTH_MAX_BAR           30.0f

/* PCF85263A RTC (I2C addr 0x51) */
#define RTC_I2C_ADDR            0x51

/* ---- 1-Wire (DS18B20 bench temp) ---- */
#define ONEWIRE_PIN             8     /* PB8 */
#define ONEWIRE_PORT            GPIOB
#define DS18B20_CMD_CONVERT     0x44
#define DS18B20_CMD_READ_SP     0xBE

/* ---- USB-C CDC (USB FS, PA11/PA12) ---- */
#define USB_FS_DM_PIN           11    /* PA11 */
#define USB_FS_DP_PIN           12    /* PA12 */

/* ---- Battery / power ---- *
 * Li-SOCl2 cell monitored via internal ADC channel (VBAT/3 → ADC_IN13).
 * Supercap voltage on ADC_IN14.  1.2 V internal ref for ratiometric.
 */
#define BAT_ADC_CHANNEL         13
#define SCAP_ADC_CHANNEL        14
#define BAT_FULL_MV             3600
#define BAT_EMPTY_MV            2400
#define BAT_SCALE_NUM           3000   /* mV = raw * 3000 / 4095 * 3 (divider) */

/* ---- Status LED / user feedback (optional) ---- */
#define STATUS_LED_PIN          7     /* PA7 — green */
#define STATUS_LED_PORT         GPIOA

/* ---- Survey / operational constants ---- */
#define SURVEY_DEFAULT_PERIOD_MS    10000UL   /* 10 s between samples */
#define SURVEY_MIN_PERIOD_MS         1000UL
#define SURVEY_MAX_PERIOD_MS       3600000UL
#define LOG_FILE_MAX_KB            10240      /* 10 MB rotating log cap     */

/* Calibration model limits (PLS) */
#define PLS_FEATURES             24          /* 6 excitations × 4 detectors */
#define PLS_LATENT_VARS          6
#define PLS_MODEL_BYTES_PER_ANALYTE  (PLS_LATENT_VARS * sizeof(int16_t) * 2 + 8)

#define ANALYTES_COUNT           5           /* CDOM, Chl-a, Phyc, Oil, Turb */

#endif /* HYDROFLUOR_BOARD_H */