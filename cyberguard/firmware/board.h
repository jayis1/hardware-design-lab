/*
 * board.h - CyberGuard Pin Definitions
 * STM32L562QE-based Multi-Protocol Hardware Security Token
 */

#ifndef BOARD_H
#define BOARD_H

#include "registers.h"

/* ============================================================
 * GPIO Pin Definitions
 * ============================================================ */

/* Port A pins */
#define PIN_USB_DM          11  /* PA11 - USB Data Minus */
#define PIN_USB_DP          12  /* PA12 - USB Data Plus */
#define PIN_USB_VBUS        9   /* PA9  - USB VBUS Detect */
#define PIN_LED_RED         0   /* PA0  - TIM2_CH1 LED Red */
#define PIN_LED_BLUE        1   /* PA1  - TIM2_CH2 LED Blue */
#define PIN_BLE_TX         2   /* PA2  - USART2_TX to nRF52833 RX */
#define PIN_BLE_RX          3   /* PA3  - USART2_RX from nRF52833 TX */
#define PIN_NFC_NSS         4   /* PA4  - SPI1_NSS for PN7150 */
#define PIN_NFC_SCK         5   /* PA5  - SPI1_SCK for PN7150 */
#define PIN_NFC_MISO        6   /* PA6  - SPI1_MISO from PN7150 */
#define PIN_NFC_MOSI        7   /* PA7  - SPI1_MOSI to PN7150 */
#define PIN_FLASH_WP        15  /* PA15 - QSPI Flash Write Protect */

/* Port B pins */
#define PIN_SE_SCL          0   /* PB0  - I2C1_SCL to A71CH */
#define PIN_FP_TX           1   /* PB1  - USART1_TX to FPC1025 */
#define PIN_FP_RX           2   /* PB2  - USART1_RX from FPC1025 */
#define PIN_FLASH_NSS       12  /* PB12 - SPI2_NSS for MX25R1635F */
#define PIN_FLASH_SCK       13  /* PB13 - SPI2_SCK for MX25R1635F */
#define PIN_FLASH_MISO      14  /* PB14 - SPI2_MISO for MX25R1635F */
#define PIN_FLASH_MOSI      15  /* PB15 - SPI2_MOSI for MX25R1635F */
#define PIN_PMIC_SCL        10  /* PB10 - I2C2_SCL to STPMIC1 */
#define PIN_PMIC_SDA        11  /* PB11 - I2C2_SDA to STPMIC1 */

/* Port C pins */
#define PIN_nBLE_RST        0   /* PC0  - nRF52833 Reset */
#define PIN_BLE_MODE        1   /* PC1  - BLE Mode Select */
#define PIN_NFC_IRQ         2   /* PC2  - PN7150 Interrupt */
#define PIN_nNFC_RST        3   /* PC3  - PN7150 Reset */
#define PIN_SE_IRQ          4   /* PC4  - A71CH Interrupt */
#define PIN_SE_SDA          5   /* PC5  - I2C1_SDA to A71CH */
#define PIN_LED_GREEN       15  /* PC15 - LED Green */
#define PIN_FLASH_D2        6   /* PC6  - QSPI Flash Data 2 */
#define PIN_FLASH_D3        7   /* PC7  - QSPI Flash Data 3 */
#define PIN_nFP_RST         13  /* PC13 - FPC1025 Reset */

/* Port H pins */
#define PIN_OSC32_IN        0   /* PH0  - 32.768 kHz crystal */
#define PIN_OSC32_OUT       1   /* PH1  - 32.768 kHz crystal */

/* ============================================================
 * GPIO Port Base Addresses
 * ============================================================ */
#define GPIOA_BASE_ADDR     0x42020000UL
#define GPIOB_BASE_ADDR     0x42020400UL
#define GPIOC_BASE_ADDR     0x42020800UL
#define GPIOH_BASE_ADDR     0x42021C00UL

/* ============================================================
 * LED Control Macros
 * ============================================================ */
#define LED_RED_ON()        (GPIOA->ODR &= ~(1U << PIN_LED_RED))
#define LED_RED_OFF()       (GPIOA->ODR |= (1U << PIN_LED_RED))
#define LED_RED_TOGGLE()    (GPIOA->ODR ^= (1U << PIN_LED_RED))

#define LED_GREEN_ON()      (GPIOC->ODR &= ~(1U << PIN_LED_GREEN))
#define LED_GREEN_OFF()     (GPIOC->ODR |= (1U << PIN_LED_GREEN))
#define LED_GREEN_TOGGLE()  (GPIOC->ODR ^= (1U << PIN_LED_GREEN))

#define LED_BLUE_ON()       (GPIOA->ODR &= ~(1U << PIN_LED_BLUE))
#define LED_BLUE_OFF()      (GPIOA->ODR |= (1U << PIN_LED_BLUE))
#define LED_BLUE_TOGGLE()   (GPIOA->ODR ^= (1U << PIN_LED_BLUE))

/* ============================================================
 * Reset Control Macros (Active-Low)
 * ============================================================ */
#define SE_RESET_ASSERT()   (GPIOC->ODR &= ~(1U << PIN_nSE_RST))
#define SE_RESET_RELEASE()  (GPIOC->ODR |= (1U << PIN_nSE_RST))

#define NFC_RESET_ASSERT()  (GPIOC->ODR &= ~(1U << PIN_nNFC_RST))
#define NFC_RESET_RELEASE() (GPIOC->ODR |= (1U << PIN_nNFC_RST))

#define FP_RESET_ASSERT()   (GPIOC->ODR &= ~(1U << PIN_nFP_RST))
#define FP_RESET_RELEASE()  (GPIOC->ODR |= (1U << PIN_nFP_RST))

#define BLE_RESET_ASSERT()  (GPIOC->ODR &= ~(1U << PIN_nBLE_RST))
#define BLE_RESET_RELEASE() (GPIOC->ODR |= (1U << PIN_nBLE_RST))

/* ============================================================
 * Chip Select Macros (Active-Low, SPI)
 * ============================================================ */
#define NFC_CS_ASSERT()     (GPIOA->ODR &= ~(1U << PIN_NFC_NSS))
#define NFC_CS_RELEASE()    (GPIOA->ODR |= (1U << PIN_NFC_NSS))

#define FLASH_CS_ASSERT()   (GPIOB->ODR &= ~(1U << PIN_FLASH_NSS))
#define FLASH_CS_RELEASE()  (GPIOB->ODR |= (1U << PIN_FLASH_NSS))

/* ============================================================
 * Clock Configuration
 * ============================================================ */
#define HSI_CLOCK           16000000UL   /* 16 MHz HSI */
#define MSI_CLOCK           4000000UL    /* 4 MHz MSI (default) */
#define PLL_CLOCK           110000000UL  /* 110 MHz PLL output */
#define SYS_CLOCK           110000000UL  /* 110 MHz system clock */
#define AHB_CLOCK           110000000UL  /* 110 MHz AHB */
#define APB1_CLOCK          110000000UL  /* 110 MHz APB1 */
#define APB2_CLOCK          110000000UL  /* 110 MHz APB2 */

/* USART baud rates */
#define USART1_BAUD         115200UL     /* Fingerprint sensor */
#define USART2_BAUD         921600UL     /* BLE coprocessor */

/* SPI clock frequencies */
#define SPI1_CLOCK          10000000UL   /* 10 MHz NFC SPI */
#define SPI2_CLOCK          80000000UL   /* 80 MHz QSPI Flash */

/* I2C clock frequency */
#define I2C1_CLOCK          400000UL     /* 400 kHz Fast Mode */
#define I2C2_CLOCK          400000UL     /* 400 kHz Fast Mode */

/* ============================================================
 * I2C Device Addresses (7-bit)
 * ============================================================ */
#define A71CH_I2C_ADDR      0x48
#define STPMIC1_I2C_ADDR    0x08

/* ============================================================
 * Power Rails (mV)
 * ============================================================ */
#define VDD_3V3_MV          3300
#define VDD_1V8_MV          1800
#define VDD_1V1_MV          1100
#define VBUS_MV             5000
#define VBAT_CR2032_MV      3000

/* ============================================================
 * LED Color Codes (for status indication)
 * ============================================================ */
typedef enum {
    LED_COLOR_OFF    = 0,
    LED_COLOR_RED    = 1,
    LED_COLOR_GREEN  = 2,
    LED_COLOR_YELLOW = 3,  /* Red + Green */
    LED_COLOR_BLUE   = 4,
    LED_COLOR_PURPLE = 5,  /* Red + Blue */
    LED_COLOR_CYAN   = 6,  /* Green + Blue */
    LED_COLOR_WHITE  = 7,  /* Red + Green + Blue */
} led_color_t;

/* ============================================================
 * CTAP2 Command IDs
 * ============================================================ */
#define CTAP2_CMD_MAKE_CREDENTIAL     0x01
#define CTAP2_CMD_GET_ASSERTION       0x02
#define CTAP2_CMD_GET_INFO            0x04
#define CTAP2_CMD_CLIENT_PIN          0x06
#define CTAP2_CMD_RESET               0x07
#define CTAP2_CMD_BIO_ENROLL          0x09
#define CTAP2_CMD_CREDENTIAL_MGMT     0x0A

/* ============================================================
 * Firmware Version
 * ============================================================ */
#define FW_VERSION_MAJOR    1
#define FW_VERSION_MINOR    0
#define FW_VERSION_PATCH    0
#define FW_VERSION_STRING   "1.0.0"

#endif /* BOARD_H */