/*
 * board.h — HexaScope Pin Definitions
 * STM32G474VET6 pin assignments for HexaScope mixed-signal oscilloscope
 */

#ifndef BOARD_H
#define BOARD_H

#include "registers.h"

/* ============================================================
 * GPIO Pin Definitions
 * ============================================================ */

/* --- Port A --- */
#define BTN_RUN_STOP_PIN    0   /* PA0  - TIM2_CH1, Run/Stop button */
#define LED_TRIG_PIN         1   /* PA1  - Trigger LED (active low) */
#define LED_USB_PIN          2   /* PA2  - USB active LED */
#define LED_PWR_PIN          3   /* PA3  - Power LED */
#define WIFI_EN_PIN          3   /* PA3  - ESP32-C6 enable (remap to PA15) */
#define SPI1_NSS_PIN         4   /* PA4  - SPI1 NSS → SPI Flash CS */
#define SPI1_SCK_PIN         5   /* PA5  - SPI1 SCK → SPI Flash Clock */
#define SPI1_MISO_PIN        6   /* PA6  - SPI1 MISO → SPI Flash Data Out */
#define SPI1_MOSI_PIN        7   /* PA7  - SPI1 MOSI → SPI Flash Data In */
#define MCU_GPIO0_PIN        15  /* PA15 - General purpose GPIO to FPGA */

/* --- Port B --- */
#define MCU_GPIO1_PIN        0   /* PB0  - General purpose GPIO to FPGA */
#define MCU_GPIO2_PIN        1   /* PB1  - General purpose GPIO to FPGA */
#define SPI2_NSS_PIN         2   /* PB2  - SPI2 NSS → DAC CS_N */
#define SPI2_SCK_PIN         3   /* PB3  - SPI2 SCK → DAC Clock */
#define SPI2_MISO_PIN        4   /* PB4  - SPI2 MISO → DAC Data Out */
#define SPI2_MOSI_PIN        5   /* PB5  - SPI2 MOSI → DAC Data In */
#define I2C1_SCL_PIN         6   /* PB6  - I2C1 SCL → PMIC */
#define I2C1_SDA_PIN         7   /* PB7  - I2C1 SDA → PMIC */
#define I2C2_SCL_PIN         8   /* PB8  - I2C2 SCL → FUSB302 */
#define I2C2_SDA_PIN         9   /* PB9  - I2C2 SDA → FUSB302 */
#define USART3_TX_PIN        10  /* PB10 - USART3 TX → ESP32 RX */
#define USART3_RX_PIN        11  /* PB11 - USART3 RX → ESP32 TX */
#define BTN_FORCE_PIN        12  /* PB12 - Force trigger button */
#define SPI4_SCK_PIN         13  /* PB13 - SPI4 SCK → FPGA register SPI */
#define SPI4_MISO_PIN        14  /* PB14 - SPI4 MISO ← FPGA register SPI */
#define SPI4_MOSI_PIN        15  /* PB15 - SPI4 MOSI → FPGA register SPI */

/* --- Port C --- */
#define BTN_RUN_STOP_ALT_PIN 13  /* PC13 - Run/Stop button (alternate) */
#define BTN_SINGLE_PIN       14  /* PC14 - Single capture button */
#define BTN_AUTO_PIN         15  /* PC15 - Auto setup button */
#define ANA_SENSE_PIN        0   /* PC0  - ADC1_IN1, analog supply monitor */
#define TEMP_SENSE_PIN       1   /* PC1  - ADC1_IN2, board temperature */
#define VBUS_SENSE_PIN       2   /* PC2  - ADC1_IN3, USB VBUS voltage */
#define LED_RUN_PIN          3   /* PC3  - Run LED (active low) */
#define SPI4_NSS_PIN         6   /* PC6  - SPI4 NSS → FPGA register CS_N */
#define MCU_IRQ_PIN          7   /* PC7  - FPGA interrupt to MCU */
#define SD_SW_PIN            8   /* PC8  - SD card detect */
#define SD_CS_N_PIN          9   /* PC9  - SD card CS_N */
#define SD_CMD_PIN           10  /* PA8 remap - SD command (SPI) */

/* --- Port D --- */
#define JTAG_TCK_PIN         0   /* PD0  - FPGA JTAG clock */
#define JTAG_TMS_PIN         1   /* PD1  - FPGA JTAG mode */
#define JTAG_TDI_PIN         2   /* PD2  - FPGA JTAG data in */
#define JTAG_TDO_PIN         3   /* PD3  - FPGA JTAG data out */
#define FPGA_PROG_N_PIN     4   /* PD4  - FPGA program_N */
#define FPGA_INIT_N_PIN     5   /* PD5  - FPGA init_N */
#define FPGA_DONE_PIN        6   /* PD6  - FPGA done */
#define PD_INT_N_PIN         7   /* PD7  - FUSB302 interrupt */
#define PMIC_INT_N_PIN       8   /* PD8  - PMIC interrupt */

/* --- LED Active States --- */
#define LED_ON               0   /* Active low: LED on = pin low */
#define LED_OFF              1   /* Active low: LED off = pin high */

/* ============================================================
 * GPIO Port Base Addresses
 * ============================================================ */
#define GPIOA                 ((GPIO_TypeDef *)GPIOA_BASE)
#define GPIOB                 ((GPIO_TypeDef *)GPIOB_BASE)
#define GPIOC                 ((GPIO_TypeDef *)GPIOC_BASE)
#define GPIOD                 ((GPIO_TypeDef *)GPIOD_BASE)

typedef struct {
    volatile uint32_t MODER;    /* 0x00 */
    volatile uint32_t OTYPER;   /* 0x04 */
    volatile uint32_t OSPEEDR;  /* 0x08 */
    volatile uint32_t PUPDR;    /* 0x0C */
    volatile uint32_t IDR;      /* 0x10 */
    volatile uint32_t ODR;      /* 0x14 */
    volatile uint32_t BSRR;     /* 0x18 */
    volatile uint32_t LCKR;     /* 0x1C */
    volatile uint32_t AFRL;     /* 0x20 */
    volatile uint32_t AFRH;     /* 0x24 */
    volatile uint32_t BRR;      /* 0x28 */
} GPIO_TypeDef;

/* ============================================================
 * Clock Configuration
 * ============================================================ */
#define SYSCLK_FREQ           170000000U  /* 170 MHz */
#define AHB_FREQ              170000000U  /* 170 MHz */
#define APB1_FREQ             170000000U  /* 170 MHz */
#define APB2_FREQ             170000000U  /* 170 MHz */
#define HSE_VALUE             25000000U   /* 25 MHz crystal */

/* ============================================================
 * SPI Configuration
 * ============================================================ */
#define SPI_FLASH_SPEED       50000000U   /* 50 MHz */
#define SPI_DAC_SPEED         20000000U   /* 20 MHz */
#define SPI_FPGA_SPEED        40000000U   /* 40 MHz */

/* ============================================================
 * I2C Addresses
 * ============================================================ */
#define PMIC_I2C_ADDR         0x58U       /* DA9063 I2C address (7-bit) */
#define PD_I2C_ADDR           0x22U       /* FUSB302 I2C address (7-bit) */

/* ============================================================
 * Utility Macros
 * ============================================================ */
#define BIT(n)                 (1U << (n))
#define SET_BIT(reg, bit)      ((reg) |= BIT(bit))
#define CLR_BIT(reg, bit)      ((reg) &= ~BIT(bit))
#define READ_BIT(reg, bit)     (((reg) & BIT(bit)) ? 1 : 0)
#define TOGGLE_BIT(reg, bit)   ((reg) ^= BIT(bit))

#define GPIO_MODE_INPUT         0x0U
#define GPIO_MODE_OUTPUT        0x1U
#define GPIO_MODE_AF            0x2U
#define GPIO_MODE_ANALOG        0x3U

#define GPIO_SPEED_LOW          0x0U
#define GPIO_SPEED_MEDIUM       0x1U
#define GPIO_SPEED_HIGH         0x2U
#define GPIO_SPEED_VERY_HIGH    0x3U

#define GPIO_PULL_NONE          0x0U
#define GPIO_PULL_UP            0x1U
#define GPIO_PULL_DOWN          0x2U

#define AF0                     0x0U
#define AF4                     0x4U    /* I2C */
#define AF5                     0x5U    /* SPI */
#define AF6                     0x6U    /* SPI4 */
#define AF7                     0x7U    /* USART */
#define AF10                    0xAU    /* USB */

#endif /* BOARD_H */