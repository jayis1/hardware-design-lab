/*
 * board.h — Chronos-RTK pin definitions and board constants
 * MCU: STM32G474RET6 (LQFP-64)
 */

#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>

/* ========================================================================== */
/* Clock configuration                                                        */
/* ========================================================================== */
#define HSE_VALUE          32000000UL   /* 32 MHz external crystal */
#define LSE_VALUE          32768UL      /* 32.768 kHz LSE crystal */
#define SYSCLK_FREQ        170000000UL  /* 170 MHz system clock */
#define AHB_FREQ            SYSCLK_FREQ  /* HCLK = 170 MHz */
#define APB1_FREQ           SYSCLK_FREQ  /* APB1 = 170 MHz */
#define APB2_FREQ           SYSCLK_FREQ  /* APB2 = 170 MHz */
#define HSI48_VALUE         48000000UL  /* 48 MHz USB RC oscillator */

/* ========================================================================== */
/* GPIO port base addresses                                                    */
/* ========================================================================== */
#define GPIOA_BASE         0x48000000UL
#define GPIOB_BASE         0x48000400UL
#define GPIOC_BASE         0x48000800UL
#define GPIOD_BASE         0x48000C00UL

#define GPIOA              ((GPIO_TypeDef *)GPIOA_BASE)
#define GPIOB              ((GPIO_TypeDef *)GPIOB_BASE)
#define GPIOC              ((GPIO_TypeDef *)GPIOC_BASE)
#define GPIOD              ((GPIO_TypeDef *)GPIOD_BASE)

/* ========================================================================== */
/* Pin definitions — STM32G474RET6 LQFP-64                                    */
/* ========================================================================== */

/* --- UART1 (GNSS) --- */
#define PIN_UART1_TX       2    /* PA2, AF7 */
#define PORT_UART1_TX      GPIOA
#define PIN_UART1_RX       3    /* PA3, AF7 */
#define PORT_UART1_RX      GPIOA
#define UART1_BAUDRATE     460800

/* --- SPI1 (Flash) --- */
#define PIN_SPI1_NSS       4    /* PA4, GPIO output (software CS) */
#define PORT_SPI1_NSS      GPIOA
#define PIN_SPI1_SCK       5    /* PA5, AF5 */
#define PORT_SPI1_SCK      GPIOA
#define PIN_SPI1_MISO      6    /* PA6, AF5 */
#define PORT_SPI1_MISO     GPIOA
#define PIN_SPI1_MOSI      7    /* PA7, AF5 */
#define PORT_SPI1_MOSI     GPIOA
#define SPI1_SPEED_HZ      50000000UL  /* 50 MHz max */

/* --- SPI2 (LoRa) --- */
#define PIN_SPI2_SCK       0    /* PA0, AF5 */
#define PORT_SPI2_SCK      GPIOA
#define PIN_SPI2_NSS       1    /* PA1, GPIO output (software CS) */
#define PORT_SPI2_NSS      GPIOA
#define PIN_SPI2_MOSI      3    /* PC3, AF5 */
#define PORT_SPI2_MOSI     GPIOC
#define PIN_SPI2_MISO      10   /* PB10 used for I2C; SPI2 uses single-wire in some configs */
#define PORT_SPI2_MISO     GPIOB
#define SPI2_SPEED_HZ      16000000UL  /* 16 MHz max */

/* --- I2C1 (OLED + BME280) --- */
#define PIN_I2C1_SCL       10   /* PB10, AF4 */
#define PORT_I2C1_SCL      GPIOB
#define PIN_I2C1_SDA       11   /* PB11, AF4 */
#define PORT_I2C1_SDA      GPIOB
#define I2C1_SPEED_HZ      400000  /* 400 kHz fast mode */

/* --- USB --- */
#define PIN_USB_DM         11   /* PA11, AF10 */
#define PORT_USB_DM        GPIOA
#define PIN_USB_DP         12   /* PA12, AF10 */
#define PORT_USB_DP        GPIOA
#define PIN_USB_VBUS       9    /* PA9, input */
#define PORT_USB_VBUS      GPIOA
#define PIN_USB_ID         10   /* PA10, tied to GND */
#define PORT_USB_ID        GPIOA

/* --- LoRa SX1262 control --- */
#define PIN_LORA_BUSY      4    /* PC4, input */
#define PORT_LORA_BUSY     GPIOC
#define PIN_LORA_RST       5    /* PC5, output */
#define PORT_LORA_RST      GPIOC
#define PIN_LORA_DIO1      0    /* PB0, EXTI0 */
#define PORT_LORA_DIO1     GPIOB
#define PIN_LORA_TCXO_EN  1    /* PB1, TIM3_CH4 or GPIO */
#define PORT_LORA_TCXO_EN GPIOB
#define PIN_LORA_ANT_SW    9    /* PC9, GPIO */
#define PORT_LORA_ANT_SW   GPIOC

/* --- GNSS ZED-F9P control --- */
#define PIN_GNSS_RST       2    /* PB2, output */
#define PORT_GNSS_RST      GPIOB
#define PIN_GNSS_PPS       15   /* PB15, input */
#define PORT_GNSS_PPS      GPIOB
#define PIN_GNSS_SAFE      7    /* PC7, output */
#define PORT_GNSS_SAFE     GPIOC
#define PIN_GNSS_INT       8    /* PC8, input */
#define PORT_GNSS_INT      GPIOC
#define PIN_GNSS_LNA_EN   10   /* PC10, output (reserved) */
#define PORT_GNSS_LNA_EN  GPIOC

/* --- OLED SSD1306 --- */
#define PIN_OLED_RST       7    /* PB7, output */
#define PORT_OLED_RST      GPIOB
#define OLED_I2C_ADDR      0x3C

/* --- BME280 --- */
#define BME280_I2C_ADDR    0x76

/* --- W25Q128 Flash --- */
#define W25Q128_PAGE_SIZE     256
#define W25Q128_SECTOR_SIZE   4096
#define W25Q128_BLOCK_SIZE    65536
#define W25Q128_TOTAL_SIZE    (16UL * 1024UL * 1024UL)  /* 16 MB */
#define W25Q128_PAGES         (W25Q128_TOTAL_SIZE / W25Q128_PAGE_SIZE)

/* --- Power control --- */
#define PIN_POWER_EN       3    /* PB3, output (3.3V rail enable) */
#define PORT_POWER_EN      GPIOB
#define PIN_CHARGE_EN      4    /* PB4, output (MCP73871 CE#, active low) */
#define PORT_CHARGE_EN     GPIOB
#define PIN_CHARGE_STAT    5    /* PB5, input */
#define PORT_CHARGE_STAT   GPIOB
#define PIN_CHARGE_STAT2   6    /* PB6, input */
#define PORT_CHARGE_STAT2  GPIOB

/* --- LEDs --- */
#define PIN_LED_R          12   /* PB12 */
#define PORT_LED_R         GPIOB
#define PIN_LED_G          13   /* PB13 */
#define PORT_LED_G         GPIOB
#define PIN_LED_B          14   /* PB14 */
#define PORT_LED_B         GPIOB

/* --- Buttons --- */
#define PIN_BTN_MODE       14   /* PC14, input, active low */
#define PORT_BTN_MODE      GPIOC
#define PIN_BTN_SEL        15   /* PC15, input, active low */
#define PORT_BTN_SEL       GPIOC
#define PIN_BTN_ACT        2    /* PC2, input, active low */
#define PORT_BTN_ACT       GPIOC

/* --- ADC channels --- */
#define PIN_VBAT_SENSE     0    /* PC0/ADC12_IN1 */
#define PORT_VBAT_SENSE    GPIOC
#define PIN_TEMP_SENSE     1    /* PC1/ADC12_IN2 */
#define PORT_TEMP_SENSE    GPIOC

/* --- Debug port (SWD) --- */
#define PIN_SWDIO          13   /* PA13 */
#define PORT_SWDIO         GPIOA
#define PIN_SWCLK          14   /* PA14 */
#define PORT_SWCLK         GPIOA

/* --- Flash control --- */
#define PIN_FLASH_WP       15   /* PA15 */
#define PORT_FLASH_WP     GPIOA
#define PIN_FLASH_HOLD     2    /* PD2 */
#define PORT_FLASH_HOLD   GPIOD

/* ========================================================================== */
/* LED helper macros                                                           */
/* ========================================================================== */
#define LED_R_ON()         (PORT_LED_R->ODR &= ~(1U << PIN_LED_R))
#define LED_R_OFF()        (PORT_LED_R->ODR |=  (1U << PIN_LED_R))
#define LED_G_ON()         (PORT_LED_G->ODR &= ~(1U << PIN_LED_G))
#define LED_G_OFF()        (PORT_LED_G->ODR |=  (1U << PIN_LED_G))
#define LED_B_ON()         (PORT_LED_B->ODR &= ~(1U << PIN_LED_B))
#define LED_B_OFF()        (PORT_LED_B->ODR |=  (1U << PIN_LED_B))

/* ========================================================================== */
/* LoRa configuration                                                          */
/* ========================================================================== */
#define LORA_FREQ_EU       868000000UL   /* 868 MHz EU band */
#define LORA_FREQ_US       915000000UL   /* 915 MHz US band */
#define LORA_SF             7             /* Spreading factor 7 */
#define LORA_BW             125000UL      /* 125 kHz bandwidth */
#define LORA_CR             4            /* Coding rate 4/5 */
#define LORA_TX_POWER       22            /* +22 dBm */
#define LORA_SYNC_WORD      0x12          /* Private network */
#define LORA_PREAMBLE_LEN   8
#define LORA_MAX_PAYLOAD    255

/* ========================================================================== */
/* GNSS configuration                                                          */
/* ========================================================================== */
#define GNSS_UART_BAUD      460800
#define GNSS_MSG_BUF_SIZE   4096

/* ========================================================================== */
/* USB configuration                                                           */
/* ========================================================================== */
#define USB_VID             0x0483   /* STMicroelectronics */
#define USB_PID             0x5740   /* Custom CDC device */
#define USB_MAX_PACKET      64

#endif /* BOARD_H */