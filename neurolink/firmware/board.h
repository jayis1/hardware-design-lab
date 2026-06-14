/*
 * NeuroLink — Board Pin Definitions
 * STM32H743ZIT6 pin mapping for all peripherals
 */

#ifndef BOARD_H
#define BOARD_H

#include "registers.h"

/* ============================================================
 * GPIO Port Base Addresses
 * ============================================================ */
#define GPIOA       GPIOA_BASE
#define GPIOB       GPIOB_BASE
#define GPIOC       GPIOC_BASE
#define GPIOD       GPIOD_BASE
#define GPIOE       GPIOE_BASE
#define GPIOF       GPIOF_BASE
#define GPIOG       GPIOG_BASE
#define GPIOH       GPIOH_BASE

/* ============================================================
 * Pin Definitions (Port, Pin, Alternate Function)
 * ============================================================ */

/* --- SPI1: ADS1299 Daisy Chain --- */
#define ADS_CS_PIN      4       // PA4  - SPI1_NSS (manual CS)
#define ADS_SCLK_PIN    5       // PA5  - SPI1_SCK
#define ADS_MISO_PIN    6       // PA6  - SPI1_MISO
#define ADS_MOSI_PIN    7       // PA7  - SPI1_MOSI
#define ADS_START_PIN   8       // PA8  - GPIO output (also MCO1)
#define ADS_DRDY_PIN    7       // PD7  - EXTI input (data ready)
#define ADS_RESET_PIN   15      // PE15 - GPIO output (reset, active low)
#define ADS_MCLK_PIN    8       // PA8 alt - TIM1_CH1 PWM (2.048 MHz)

#define ADS_CS_PORT     GPIOA
#define ADS_SCLK_PORT   GPIOA
#define ADS_MISO_PORT   GPIOA
#define ADS_MOSI_PORT   GPIOA
#define ADS_START_PORT  GPIOA
#define ADS_DRDY_PORT   GPIOD
#define ADS_RESET_PORT  GPIOE

/* --- SPI2: iCE40 FPGA --- */
#define FPGA_CS_PIN     12      // PB12 - SPI2_NSS (manual CS)
#define FPGA_SCK_PIN    10      // PB10 - SPI2_SCK
#define FPGA_MISO_PIN   14      // PB14 - SPI2_MISO
#define FPGA_MOSI_PIN   15      // PB15 - SPI2_MOSI
#define FPGA_RESET_PIN  0       // PB0  - GPIO output (reset, active low)
#define FPGA_CDONE_PIN  1       // PB1  - GPIO input (config done)
#define FPGA_IRQ_PIN    8       // PE8  - EXTI input (FPGA interrupt)

#define FPGA_CS_PORT    GPIOB
#define FPGA_SCK_PORT   GPIOB
#define FPGA_MISO_PORT  GPIOB
#define FPGA_MOSI_PORT  GPIOB
#define FPGA_RESET_PORT GPIOB
#define FPGA_CDONE_PORT GPIOB
#define FPGA_IRQ_PORT   GPIOE

/* --- SPI3/QSPI: W25Q128 Boot Flash --- */
#define QSPI_CS_PIN     15      // PA15 - SPI3_NSS / QSPI_CS
#define QSPI_CLK_PIN    3       // PB3  - SPI3_SCK / QSPI_CLK
#define QSPI_D0_PIN     2       // PB2  - SPI3_MOSI / QSPI_D0
#define QSPI_D1_PIN     4       // PB4  - SPI3_MISO / QSPI_D1
#define QSPI_D2_PIN     5       // PB5  - GPIO / QSPI_D2
#define QSPI_D3_PIN     6       // PB6  - GPIO / QSPI_D3

#define QSPI_CS_PORT    GPIOA
#define QSPI_CLK_PORT   GPIOB
#define QSPI_D0_PORT    GPIOB
#define QSPI_D1_PORT    GPIOB

/* --- I2C1: LSM6DSOX IMU --- */
#define IMU_SDA_PIN     0       // PC0  - I2C1_SDA
#define IMU_SCL_PIN     1       // PC1  - I2C1_SCL

#define IMU_SDA_PORT    GPIOC
#define IMU_SCL_PORT    GPIOC

#define LSM6DSOX_ADDR   0x6A    // 7-bit I2C address (SA0=1)

/* --- I2C2: BQ25895 + TMP117 --- */
#define PMIC_SDA_PIN    4       // PC4  - I2C2_SDA
#define PMIC_SCL_PIN    5       // PC5  - I2C2_SCL

#define PMIC_SDA_PORT   GPIOC
#define PMIC_SCL_PORT   GPIOC

#define BQ25895_ADDR    0x6A    // 7-bit (PRO pin = LOW)
#define TMP117_ADDR     0x48    // 7-bit (A0=GND, A1=GND)

/* --- USART1: nRF52840 BLE --- */
#define BLE_TX_PIN      9       // PA9  - USART1_TX
#define BLE_RX_PIN      10      // PA10 - USART1_RX
#define BLE_CTS_PIN     7       // PA7 alt - USART1_CTS (if available)
#define BLE_RTS_PIN     8       // PA8 alt - USART1_RTS
#define BLE_RST_PIN     3       // PB3 alt - GPIO output (module reset)
#define BLE_IRQ_PIN     4       // PB4 alt - EXTI input (BLE event)

#define BLE_TX_PORT     GPIOA
#define BLE_RX_PORT     GPIOA
#define BLE_RST_PORT    GPIOB

/* --- USB OTG HS --- */
#define USB_DM_PIN      11      // PA11 - USB_OTG_HS_DM
#define USB_DP_PIN      12      // PA12 - USB_OTG_HS_DP
#define USB_VBUS_PIN    9       // PA9 alt - GPIO input (VBUS detect)
#define USB_VBUS_EN_PIN 4       // PH4  - GPIO output (VBUS power enable)

#define USB_DM_PORT     GPIOA
#define USB_DP_PORT     GPIOA
#define USB_VBUS_EN_PORT GPIOH

/* --- SDMMC1: MicroSD --- */
#define SD_D0_PIN       8       // PC8  - SDMMC1_D0
#define SD_D1_PIN       9       // PC9  - SDMMC1_D1
#define SD_D2_PIN       10      // PC10 - SDMMC1_D2
#define SD_D3_PIN       11      // PC11 - SDMMC1_D3
#define SD_CLK_PIN      12      // PC12 - SDMMC1_CK
#define SD_CMD_PIN      2       // PD2  - SDMMC1_CMD

/* --- LED Outputs --- */
#define LED0_R_PIN      5       // PE5  - TIM9_CH1 (PWM capable)
#define LED0_G_PIN      6       // PE6  - TIM9_CH2 (PWM capable)
#define LED0_B_PIN      7       // PE7  - TIM1_CH1N (PWM capable)

#define LED0_R_PORT     GPIOE
#define LED0_G_PORT     GPIOE
#define LED0_B_PORT     GPIOE

/* --- MUX Control (Impedance Measurement) --- */
#define MUX_EN_PIN      14      // PE14 - GPIO output (MUX enable)
#define MUX_A0_PIN      28      // ADS1299 GPIO1 (via SPI reg)
#define MUX_A1_PIN      29      // ADS1299 GPIO2 (via SPI reg)
#define MUX_A2_PIN      30      // ADS1299 GPIO3 (via SPI reg)

#define MUX_EN_PORT     GPIOE

/* --- Channel Power-Down --- */
#define CH_PD_PIN       15      // PE15 - shared with ADS_RESET
/* Using ADS_RESET_PIN for this function (shared) */

/* --- Analog Inputs --- */
#define VBAT_SENSE_PIN  0       // PA0  - ADC123_IN0 (battery voltage)
#define IMPEDANCE_OUT_PIN 1     // PA1  - ADC12_IN1 (impedance DAC out)

/* --- SWD Debug --- */
#define SWDIO_PIN       13      // PA13 - SWDIO
#define SWCLK_PIN       14      // PA14 - SWCLK

/* ============================================================
 * GPIO Mode Definitions
 * ============================================================ */
#define GPIO_MODE_INPUT     0x00
#define GPIO_MODE_OUTPUT    0x01
#define GPIO_MODE_AF        0x02
#define GPIO_MODE_ANALOG    0x03

#define GPIO_SPEED_LOW      0x00
#define GPIO_SPEED_MED      0x01
#define GPIO_SPEED_HIGH     0x02
#define GPIO_SPEED_VHIGH    0x03

#define GPIO_PULL_NONE      0x00
#define GPIO_PULL_UP        0x01
#define GPIO_PULL_DOWN      0x02

#define GPIO_OTYPE_PP       0x00
#define GPIO_OTYPE_OD       0x01

/* Alternate function mappings */
#define AF_SPI1             5
#define AF_SPI2             5
#define AF_SPI3             6
#define AF_I2C1             4
#define AF_I2C2             4
#define AF_USART1           7
#define AF_USB_OTG          10
#define AF_SDMMC1           12
#define AF_TIM1             1
#define AF_TIM9             3
#define AF_MCO1             0

/* ============================================================
 * Clock Configuration Constants
 * ============================================================ */
#define HSE_FREQ            27000000UL   // 27 MHz external crystal
#define LSE_FREQ            32768UL     // 32.768 kHz RTC crystal
#define SYSCLK_FREQ         480000000UL // 480 MHz system clock
#define AHB_FREQ            240000000UL // HPRE=/2
#define APB1_FREQ           120000000UL // D1PPRE=/2, D2PPRE=/2
#define APB2_FREQ           120000000UL // Same
#define SPI1_CLK_FREQ       200000000UL // PLL2_Q kernel clock
#define SPI2_CLK_FREQ       200000000UL // PLL2_Q kernel clock
#define USART1_CLK_FREQ     120000000UL // APB2 peripheral clock
#define ADS_MCLK_FREQ       2048000UL   // 2.048 MHz for ADS1299

/* ============================================================
 * Buffer Sizes
 * ============================================================ */
#define ADS_SAMPLE_SIZE     27          // 3 bytes status + 24 bytes (8ch × 3B)
#define ADS_CHAIN_FRAMES    8           // 8 devices in daisy chain
#define ADS_FRAME_SIZE      (ADS_SAMPLE_SIZE * ADS_CHAIN_FRAMES) // 216 bytes
#define USB_BULK_MAX        512         // USB HS bulk max packet
#define BLE_MTU             247         // BLE 5.0 max MTU
#define DMA_BUF_SIZE        (ADS_FRAME_SIZE * 4) // Double-buffer: 864 bytes
#define RECORDING_BUF_SIZE  (512 * 1024) // 512 KB recording buffer in DDR3L

#endif /* BOARD_H */