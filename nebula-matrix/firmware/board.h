/**
 * board.h — Nebula Matrix Board Pin Definitions
 *
 * Complete pin mapping for STM32H743VIT6 (LQFP-100) on the Nebula Matrix PCB.
 * Includes all alternate functions, peripheral assignments, and signal descriptions.
 *
 * Pin naming convention: P<port><pin>_<function>
 * Example: PA9_USART1_TX = Port A, Pin 9, USART1 TX function
 */

#ifndef BOARD_H
#define BOARD_H

#include "stm32h7xx_hal.h"

/* =========================================================================
 * Clock Sources
 * ========================================================================= */

/* HSE: 25 MHz from Si5351A CLK0 (bypass mode, external oscillator) */
#define HSE_FREQUENCY_HZ        25000000UL
#define HSE_BYPASS_MODE         1       /* External clock, no internal oscillator */

/* LSE: Not used (no RTC requirement) */
#define LSE_ENABLED             0

/* System clock target */
#define SYSCLK_FREQUENCY_HZ     480000000UL
#define HCLK_FREQUENCY_HZ       240000000UL
#define APB1_FREQUENCY_HZ       120000000UL
#define APB2_FREQUENCY_HZ       120000000UL

/* =========================================================================
 * FPGA Interface Pins
 * ========================================================================= */

/* SPI2 — Pixel Data Channel (STM32 → FPGA, 50 MHz)
 * AF5, Very High Speed, Push-Pull, No Pull */
#define FPGA_SPI2_SCK_PIN       GPIO_PIN_13
#define FPGA_SPI2_SCK_PORT      GPIOB
#define FPGA_SPI2_SCK_AF        GPIO_AF5_SPI2

#define FPGA_SPI2_MISO_PIN      GPIO_PIN_14
#define FPGA_SPI2_MISO_PORT     GPIOB
#define FPGA_SPI2_MISO_AF       GPIO_AF5_SPI2

#define FPGA_SPI2_MOSI_PIN      GPIO_PIN_15
#define FPGA_SPI2_MOSI_PORT     GPIOB
#define FPGA_SPI2_MOSI_AF       GPIO_AF5_SPI2

#define FPGA_SPI2_NSS_PIN       GPIO_PIN_12
#define FPGA_SPI2_NSS_PORT      GPIOB
#define FPGA_SPI2_NSS_AF        GPIO_AF5_SPI2

/* UART1 — Command Channel (STM32 ↔ FPGA, 3 Mbps)
 * AF7, High Speed, Push-Pull */
#define FPGA_UART1_TX_PIN       GPIO_PIN_9
#define FPGA_UART1_TX_PORT      GPIOA
#define FPGA_UART1_TX_AF        GPIO_AF7_USART1

#define FPGA_UART1_RX_PIN       GPIO_PIN_10
#define FPGA_UART1_RX_PORT      GPIOA
#define FPGA_UART1_RX_AF        GPIO_AF7_USART1

/* FPGA Control Signals */
#define FPGA_IRQ_PIN            GPIO_PIN_0
#define FPGA_IRQ_PORT           GPIOE
#define FPGA_IRQ_EXTI_LINE      EXTI_LINE_0
#define FPGA_IRQ_IRQN           EXTI0_IRQn

#define FPGA_READY_PIN          GPIO_PIN_1
#define FPGA_READY_PORT         GPIOE

#define FPGA_RESET_N_PIN        GPIO_PIN_2
#define FPGA_RESET_N_PORT       GPIOE

#define FPGA_DONE_PIN           GPIO_PIN_3
#define FPGA_DONE_PORT          GPIOE
#define FPGA_DONE_EXTI_LINE     EXTI_LINE_3
#define FPGA_DONE_IRQN          EXTI3_IRQn

/* =========================================================================
 * SPI1 — FPGA Configuration Flash (W25Q128JV)
 * AF5, Very High Speed, Push-Pull
 * ========================================================================= */

#define FPGA_FLASH_SPI1_NSS_PIN     GPIO_PIN_4
#define FPGA_FLASH_SPI1_NSS_PORT    GPIOA
#define FPGA_FLASH_SPI1_NSS_AF      GPIO_AF5_SPI1

#define FPGA_FLASH_SPI1_SCK_PIN     GPIO_PIN_5
#define FPGA_FLASH_SPI1_SCK_PORT    GPIOA
#define FPGA_FLASH_SPI1_SCK_AF      GPIO_AF5_SPI1

#define FPGA_FLASH_SPI1_MISO_PIN    GPIO_PIN_6
#define FPGA_FLASH_SPI1_MISO_PORT   GPIOA
#define FPGA_FLASH_SPI1_MISO_AF     GPIO_AF5_SPI1

#define FPGA_FLASH_SPI1_MOSI_PIN    GPIO_PIN_7
#define FPGA_FLASH_SPI1_MOSI_PORT   GPIOA
#define FPGA_FLASH_SPI1_MOSI_AF     GPIO_AF5_SPI1

/* =========================================================================
 * Ethernet RMII Interface (KSZ9031RNX)
 * AF11, Very High Speed, Push-Pull
 * ========================================================================= */

#define ETH_RMII_CRS_DV_PIN     GPIO_PIN_8
#define ETH_RMII_CRS_DV_PORT    GPIOA
#define ETH_RMII_CRS_DV_AF      GPIO_AF11_ETH

#define ETH_RMII_TX_EN_PIN      GPIO_PIN_0
#define ETH_RMII_TX_EN_PORT     GPIOC
#define ETH_RMII_TX_EN_AF       GPIO_AF11_ETH

#define ETH_MDC_PIN             GPIO_PIN_1
#define ETH_MDC_PORT            GPIOC
#define ETH_MDC_AF              GPIO_AF11_ETH

#define ETH_RMII_TXD0_PIN       GPIO_PIN_2
#define ETH_RMII_TXD0_PORT      GPIOC
#define ETH_RMII_TXD0_AF        GPIO_AF11_ETH

#define ETH_RMII_TXD1_PIN       GPIO_PIN_3
#define ETH_RMII_TXD1_PORT      GPIOC
#define ETH_RMII_TXD1_AF        GPIO_AF11_ETH

#define ETH_RMII_RXD0_PIN       GPIO_PIN_4
#define ETH_RMII_RXD0_PORT      GPIOC
#define ETH_RMII_RXD0_AF        GPIO_AF11_ETH

#define ETH_RMII_RXD1_PIN       GPIO_PIN_5
#define ETH_RMII_RXD1_PORT      GPIOC
#define ETH_RMII_RXD1_AF        GPIO_AF11_ETH

#define ETH_RMII_RXER_PIN       GPIO_PIN_5
#define ETH_RMII_RXER_PORT      GPIOB
#define ETH_RMII_RXER_AF        GPIO_AF11_ETH

#define ETH_RMII_REF_CLK_PIN    GPIO_PIN_0
#define ETH_RMII_REF_CLK_PORT   GPIOH
#define ETH_RMII_REF_CLK_AF     GPIO_AF11_ETH

#define ETH_MDIO_PIN            GPIO_PIN_1
#define ETH_MDIO_PORT           GPIOH
#define ETH_MDIO_AF             GPIO_AF11_ETH

/* Ethernet PHY Control */
#define ETH_RESET_N_PIN         GPIO_PIN_10
#define ETH_RESET_N_PORT        GPIOE

#define ETH_INT_N_PIN           GPIO_PIN_12
#define ETH_INT_N_PORT          GPIOE
#define ETH_INT_N_EXTI_LINE     EXTI_LINE_12
#define ETH_INT_N_IRQN          EXTI15_10_IRQn

/* =========================================================================
 * USB ULPI Interface (USB3320C)
 * AF10, Very High Speed, Push-Pull
 * ========================================================================= */

#define USB_OTG_FS_DM_PIN       GPIO_PIN_11
#define USB_OTG_FS_DM_PORT      GPIOA
#define USB_OTG_FS_DM_AF        GPIO_AF10_OTG1_FS

#define USB_OTG_FS_DP_PIN       GPIO_PIN_12
#define USB_OTG_FS_DP_PORT      GPIOA
#define USB_OTG_FS_DP_AF        GPIO_AF10_OTG1_FS

/* USB PHY Control */
#define USB_RESET_N_PIN         GPIO_PIN_11
#define USB_RESET_N_PORT        GPIOE

#define USB_INT_N_PIN           GPIO_PIN_13
#define USB_INT_N_PORT          GPIOE
#define USB_INT_N_EXTI_LINE     EXTI_LINE_13
#define USB_INT_N_IRQN          EXTI15_10_IRQn

/* =========================================================================
 * SDMMC1 Interface (microSD Socket)
 * AF12, Very High Speed, Push-Pull, Pull-Up
 * ========================================================================= */

#define SDMMC1_D6_PIN           GPIO_PIN_6
#define SDMMC1_D6_PORT          GPIOC
#define SDMMC1_D6_AF            GPIO_AF12_SDMMC1

#define SDMMC1_D7_PIN           GPIO_PIN_7
#define SDMMC1_D7_PORT          GPIOC
#define SDMMC1_D7_AF            GPIO_AF12_SDMMC1

#define SDMMC1_D0_PIN           GPIO_PIN_8
#define SDMMC1_D0_PORT          GPIOC
#define SDMMC1_D0_AF            GPIO_AF12_SDMMC1

#define SDMMC1_D1_PIN           GPIO_PIN_9
#define SDMMC1_D1_PORT          GPIOC
#define SDMMC1_D1_AF            GPIO_AF12_SDMMC1

#define SDMMC1_D2_PIN           GPIO_PIN_10
#define SDMMC1_D2_PORT          GPIOC
#define SDMMC1_D2_AF            GPIO_AF12_SDMMC1

#define SDMMC1_D3_PIN           GPIO_PIN_11
#define SDMMC1_D3_PORT          GPIOC
#define SDMMC1_D3_AF            GPIO_AF12_SDMMC1

#define SDMMC1_CK_PIN           GPIO_PIN_12
#define SDMMC1_CK_PORT          GPIOC
#define SDMMC1_CK_AF            GPIO_AF12_SDMMC1

#define SDMMC1_CMD_PIN          GPIO_PIN_2
#define SDMMC1_CMD_PORT         GPIOD
#define SDMMC1_CMD_AF           GPIO_AF12_SDMMC1

/* =========================================================================
 * I2C Buses
 * ========================================================================= */

/* I2C2 — Temperature Sensors (TMP117 ×4)
 * AF4, Open-Drain, Low Speed, External Pull-Up (4.7kΩ to +3V3) */

/* Primary I2C2 pair (TMP117 #1) */
#define I2C2_SCL_PIN            GPIO_PIN_6
#define I2C2_SCL_PORT           GPIOB
#define I2C2_SCL_AF             GPIO_AF4_I2C2

#define I2C2_SDA_PIN            GPIO_PIN_7
#define I2C2_SDA_PORT           GPIOB
#define I2C2_SDA_AF             GPIO_AF4_I2C2

/* Secondary I2C2 pair (TMP117 #2) */
#define I2C2_SCL2_PIN           GPIO_PIN_4
#define I2C2_SCL2_PORT          GPIOE
#define I2C2_SCL2_AF            GPIO_AF4_I2C2

#define I2C2_SDA2_PIN           GPIO_PIN_5
#define I2C2_SDA2_PORT          GPIOE
#define I2C2_SDA2_AF            GPIO_AF4_I2C2

/* Tertiary I2C2 pair (TMP117 #3) */
#define I2C2_SCL3_PIN           GPIO_PIN_6
#define I2C2_SCL3_PORT          GPIOE
#define I2C2_SCL3_AF            GPIO_AF4_I2C2

#define I2C2_SDA3_PIN           GPIO_PIN_7
#define I2C2_SDA3_PORT          GPIOE
#define I2C2_SDA3_AF            GPIO_AF4_I2C2

/* Quaternary I2C2 pair (TMP117 #4) */
#define I2C2_SCL4_PIN           GPIO_PIN_8
#define I2C2_SCL4_PORT          GPIOE
#define I2C2_SCL4_AF            GPIO_AF4_I2C2

#define I2C2_SDA4_PIN           GPIO_PIN_9
#define I2C2_SDA4_PORT          GPIOE
#define I2C2_SDA4_AF            GPIO_AF4_I2C2

/* I2C3 — Si5351A Clock Generator
 * AF4, Open-Drain, Low Speed, External Pull-Up (4.7kΩ to +3V3) */
#define I2C3_SCL_PIN            GPIO_PIN_8
#define I2C3_SCL_PORT           GPIOB
#define I2C3_SCL_AF             GPIO_AF4_I2C3

#define I2C3_SDA_PIN            GPIO_PIN_9
#define I2C3_SDA_PORT           GPIOB
#define I2C3_SDA_AF             GPIO_AF4_I2C3

/* I2C4 — TPS65218 PMIC
 * AF6, Open-Drain, Low Speed, External Pull-Up (4.7kΩ to +3V3) */
#define I2C4_SCL_PIN            GPIO_PIN_10
#define I2C4_SCL_PORT           GPIOB
#define I2C4_SCL_AF             GPIO_AF6_I2C4

#define I2C4_SDA_PIN            GPIO_PIN_11
#define I2C4_SDA_PORT           GPIOB
#define I2C4_SDA_AF             GPIO_AF6_I2C4

/* =========================================================================
 * Status LEDs
 * ========================================================================= */

#define LED_STATUS_PIN          GPIO_PIN_0   /* PB0 — Amber, system status */
#define LED_STATUS_PORT         GPIOB

#define LED_ERROR_PIN           GPIO_PIN_1   /* PB1 — Red, error indicator */
#define LED_ERROR_PORT          GPIOB

#define LED_ETH_ACT_PIN         GPIO_PIN_2   /* PB2 — Blue, Ethernet activity */
#define LED_ETH_ACT_PORT        GPIOB

/* =========================================================================
 * Fan PWM (TIM1 Channels)
 * AF1, Push-Pull, Low Speed
 * ========================================================================= */

#define FAN1_PWM_PIN            GPIO_PIN_0   /* PA0 — TIM1_CH1 */
#define FAN1_PWM_PORT           GPIOA
#define FAN1_PWM_AF             GPIO_AF1_TIM1
#define FAN1_PWM_TIM            TIM1
#define FAN1_PWM_CHANNEL        TIM_CHANNEL_1

#define FAN2_PWM_PIN            GPIO_PIN_1   /* PA1 — TIM1_CH2 */
#define FAN2_PWM_PORT           GPIOA
#define FAN2_PWM_AF             GPIO_AF1_TIM1
#define FAN2_PWM_TIM            TIM1
#define FAN2_PWM_CHANNEL        TIM_CHANNEL_2

/* =========================================================================
 * ADC Inputs
 * ========================================================================= */

#define ADC_VSHUNT_PIN          GPIO_PIN_2   /* PA2 — ADC1_IN2, INA219 Vshunt */
#define ADC_VSHUNT_PORT         GPIOA
#define ADC_VSHUNT_ADC          ADC1
#define ADC_VSHUNT_CHANNEL      ADC_CHANNEL_2

#define ADC_12V_DIV_PIN         GPIO_PIN_3   /* PA3 — ADC1_IN3, 12V divider */
#define ADC_12V_DIV_PORT        GPIOA
#define ADC_12V_DIV_ADC         ADC1
#define ADC_12V_DIV_CHANNEL     ADC_CHANNEL_3

/* =========================================================================
 * HDMI Receiver Control (ADV7611)
 * ========================================================================= */

#define HDMI_RESET_N_PIN        GPIO_PIN_3   /* PB3 — HDMI Rx reset */
#define HDMI_RESET_N_PORT       GPIOB

#define HDMI_INT1_PIN           GPIO_PIN_4   /* PB4 — HDMI Rx interrupt */
#define HDMI_INT1_PORT          GPIOB
#define HDMI_INT1_EXTI_LINE     EXTI_LINE_4
#define HDMI_INT1_IRQN          EXTI4_IRQn

/* =========================================================================
 * PMIC Control (TPS65218)
 * ========================================================================= */

#define PMIC_EN_PIN             GPIO_PIN_14  /* PE14 — PMIC enable */
#define PMIC_EN_PORT            GPIOE

#define PMIC_INT_PIN            GPIO_PIN_15  /* PE15 — PMIC interrupt */
#define PMIC_INT_PORT           GPIOE
#define PMIC_INT_EXTI_LINE      EXTI_LINE_15
#define PMIC_INT_IRQN           EXTI15_10_IRQn

/* =========================================================================
 * Debug Headers
 * ========================================================================= */

/* SWD (Tag-Connect TC2050) */
#define SWDIO_PIN               GPIO_PIN_13  /* PA13 — SWDIO (default AF0) */
#define SWDIO_PORT              GPIOA

#define SWCLK_PIN               GPIO_PIN_14  /* PA14 — SWCLK (default AF0) */
#define SWCLK_PORT              GPIOA

#define SWO_PIN                 GPIO_PIN_3   /* PB3 — SWO (default AF0, shared with HDMI_RESET) */
#define SWO_PORT                GPIOB

/* Optional debug UART (USART3) */
#define DBG_UART3_TX_PIN        GPIO_PIN_8   /* PD8 — USART3 TX, AF7 */
#define DBG_UART3_TX_PORT       GPIOD
#define DBG_UART3_TX_AF         GPIO_AF7_USART3

#define DBG_UART3_RX_PIN        GPIO_PIN_9   /* PD9 — USART3 RX, AF7 */
#define DBG_UART3_RX_PORT       GPIOD
#define DBG_UART3_RX_AF         GPIO_AF7_USART3

/* =========================================================================
 * I2C Device Addresses
 * ========================================================================= */

#define TMP117_I2C_ADDR_1       0x48  /* ADDR pin = GND */
#define TMP117_I2C_ADDR_2       0x49  /* ADDR pin = VCC */
#define TMP117_I2C_ADDR_3       0x4A  /* ADDR pin = SDA */
#define TMP117_I2C_ADDR_4       0x4B  /* ADDR pin = SCL */

#define SI5351A_I2C_ADDR        0x60  /* Fixed address */

#define TPS65218_I2C_ADDR       0x24  /* Fixed address */

#define INA219_I2C_ADDR         0x40  /* A0=GND, A1=GND */

/* =========================================================================
 * FPGA Register Addresses (via UART command interface)
 * ========================================================================= */

#define FPGA_REG_ID             0x0000
#define FPGA_REG_VERSION        0x0004
#define FPGA_REG_STATUS         0x0008
#define FPGA_REG_CONTROL        0x000C
#define FPGA_REG_MATRIX_WIDTH   0x0010
#define FPGA_REG_MATRIX_HEIGHT  0x0012
#define FPGA_REG_PANEL_SCAN     0x0014
#define FPGA_REG_PANEL_CONFIG   0x0015
#define FPGA_REG_BRIGHTNESS     0x0018
#define FPGA_REG_CONTRAST       0x001A
#define FPGA_REG_GAMMA_R_BASE   0x0020
#define FPGA_REG_GAMMA_G_BASE   0x0024
#define FPGA_REG_GAMMA_B_BASE   0x0028
#define FPGA_REG_FB0_BASE       0x0030
#define FPGA_REG_FB1_BASE       0x0034
#define FPGA_REG_FB_ACTIVE      0x0038
#define FPGA_REG_FB_SWAP        0x003C
#define FPGA_REG_INPUT_SOURCE   0x0040
#define FPGA_REG_INPUT_STATUS   0x0041
#define FPGA_REG_DITHER_MODE    0x0070
#define FPGA_REG_DITHER_STRENGTH 0x0071
#define FPGA_REG_HUB75E_REFRESH 0x0080
#define FPGA_REG_HUB75E_FRAMES  0x0084
#define FPGA_REG_SPI_PIXELS_RX  0x0090
#define FPGA_REG_SPI_FRAMES_RX  0x0094
#define FPGA_REG_SPI_ERRORS     0x0098
#define FPGA_REG_TEMP_FPGA      0x00A0

/* =========================================================================
 * FPGA Status Register Bit Definitions
 * ========================================================================= */

#define FPGA_STATUS_FB0_READY       (1 << 0)
#define FPGA_STATUS_FB1_READY       (1 << 1)
#define FPGA_STATUS_HDMI_LOCK       (1 << 2)
#define FPGA_STATUS_HDMI_ACTIVE     (1 << 3)
#define FPGA_STATUS_SPI_ACTIVE      (1 << 4)
#define FPGA_STATUS_SD_PLAYBACK     (1 << 5)
#define FPGA_STATUS_HUB75E_RUNNING  (1 << 6)
#define FPGA_STATUS_DDR_CAL_DONE    (1 << 7)
#define FPGA_STATUS_DDR_A_ERROR     (1 << 8)
#define FPGA_STATUS_DDR_B_ERROR     (1 << 9)
#define FPGA_STATUS_THERMAL_WARN    (1 << 10)
#define FPGA_STATUS_THERMAL_CRIT    (1 << 11)
#define FPGA_STATUS_INPUT_OVERFLOW  (1 << 12)
#define FPGA_STATUS_OUTPUT_UNDERRUN (1 << 13)

/* =========================================================================
 * FPGA Control Register Bit Definitions
 * ========================================================================= */

#define FPGA_CTRL_ENABLE_OUTPUT     (1 << 0)
#define FPGA_CTRL_FREEZE_FRAME      (1 << 1)
#define FPGA_CTRL_TEST_PATTERN      (1 << 2)
#define FPGA_CTRL_SOFT_RESET        (1 << 7)
#define FPGA_CTRL_GAMMA_BYPASS      (1 << 8)
#define FPGA_CTRL_DITHER_BYPASS     (1 << 9)
#define FPGA_CTRL_CSC_BYPASS        (1 << 10)
#define FPGA_CTRL_SCALER_BYPASS     (1 << 11)

/* =========================================================================
 * Input Source Select Values
 * ========================================================================= */

#define INPUT_SRC_NONE              0x00
#define INPUT_SRC_ETHERNET          0x01
#define INPUT_SRC_HDMI              0x02
#define INPUT_SRC_SD_CARD           0x03
#define INPUT_SRC_TEST_PATTERN      0x04
#define INPUT_SRC_SPI_DIRECT        0x05

/* =========================================================================
 * Dither Mode Values
 * ========================================================================= */

#define DITHER_MODE_NONE            0x00
#define DITHER_MODE_FLOYD_STEINBERG 0x01
#define DITHER_MODE_ATKINSON        0x02
#define DITHER_MODE_BLUE_NOISE      0x03
#define DITHER_MODE_TEMPORAL        0x04
#define DITHER_MODE_HYBRID          0x05

/* =========================================================================
 * Matrix Default Configuration
 * ========================================================================= */

#define DEFAULT_MATRIX_WIDTH        256
#define DEFAULT_MATRIX_HEIGHT       256
#define DEFAULT_PANEL_SCAN          32    /* 1/32 scan */
#define DEFAULT_BRIGHTNESS          65535 /* Full brightness */
#define DEFAULT_CONTRAST            32768 /* Neutral contrast */
#define DEFAULT_REFRESH_HZ          120

/* =========================================================================
 * SPI Pixel Stream Protocol
 * ========================================================================= */

#define SPI_CMD_PIXEL               0x1
#define SPI_CMD_FRAME_END           0x2
#define SPI_CMD_FRAME_START         0x3
#define SPI_CMD_CONFIG              0x4
#define SPI_CMD_NOP                 0xF

#define SPI_PIXEL_BUF_SIZE          2048  /* 32-bit words */
#define SPI_DMA_TIMEOUT_MS          100

/* =========================================================================
 * UART Command Protocol
 * ========================================================================= */

#define UART_SYNC_BYTE              0xA5
#define UART_CMD_NOP                0x00
#define UART_CMD_READ               0x01
#define UART_CMD_WRITE              0x02
#define UART_CMD_BURST_READ         0x03
#define UART_CMD_BURST_WRITE        0x04
#define UART_CMD_GAMMA_WR           0x10
#define UART_CMD_GAMMA_RD           0x11
#define UART_CMD_FB_SWAP            0x20
#define UART_CMD_FB_SELECT          0x21
#define UART_CMD_RESET              0x30
#define UART_CMD_TEST_PATTERN       0x31
#define UART_CMD_PING               0xFF

#define UART_STATUS_OK              0x00
#define UART_STATUS_INVALID_CMD     0x01
#define UART_STATUS_INVALID_ADDR    0x02
#define UART_STATUS_CRC_ERROR       0x03
#define UART_STATUS_WRITE_PROTECTED 0x04
#define UART_STATUS_BUSY            0x05

/* =========================================================================
 * Thermal Management
 * ========================================================================= */

#define THERMAL_WARN_THRESHOLD_C    80.0f
#define THERMAL_CRIT_THRESHOLD_C    85.0f
#define THERMAL_SHUTDOWN_THRESHOLD_C 90.0f

#define FAN_OFF_TEMP_C              40.0f
#define FAN_MIN_TEMP_C              50.0f
#define FAN_MAX_TEMP_C              70.0f

#define FAN_PWM_FREQUENCY_HZ        25000   /* 25 kHz, above audible range */
#define FAN_PWM_PERIOD              (APB2_FREQUENCY_HZ / FAN_PWM_FREQUENCY_HZ)

/* =========================================================================
 * Network Configuration Defaults
 * ========================================================================= */

#define DEFAULT_IP_ADDR             "192.168.1.100"
#define DEFAULT_NETMASK             "255.255.255.0"
#define DEFAULT_GATEWAY             "192.168.1.1"
#define DEFAULT_ARTNET_PORT         6454
#define DEFAULT_SACN_PORT           5568

/* =========================================================================
 * USB RNDIS Configuration
 * ========================================================================= */

#define RNDIS_VENDOR_ID             "jayis1"
#define RNDIS_PRODUCT_ID            "Nebula Matrix"
#define RNDIS_MAC_ADDR              {0x02, 0x00, 0x00, 0x00, 0x00, 0x01}

/* =========================================================================
 * Macros
 * ========================================================================= */

/* Set/Clear/Read GPIO pin */
#define GPIO_SET(port, pin)         HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET)
#define GPIO_CLR(port, pin)         HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET)
#define GPIO_TOGGLE(port, pin)      HAL_GPIO_TogglePin(port, pin)
#define GPIO_READ(port, pin)        HAL_GPIO_ReadPin(port, pin)

/* Assert/de-assert reset lines (active low) */
#define RESET_ASSERT(port, pin)     GPIO_CLR(port, pin)
#define RESET_DEASSERT(port, pin)   GPIO_SET(port, pin)

/* LED control */
#define LED_ON(port, pin)           GPIO_SET(port, pin)
#define LED_OFF(port, pin)          GPIO_CLR(port, pin)
#define LED_TOGGLE(port, pin)       GPIO_TOGGLE(port, pin)

#endif /* BOARD_H */
