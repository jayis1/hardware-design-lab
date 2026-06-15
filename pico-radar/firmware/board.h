/*
 * board.h — PicoRadar pin definitions for STM32H743VIT6
 */

#ifndef BOARD_H
#define BOARD_H

/* ---- GPIO Port Base Addresses ---- */
#define GPIOA   0x58020000UL
#define GPIOB   0x58020400UL
#define GPIOC   0x58020800UL
#define GPIOD   0x58020C00UL
#define GPIOE   0x58021000UL

/* ---- Radar Host Interface (USART1) ---- */
#define RADAR_UART_TX_PIN       9       /* PA9  = USART1_TX  (AF7) */
#define RADAR_UART_TX_PORT      GPIOA
#define RADAR_UART_RX_PIN       10      /* PA10 = USART1_RX  (AF7) */
#define RADAR_UART_RX_PORT      GPIOA
#define RADAR_UART_CTS_PIN      11      /* PA11 = USART1_CTS (AF7) */
#define RADAR_UART_CTS_PORT     GPIOA
#define RADAR_UART_RTS_PIN      12      /* PA12 = USART1_RTS (AF7) */
#define RADAR_UART_RTS_PORT     GPIOA

/* ---- Radar Control GPIOs ---- */
#define RADAR_INTR_PIN          6       /* PC6  = HOST_INTR input */
#define RADAR_INTR_PORT         GPIOC
#define RADAR_NRST_PIN          0       /* PD0  = NRST output (active low) */
#define RADAR_NRST_PORT         GPIOD
#define RADAR_SYNC_IN_PIN       1       /* PD1  = SYNC_IN output */
#define RADAR_SYNC_IN_PORT      GPIOD
#define RADAR_SYNC_OUT_PIN      2       /* PD2  = SYNC_OUT input */
#define RADAR_SYNC_OUT_PORT     GPIOD

/* ---- ESP32-C3 (USART2) ---- */
#define ESP_UART_TX_PIN         5       /* PD5  = USART2_TX (AF7) */
#define ESP_UART_TX_PORT        GPIOD
#define ESP_UART_RX_PIN         6       /* PD6  = USART2_RX (AF7) */
#define ESP_UART_RX_PORT        GPIOD
#define ESP_EN_PIN              7       /* PC7  = ESP32 EN (active low) */
#define ESP_EN_PORT             GPIOC
#define ESP_BOOT_PIN            8       /* PC8  = ESP32 BOOT mode */
#define ESP_BOOT_PORT           GPIOC

/* ---- SPI1 (IMU ICM-42688-P) ---- */
#define IMU_SCK_PIN             5       /* PA5  = SPI1_SCK  (AF5) */
#define IMU_SCK_PORT            GPIOA
#define IMU_MISO_PIN            6       /* PA6  = SPI1_MISO (AF5) */
#define IMU_MISO_PORT           GPIOA
#define IMU_MOSI_PIN            5       /* PB5  = SPI1_MOSI (AF5) */
#define IMU_MOSI_PORT           GPIOB
#define IMU_CS_PIN              4       /* PA4  = SPI1_NSS (manual GPIO) */
#define IMU_CS_PORT             GPIOA
#define IMU_INT1_PIN            9       /* PC9  = INT1 input */
#define IMU_INT1_PORT           GPIOC

/* ---- I2C1 (OLED SH1106 + PMIC TPS65263) ---- */
#define I2C_SCL_PIN             6       /* PB6  = I2C1_SCL (AF4) */
#define I2C_SCL_PORT            GPIOB
#define I2C_SDA_PIN             7       /* PB7  = I2C1_SDA (AF4) */
#define I2C_SDA_PORT            GPIOB

/* ---- Ethernet RMII (LAN8720A) ---- */
#define ETH_REFCLK_PIN          2       /* PA2  = RMII_REFCLK (AF11) */
#define ETH_REFCLK_PORT         GPIOA
#define ETH_MDC_PIN             1       /* PC1  = RMII_MDC (AF11) */
#define ETH_MDC_PORT            GPIOC
#define ETH_MDIO_PIN            7       /* PA7  = RMII_MDIO (AF11) */
#define ETH_MDIO_PORT           GPIOA
#define ETH_TXD0_PIN            13      /* PB13 = RMII_TXD0 (AF11) */
#define ETH_TXD0_PORT           GPIOB
#define ETH_TXD1_PIN            14      /* PB14 = RMII_TXD1 (AF11) */
#define ETH_TXD1_PORT           GPIOB
#define ETH_TXEN_PIN            11      /* PB11 = RMII_TXEN (AF11) */
#define ETH_TXEN_PORT           GPIOB
#define ETH_RXD0_PIN            4       /* PC4  = RMII_RXD0 (AF11) */
#define ETH_RXD0_PORT           GPIOC
#define ETH_RXD1_PIN            5       /* PC5  = RMII_RXD1 (AF11) */
#define ETH_RXD1_PORT           GPIOC
#define ETH_RXDV_PIN            0       /* PB0  = RMII_CRS_DV (AF11) */
#define ETH_RXDV_PORT           GPIOB
#define ETH_nRST_PIN            3       /* PD3  = PHY nRST (active low) */
#define ETH_nRST_PORT           GPIOD
#define ETH_nINT_PIN            3       /* PD3 alt — see pin conflict note */
#define ETH_nINT_PORT           GPIOD   /* Actually use PB3 or remap */

/* ---- QSPI Flash (MX25L25645G) ---- */
#define QSPI_CLK_PIN            2       /* PE2  = QSPI_CLK (AF9) */
#define QSPI_CLK_PORT           GPIOE
#define QSPI_NCS_PIN            3       /* PE3  = QSPI_NCS (AF9) */
#define QSPI_NCS_PORT           GPIOE
#define QSPI_DQ0_PIN            4       /* PE4  = QSPI_DQ0 (AF9) */
#define QSPI_DQ0_PORT           GPIOE
#define QSPI_DQ1_PIN            5       /* PE5  = QSPI_DQ1 (AF9) */
#define QSPI_DQ1_PORT           GPIOE
#define QSPI_DQ2_PIN            6       /* PE6  = QSPI_DQ2 (AF9) */
#define QSPI_DQ2_PORT           GPIOE
#define QSPI_DQ3_PIN            7       /* PE7  = QSPI_DQ3 (AF9) */
#define QSPI_DQ3_PORT           GPIOE

/* ---- USB FS ---- */
#define USB_DM_PIN              8       /* PD8  = USB_DM (AF10) */
#define USB_DM_PORT             GPIOD
#define USB_DP_PIN              9       /* PD9  = USB_DP (AF10) */
#define USB_DP_PORT             GPIOD

/* ---- SWD Debug ---- */
#define SWD_SWDIO_PIN           13      /* PA13 */
#define SWD_SWDIO_PORT          GPIOA
#define SWD_SWCLK_PIN           14      /* PA14 */
#define SWD_SWCLK_PORT          GPIOA

/* ---- Power ---- */
#define PGOOD_PIN               5       /* PB5 — shared, use alternate */
#define PGOOD_PORT              GPIOB

/* ---- Radar SPI Flash (IWR6843 dedicated) ---- */
#define RADAR_FLASH_CS_PIN      0       /* Not connected to STM32 (IWR6843 manages) */
#define RADAR_FLASH_CLK_PIN     0
#define RADAR_FLASH_MOSI_PIN    0
#define RADAR_FLASH_MISO_PIN    0

/* ---- Crystal Pins ---- */
#define HSE_OSC_IN_PIN          0       /* PH0 — 8 MHz HSE crystal */
#define HSE_OSC_IN_PORT         0x58021C00UL  /* GPIOH */
#define HSE_OSC_OUT_PIN         1       /* PH1 — 8 MHz HSE crystal */
#define HSE_OSC_OUT_PORT        0x58021C00UL

/* ---- I2C Device Addresses ---- */
#define OLED_I2C_ADDR            0x3C
#define PMIC_I2C_ADDR            0x5A

/* ---- Clock frequencies ---- */
#define SYSCLK_FREQ              480000000UL  /* Cortex-M7 @ 480 MHz */
#define AHB_FREQ                 240000000UL
#define APB1_FREQ                120000000UL
#define APB2_FREQ                120000000UL
#define APB4_FREQ                120000000UL
#define HSE_FREQ                 8000000UL
#define LSE_FREQ                 32768UL

/* ---- UART Baud Rates ---- */
#define RADAR_UART_BAUD          921600UL
#define ESP_UART_BAUD            115200UL

/* ---- SPI Clock ---- */
#define IMU_SPI_FREQ             15000000UL  /* 15 MHz */

#endif /* BOARD_H */