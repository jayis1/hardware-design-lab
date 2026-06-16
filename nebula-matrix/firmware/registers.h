/**
 * registers.h — Nebula Matrix Complete MMIO Register Map
 *
 * This file defines all Memory-Mapped I/O (MMIO) register addresses and
 * bit-field definitions for the STM32H743VIT6 microcontroller as used
 * in the Nebula Matrix design.
 *
 * Includes:
 *   - RCC (Reset & Clock Control) register definitions
 *   - GPIO register base addresses and offsets
 *   - SPI1/SPI2 register definitions
 *   - USART1 register definitions
 *   - I2C2/I2C3/I2C4 register definitions
 *   - SDMMC1 register definitions
 *   - ETH (Ethernet MAC) register definitions
 *   - USB OTG register definitions
 *   - TIM1 register definitions
 *   - ADC1 register definitions
 *   - DMA register definitions
 *   - NVIC/EXTI register definitions
 *   - FPGA register map (accessed via UART)
 *   - Peripheral clock enable macros
 */

#ifndef REGISTERS_H
#define REGISTERS_H

#include <stdint.h>

/* =========================================================================
 * ARM Cortex-M7 Core Peripherals
 * ========================================================================= */

#define NVIC_BASE               0xE000E100UL
#define SCB_BASE                0xE000ED00UL
#define SYSTICK_BASE            0xE000E010UL
#define MPU_BASE                0xE000ED90UL
#define FPU_BASE                0xE000EF30UL

/* NVIC Registers */
#define NVIC_ISER0              (*(volatile uint32_t *)(NVIC_BASE + 0x000))
#define NVIC_ISER1              (*(volatile uint32_t *)(NVIC_BASE + 0x004))
#define NVIC_ISER2              (*(volatile uint32_t *)(NVIC_BASE + 0x008))
#define NVIC_ICER0              (*(volatile uint32_t *)(NVIC_BASE + 0x080))
#define NVIC_ICER1              (*(volatile uint32_t *)(NVIC_BASE + 0x084))
#define NVIC_ICER2              (*(volatile uint32_t *)(NVIC_BASE + 0x088))
#define NVIC_ISPR0              (*(volatile uint32_t *)(NVIC_BASE + 0x100))
#define NVIC_ICPR0              (*(volatile uint32_t *)(NVIC_BASE + 0x180))
#define NVIC_IABR0              (*(volatile uint32_t *)(NVIC_BASE + 0x200))
#define NVIC_IPR0               (*(volatile uint32_t *)(NVIC_BASE + 0x300))

/* SCB Registers */
#define SCB_CPUID               (*(volatile uint32_t *)(SCB_BASE + 0x00))
#define SCB_ICSR                (*(volatile uint32_t *)(SCB_BASE + 0x04))
#define SCB_VTOR                (*(volatile uint32_t *)(SCB_BASE + 0x08))
#define SCB_AIRCR               (*(volatile uint32_t *)(SCB_BASE + 0x0C))
#define SCB_SCR                 (*(volatile uint32_t *)(SCB_BASE + 0x10))
#define SCB_CCR                 (*(volatile uint32_t *)(SCB_BASE + 0x14))
#define SCB_SHPR1               (*(volatile uint32_t *)(SCB_BASE + 0x18))
#define SCB_SHPR2               (*(volatile uint32_t *)(SCB_BASE + 0x1C))
#define SCB_SHPR3               (*(volatile uint32_t *)(SCB_BASE + 0x20))
#define SCB_SHCSR               (*(volatile uint32_t *)(SCB_BASE + 0x24))
#define SCB_CFSR                (*(volatile uint32_t *)(SCB_BASE + 0x28))
#define SCB_HFSR                (*(volatile uint32_t *)(SCB_BASE + 0x2C))
#define SCB_MMFAR               (*(volatile uint32_t *)(SCB_BASE + 0x34))
#define SCB_BFAR                (*(volatile uint32_t *)(SCB_BASE + 0x38))

/* =========================================================================
 * STM32H7 Peripheral Base Addresses
 * ========================================================================= */

/* AHB1 Peripherals (D1 Domain, 240 MHz) */
#define DMA1_BASE               0x40020000UL
#define DMA2_BASE               0x40020400UL
#define DMAMUX1_BASE            0x40020800UL
#define ADC12_COMMON_BASE       0x40022300UL
#define ADC1_BASE               0x40022000UL
#define ADC2_BASE               0x40022100UL
#define ETH_BASE                0x40028000UL
#define USB1_OTG_HS_BASE        0x40040000UL
#define USB2_OTG_FS_BASE        0x50000000UL

/* AHB2 Peripherals (D2 Domain, 240 MHz) */
#define GPIOA_BASE              0x58020000UL
#define GPIOB_BASE              0x58020400UL
#define GPIOC_BASE              0x58020800UL
#define GPIOD_BASE              0x58020C00UL
#define GPIOE_BASE              0x58021000UL
#define GPIOF_BASE              0x58021400UL
#define GPIOG_BASE              0x58021800UL
#define GPIOH_BASE              0x58021C00UL
#define GPIOI_BASE              0x58022000UL
#define GPIOJ_BASE              0x58022400UL
#define GPIOK_BASE              0x58022800UL

/* AHB3 Peripherals (D1 Domain) */
#define FMC_BASE                0x52004000UL
#define QSPI_BASE               0x52005000UL
#define SDMMC1_BASE             0x52007000UL
#define SDMMC2_BASE             0x52007400UL

/* AHB4 Peripherals (D3 Domain, 240 MHz) */
#define RCC_BASE                0x58024400UL
#define PWR_BASE                0x58024800UL
#define EXTI_BASE               0x58022C00UL
#define SYSCFG_BASE             0x58000400UL

/* APB1 Peripherals (D2 Domain, 120 MHz) */
#define TIM2_BASE               0x40000000UL
#define TIM3_BASE               0x40000400UL
#define TIM4_BASE               0x40000800UL
#define TIM5_BASE               0x40000C00UL
#define TIM6_BASE               0x40001000UL
#define TIM7_BASE               0x40001400UL
#define TIM12_BASE              0x40001800UL
#define TIM13_BASE              0x40001C00UL
#define TIM14_BASE              0x40002000UL
#define SPI2_BASE               0x40003800UL
#define SPI3_BASE               0x40003C00UL
#define USART2_BASE             0x40004400UL
#define USART3_BASE             0x40004800UL
#define UART4_BASE              0x40004C00UL
#define UART5_BASE              0x40005000UL
#define I2C1_BASE               0x40005400UL
#define I2C2_BASE               0x40005800UL
#define I2C3_BASE               0x40005C00UL

/* APB2 Peripherals (D2 Domain, 120 MHz) */
#define TIM1_BASE               0x40010000UL
#define TIM8_BASE               0x40010400UL
#define USART1_BASE             0x40011000UL
#define USART6_BASE             0x40011400UL
#define UART7_BASE              0x40011800UL
#define UART8_BASE              0x40011C00UL
#define SPI1_BASE               0x40013000UL
#define SPI4_BASE               0x40013400UL
#define SPI5_BASE               0x40013800UL

/* APB4 Peripherals (D3 Domain, 120 MHz) */
#define I2C4_BASE               0x58001C00UL
#define LPUART1_BASE            0x58002400UL
#define SPI6_BASE               0x58001400UL

/* =========================================================================
 * RCC Register Offsets (from RCC_BASE)
 * ========================================================================= */

#define RCC_CR                  0x00   /* Clock control register */
#define RCC_CFGR                0x10   /* Clock configuration register */
#define RCC_D1CFGR              0x18   /* D1 domain clock config */
#define RCC_D2CFGR              0x1C   /* D2 domain clock config */
#define RCC_D3CFGR              0x20   /* D3 domain clock config */
#define RCC_PLLCKSELR           0x28   /* PLL clock source selection */
#define RCC_PLLCFGR             0x2C   /* PLL configuration */
#define RCC_PLL1DIVR            0x30   /* PLL1 dividers */
#define RCC_PLL1FRACR           0x34   /* PLL1 fractional divider */
#define RCC_PLL2DIVR            0x38   /* PLL2 dividers */
#define RCC_PLL2FRACR           0x3C   /* PLL2 fractional divider */
#define RCC_PLL3DIVR            0x40   /* PLL3 dividers */
#define RCC_PLL3FRACR           0x44   /* PLL3 fractional divider */
#define RCC_D1CCIPR             0x4C   /* D1 domain peripheral clock selection */
#define RCC_D2CCIP1R            0x50   /* D2 domain peripheral clock selection 1 */
#define RCC_D2CCIP2R            0x54   /* D2 domain peripheral clock selection 2 */
#define RCC_D3CCIPR             0x58   /* D3 domain peripheral clock selection */
#define RCC_CIER                0x60   /* Clock interrupt enable */
#define RCC_CIFR                0x64   /* Clock interrupt flag */
#define RCC_AHB1RSTR            0x80   /* AHB1 peripheral reset */
#define RCC_AHB2RSTR            0x84   /* AHB2 peripheral reset */
#define RCC_AHB3RSTR            0x88   /* AHB3 peripheral reset */
#define RCC_AHB4RSTR            0x8C   /* AHB4 peripheral reset */
#define RCC_APB1LRSTR           0x90   /* APB1 low peripheral reset */
#define RCC_APB1HRSTR           0x94   /* APB1 high peripheral reset */
#define RCC_APB2RSTR            0x98   /* APB2 peripheral reset */
#define RCC_APB3RSTR            0x9C   /* APB3 peripheral reset */
#define RCC_APB4RSTR            0xA0   /* APB4 peripheral reset */
#define RCC_AHB1ENR             0xD0   /* AHB1 peripheral clock enable */
#define RCC_AHB2ENR             0xD4   /* AHB2 peripheral clock enable */
#define RCC_AHB3ENR             0xD8   /* AHB3 peripheral clock enable */
#define RCC_AHB4ENR             0xDC   /* AHB4 peripheral clock enable */
#define RCC_APB1LENR            0xE0   /* APB1 low peripheral clock enable */
#define RCC_APB1HENR            0xE4   /* APB1 high peripheral clock enable */
#define RCC_APB2ENR             0xE8   /* APB2 peripheral clock enable */
#define RCC_APB3ENR             0xEC   /* APB3 peripheral clock enable */
#define RCC_APB4ENR             0xF0   /* APB4 peripheral clock enable */

/* RCC_CR bit definitions */
#define RCC_CR_HSION            (1 << 0)
#define RCC_CR_HSIRDY           (1 << 2)
#define RCC_CR_HSEON            (1 << 8)
#define RCC_CR_HSERDY           (1 << 10)
#define RCC_CR_HSEBYP           (1 << 12)
#define RCC_CR_PLL1ON           (1 << 16)
#define RCC_CR_PLL1RDY          (1 << 18)
#define RCC_CR_PLL2ON           (1 << 20)
#define RCC_CR_PLL2RDY          (1 << 22)
#define RCC_CR_PLL3ON           (1 << 24)
#define RCC_CR_PLL3RDY          (1 << 26)

/* RCC_AHB2ENR bit definitions (GPIO clocks) */
#define RCC_AHB2ENR_GPIOAEN     (1 << 0)
#define RCC_AHB2ENR_GPIOBEN     (1 << 1)
#define RCC_AHB2ENR_GPIOCEN     (1 << 2)
#define RCC_AHB2ENR_GPIODEN     (1 << 3)
#define RCC_AHB2ENR_GPIOEEN     (1 << 4)
#define RCC_AHB2ENR_GPIOFEN     (1 << 5)
#define RCC_AHB2ENR_GPIOGEN     (1 << 6)
#define RCC_AHB2ENR_GPIOHEN     (1 << 7)

/* RCC_APB1LENR bit definitions */
#define RCC_APB1LENR_TIM2EN     (1 << 0)
#define RCC_APB1LENR_TIM3EN     (1 << 1)
#define RCC_APB1LENR_TIM4EN     (1 << 2)
#define RCC_APB1LENR_TIM5EN     (1 << 3)
#define RCC_APB1LENR_TIM6EN     (1 << 4)
#define RCC_APB1LENR_TIM7EN     (1 << 5)
#define RCC_APB1LENR_TIM12EN    (1 << 6)
#define RCC_APB1LENR_TIM13EN    (1 << 7)
#define RCC_APB1LENR_TIM14EN    (1 << 8)
#define RCC_APB1LENR_SPI2EN     (1 << 14)
#define RCC_APB1LENR_SPI3EN     (1 << 15)
#define RCC_APB1LENR_USART2EN   (1 << 17)
#define RCC_APB1LENR_USART3EN   (1 << 18)
#define RCC_APB1LENR_UART4EN    (1 << 19)
#define RCC_APB1LENR_UART5EN    (1 << 20)
#define RCC_APB1LENR_I2C1EN     (1 << 21)
#define RCC_APB1LENR_I2C2EN     (1 << 22)
#define RCC_APB1LENR_I2C3EN     (1 << 23)

/* RCC_APB2ENR bit definitions */
#define RCC_APB2ENR_TIM1EN      (1 << 0)
#define RCC_APB2ENR_TIM8EN      (1 << 1)
#define RCC_APB2ENR_USART1EN    (1 << 4)
#define RCC_APB2ENR_USART6EN    (1 << 5)
#define RCC_APB2ENR_SPI1EN      (1 << 12)
#define RCC_APB2ENR_SPI4EN      (1 << 13)

/* RCC_APB4ENR bit definitions */
#define RCC_APB4ENR_I2C4EN      (1 << 7)
#define RCC_APB4ENR_SPI6EN      (1 << 5)

/* RCC_AHB1ENR bit definitions */
#define RCC_AHB1ENR_DMA1EN      (1 << 0)
#define RCC_AHB1ENR_DMA2EN      (1 << 1)
#define RCC_AHB1ENR_ADC12EN     (1 << 5)
#define RCC_AHB1ENR_ETH1MACEN   (1 << 15)
#define RCC_AHB1ENR_ETH1TXEN    (1 << 16)
#define RCC_AHB1ENR_ETH1RXEN    (1 << 17)
#define RCC_AHB1ENR_USB1OTGHSEN (1 << 25)
#define RCC_AHB1ENR_USB2OTGFSEN (1 << 27)

/* RCC_AHB3ENR bit definitions */
#define RCC_AHB3ENR_SDMMC1EN    (1 << 16)

/* RCC_AHB4ENR bit definitions */
#define RCC_AHB4ENR_SYSCFGEN    (1 << 1)

/* =========================================================================
 * GPIO Register Offsets (from GPIOx_BASE)
 * ========================================================================= */

#define GPIO_MODER_OFFSET       0x00   /* Mode register */
#define GPIO_OTYPER_OFFSET      0x04   /* Output type register */
#define GPIO_OSPEEDR_OFFSET     0x08   /* Output speed register */
#define GPIO_PUPDR_OFFSET       0x0C   /* Pull-up/pull-down register */
#define GPIO_IDR_OFFSET         0x10   /* Input data register */
#define GPIO_ODR_OFFSET         0x14   /* Output data register */
#define GPIO_BSRR_OFFSET        0x18   /* Bit set/reset register */
#define GPIO_LCKR_OFFSET        0x1C   /* Lock register */
#define GPIO_AFRL_OFFSET        0x20   /* Alternate function low (pins 0-7) */
#define GPIO_AFRH_OFFSET        0x24   /* Alternate function high (pins 8-15) */

/* GPIO_MODER bit definitions (2 bits per pin) */
#define GPIO_MODE_INPUT         0x0
#define GPIO_MODE_OUTPUT        0x1
#define GPIO_MODE_AF            0x2
#define GPIO_MODE_ANALOG        0x3

/* GPIO_OTYPER bit definitions */
#define GPIO_OTYPE_PP           0x0   /* Push-pull */
#define GPIO_OTYPE_OD           0x1   /* Open-drain */

/* GPIO_OSPEEDR bit definitions (2 bits per pin) */
#define GPIO_SPEED_LOW          0x0
#define GPIO_SPEED_MEDIUM       0x1
#define GPIO_SPEED_HIGH         0x2
#define GPIO_SPEED_VERY_HIGH    0x3

/* GPIO_PUPDR bit definitions (2 bits per pin) */
#define GPIO_PUPD_NONE          0x0
#define GPIO_PUPD_PULLUP        0x1
#define GPIO_PUPD_PULLDOWN      0x2

/* =========================================================================
 * SPI Register Offsets (from SPIx_BASE)
 * ========================================================================= */

#define SPI_CR1_OFFSET          0x00   /* Control register 1 */
#define SPI_CR2_OFFSET          0x04   /* Control register 2 */
#define SPI_SR_OFFSET           0x08   /* Status register */
#define SPI_DR_OFFSET           0x0C   /* Data register */
#define SPI_CRCPR_OFFSET        0x10   /* CRC polynomial register */
#define SPI_RXCRCR_OFFSET       0x14   /* RX CRC register */
#define SPI_TXCRCR_OFFSET       0x18   /* TX CRC register */
#define SPI_CFG1_OFFSET         0x00   /* Configuration register 1 (H7 specific) */
#define SPI_CFG2_OFFSET         0x04   /* Configuration register 2 (H7 specific) */

/* SPI_CFG1 bit definitions (STM32H7) */
#define SPI_CFG1_DSIZE_32BIT    (31 << 0)
#define SPI_CFG1_DSIZE_16BIT    (15 << 0)
#define SPI_CFG1_DSIZE_8BIT     (7 << 0)
#define SPI_CFG1_CPHA           (1 << 1)
#define SPI_CFG1_CPOL           (1 << 2)
#define SPI_CFG1_MASTER         (1 << 3)
#define SPI_CFG1_SSM            (1 << 9)   /* Software slave management */
#define SPI_CFG1_MBR_DIV2       (0 << 12)  /* Master baud rate = fPCLK/2 */
#define SPI_CFG1_MBR_DIV4       (1 << 12)
#define SPI_CFG1_MBR_DIV8       (2 << 12)
#define SPI_CFG1_MBR_DIV16      (3 << 12)
#define SPI_CFG1_MBR_DIV32      (4 << 12)
#define SPI_CFG1_MBR_DIV64      (5 << 12)
#define SPI_CFG1_MBR_DIV128     (6 << 12)
#define SPI_CFG1_MBR_DIV256     (7 << 12)

/* SPI_CFG2 bit definitions */
#define SPI_CFG2_SSOE           (1 << 0)   /* SS output enable */
#define SPI_CFG2_AFCNTR         (1 << 4)   /* Alternate function control */
#define SPI_CFG2_MASTER_SS      (1 << 22)  /* Master SS polarity */
#define SPI_CFG2_COMM           (1 << 17)  /* Communication mode (0=full duplex) */

/* SPI_SR bit definitions */
#define SPI_SR_RXP              (1 << 0)   /* RX data available */
#define SPI_SR_TXP              (1 << 1)   /* TX buffer empty */
#define SPI_SR_EOT              (1 << 3)   /* End of transfer */
#define SPI_SR_TXTF             (1 << 4)   /* TX FIFO threshold */
#define SPI_SR_OVR              (1 << 6)   /* Overrun error */
#define SPI_SR_MODF             (1 << 7)   /* Mode fault */
#define SPI_SR_TIFRE            (1 << 12)  /* TI frame format error */

/* =========================================================================
 * USART Register Offsets (from USARTx_BASE)
 * ========================================================================= */

#define USART_CR1_OFFSET        0x00
#define USART_CR2_OFFSET        0x04
#define USART_CR3_OFFSET        0x08
#define USART_BRR_OFFSET        0x0C
#define USART_GTPR_OFFSET       0x10
#define USART_RTOR_OFFSET       0x14
#define USART_RQR_OFFSET        0x18
#define USART_ISR_OFFSET        0x1C
#define USART_ICR_OFFSET        0x20
#define USART_RDR_OFFSET        0x24
#define USART_TDR_OFFSET        0x28
#define USART_PRESC_OFFSET      0x2C   /* H7 specific: prescaler */

/* USART_CR1 bit definitions */
#define USART_CR1_UE            (1 << 0)   /* USART enable */
#define USART_CR1_TE            (1 << 3)   /* Transmitter enable */
#define USART_CR1_RE            (1 << 2)   /* Receiver enable */
#define USART_CR1_RXNEIE        (1 << 5)   /* RX not empty interrupt */
#define USART_CR1_TXEIE         (1 << 7)   /* TX empty interrupt */
#define USART_CR1_M0            (1 << 12)  /* Word length (with M1) */
#define USART_CR1_M1            (1 << 28)  /* Word length: 00=8bit, 01=9bit, 10=7bit */
#define USART_CR1_OVER8         (1 << 15)  /* Oversampling 8 (vs 16) */

/* USART_ISR bit definitions */
#define USART_ISR_TXE           (1 << 7)   /* TX data register empty */
#define USART_ISR_TC            (1 << 6)   /* Transmission complete */
#define USART_ISR_RXNE          (1 << 5)   /* RX data register not empty */
#define USART_ISR_ORE           (1 << 3)   /* Overrun error */
#define USART_ISR_FE            (1 << 1)   /* Framing error */
#define USART_ISR_PE            (1 << 0)   /* Parity error */

/* =========================================================================
 * I2C Register Offsets (from I2Cx_BASE)
 * ========================================================================= */

#define I2C_CR1_OFFSET          0x00
#define I2C_CR2_OFFSET          0x04
#define I2C_OAR1_OFFSET         0x08
#define I2C_OAR2_OFFSET         0x0C
#define I2C_TIMINGR_OFFSET      0x10
#define I2C_TIMEOUTR_OFFSET     0x14
#define I2C_ISR_OFFSET          0x18
#define I2C_ICR_OFFSET          0x1C
#define I2C_PECR_OFFSET         0x20
#define I2C_RXDR_OFFSET         0x24
#define I2C_TXDR_OFFSET         0x28

/* I2C_CR1 bit definitions */
#define I2C_CR1_PE              (1 << 0)   /* Peripheral enable */
#define I2C_CR1_TXIE            (1 << 1)   /* TX interrupt enable */
#define I2C_CR1_RXIE            (1 << 2)   /* RX interrupt enable */
#define I2C_CR1_NACKIE          (1 << 4)   /* NACK interrupt enable */
#define I2C_CR1_STOPIE          (1 << 5)   /* STOP detection interrupt */
#define I2C_CR1_TCIE            (1 << 6)   /* Transfer complete interrupt */
#define I2C_CR1_ERRIE           (1 << 7)   /* Error interrupt enable */

/* I2C_ISR bit definitions */
#define I2C_ISR_TXE             (1 << 0)   /* TX data register empty */
#define I2C_ISR_TXIS            (1 << 1)   /* TX interrupt status */
#define I2C_ISR_RXNE            (1 << 2)   /* RX data register not empty */
#define I2C_ISR_ADDR            (1 << 3)   /* Address matched */
#define I2C_ISR_NACKF           (1 << 4)   /* NACK received */
#define I2C_ISR_STOPF           (1 << 5)   /* STOP detected */
#define I2C_ISR_TC              (1 << 6)   /* Transfer complete */
#define I2C_ISR_TCR             (1 << 7)   /* Transfer complete reload */
#define I2C_ISR_BERR            (1 << 8)   /* Bus error */
#define I2C_ISR_ARLO            (1 << 9)   /* Arbitration lost */
#define I2C_ISR_OVR             (1 << 10)  /* Overrun/underrun */
#define I2C_ISR_PECERR          (1 << 11)  /* PEC error */
#define I2C_ISR_TIMEOUT         (1 << 12)  /* Timeout */
#define I2C_ISR_ALERT           (1 << 13)  /* SMBus alert */
#define I2C_ISR_BUSY            (1 << 15)  /* Bus busy */

/* =========================================================================
 * SDMMC1 Register Offsets (from SDMMC1_BASE)
 * ========================================================================= */

#define SDMMC_POWER_OFFSET      0x00
#define SDMMC_CLKCR_OFFSET      0x04
#define SDMMC_ARG_OFFSET        0x08
#define SDMMC_CMD_OFFSET        0x0C
#define SDMMC_RESPCMD_OFFSET    0x10
#define SDMMC_RESP1_OFFSET      0x14
#define SDMMC_RESP2_OFFSET      0x18
#define SDMMC_RESP3_OFFSET      0x1C
#define SDMMC_RESP4_OFFSET      0x20
#define SDMMC_DTIMER_OFFSET     0x24
#define SDMMC_DLEN_OFFSET       0x28
#define SDMMC_DCTRL_OFFSET      0x2C
#define SDMMC_DCOUNT_OFFSET     0x30
#define SDMMC_STA_OFFSET        0x34
#define SDMMC_ICR_OFFSET        0x38
#define SDMMC_MASK_OFFSET       0x3C
#define SDMMC_FIFOCNT_OFFSET    0x48
#define SDMMC_FIFO_OFFSET       0x80

/* SDMMC_CLKCR bit definitions */
#define SDMMC_CLKCR_CLKDIV_MASK 0xFF
#define SDMMC_CLKCR_CLKEN       (1 << 8)
#define SDMMC_CLKCR_PWRSAV      (1 << 9)
#define SDMMC_CLKCR_BYPASS      (1 << 10)
#define SDMMC_CLKCR_WIDBUS_1    (0 << 11)
#define SDMMC_CLKCR_WIDBUS_4    (1 << 11)
#define SDMMC_CLKCR_WIDBUS_8    (2 << 11)
#define SDMMC_CLKCR_NEGEDGE     (1 << 13)
#define SDMMC_CLKCR_HWFC_EN     (1 << 14)

/* =========================================================================
 * DMA Register Offsets (from DMAx_BASE + stream_offset)
 * ========================================================================= */

#define DMA_STREAM_OFFSET(n)    (0x10 * (n))  /* n = 0-7 */
#define DMA_LISR_OFFSET         0x00   /* Low interrupt status */
#define DMA_HISR_OFFSET         0x04   /* High interrupt status */
#define DMA_LIFCR_OFFSET        0x08   /* Low interrupt flag clear */
#define DMA_HIFCR_OFFSET        0x0C   /* High interrupt flag clear */

/* Per-stream registers (add DMA_STREAM_OFFSET(n)) */
#define DMA_SxCR_OFFSET         0x00   /* Stream configuration */
#define DMA_SxNDTR_OFFSET       0x04   /* Number of data items to transfer */
#define DMA_SxPAR_OFFSET        0x08   /* Peripheral address */
#define DMA_SxM0AR_OFFSET       0x0C   /* Memory 0 address */
#define DMA_SxM1AR_OFFSET       0x10   /* Memory 1 address */
#define DMA_SxFCR_OFFSET        0x14   /* FIFO control */

/* DMA_SxCR bit definitions */
#define DMA_SxCR_EN             (1 << 0)   /* Stream enable */
#define DMA_SxCR_DMEIE          (1 << 1)   /* Direct mode error interrupt */
#define DMA_SxCR_TEIE           (1 << 2)   /* Transfer error interrupt */
#define DMA_SxCR_HTIE           (1 << 3)   /* Half transfer interrupt */
#define DMA_SxCR_TCIE           (1 << 4)   /* Transfer complete interrupt */
#define DMA_SxCR_DIR_P2M        (0 << 6)   /* Peripheral-to-memory */
#define DMA_SxCR_DIR_M2P        (1 << 6)   /* Memory-to-peripheral */
#define DMA_SxCR_DIR_M2M        (2 << 6)   /* Memory-to-memory */
#define DMA_SxCR_CIRC           (1 << 8)   /* Circular mode */
#define DMA_SxCR_PINC           (1 << 9)   /* Peripheral increment */
#define DMA_SxCR_MINC           (1 << 10)  /* Memory increment */
#define DMA_SxCR_PSIZE_BYTE     (0 << 11)  /* Peripheral data size: byte */
#define DMA_SxCR_PSIZE_HWORD    (1 << 11)  /* Half-word */
#define DMA_SxCR_PSIZE_WORD     (2 << 11)  /* Word */
#define DMA_SxCR_MSIZE_BYTE     (0 << 13)  /* Memory data size: byte */
#define DMA_SxCR_MSIZE_HWORD    (1 << 13)  /* Half-word */
#define DMA_SxCR_MSIZE_WORD     (2 << 13)  /* Word */
#define DMA_SxCR_PL_LOW         (0 << 16)  /* Priority: low */
#define DMA_SxCR_PL_MEDIUM      (1 << 16)  /* Medium */
#define DMA_SxCR_PL_HIGH        (2 << 16)  /* High */
#define DMA_SxCR_PL_VERY_HIGH   (3 << 16)  /* Very high */
#define DMA_SxCR_DBM            (1 << 18)  /* Double buffer mode */
#define DMA_SxCR_CT             (1 << 19)  /* Current target (in DBM) */
#define DMA_SxCR_PFCTRL         (1 << 21)  /* Peripheral flow controller */

/* DMA_SxFCR bit definitions */
#define DMA_SxFCR_FTH_QUARTER   (0 << 0)   /* FIFO threshold: 1/4 */
#define DMA_SxFCR_FTH_HALF      (1 << 0)   /* 1/2 */
#define DMA_SxFCR_FTH_3QUARTER  (2 << 0)   /* 3/4 */
#define DMA_SxFCR_FTH_FULL      (3 << 0)   /* Full */
#define DMA_SxFCR_DMDIS         (1 << 2)   /* Direct mode disable */
#define DMA_SxFCR_FS_MASK       (7 << 3)   /* FIFO status */

/* =========================================================================
 * EXTI Register Offsets (from EXTI_BASE)
 * ========================================================================= */

#define EXTI_RTSR1_OFFSET       0x00   /* Rising trigger selection */
#define EXTI_FTSR1_OFFSET       0x04   /* Falling trigger selection */
#define EXTI_SWIER1_OFFSET      0x08   /* Software interrupt event */
#define EXTI_PR1_OFFSET         0x10   /* Pending register */
#define EXTI_IMR1_OFFSET        0x80   /* Interrupt mask */

/* =========================================================================
 * TIM1 Register Offsets (from TIM1_BASE)
 * ========================================================================= */

#define TIM_CR1_OFFSET          0x00
#define TIM_CR2_OFFSET          0x04
#define TIM_SMCR_OFFSET         0x08
#define TIM_DIER_OFFSET         0x0C
#define TIM_SR_OFFSET           0x10
#define TIM_EGR_OFFSET          0x14
#define TIM_CCMR1_OFFSET        0x18
#define TIM_CCMR2_OFFSET        0x1C
#define TIM_CCER_OFFSET         0x20
#define TIM_CNT_OFFSET          0x24
#define TIM_PSC_OFFSET          0x28
#define TIM_ARR_OFFSET          0x2C
#define TIM_RCR_OFFSET          0x30
#define TIM_CCR1_OFFSET         0x34
#define TIM_CCR2_OFFSET         0x38
#define TIM_CCR3_OFFSET         0x3C
#define TIM_CCR4_OFFSET         0x40
#define TIM_BDTR_OFFSET         0x44
#define TIM_DCR_OFFSET          0x48
#define TIM_DMAR_OFFSET         0x4C

/* TIM_CR1 bit definitions */
#define TIM_CR1_CEN             (1 << 0)   /* Counter enable */
#define TIM_CR1_UDIS            (1 << 1)   /* Update disable */
#define TIM_CR1_URS             (1 << 2)   /* Update request source */
#define TIM_CR1_OPM             (1 << 3)   /* One-pulse mode */
#define TIM_CR1_DIR             (1 << 4)   /* Direction */
#define TIM_CR1_ARPE            (1 << 7)   /* Auto-reload preload enable */

/* TIM_CCMR1 bit definitions (output compare, channel 1) */
#define TIM_CCMR1_OC1M_PWM1     (6 << 4)   /* PWM mode 1 */
#define TIM_CCMR1_OC1M_PWM2     (7 << 4)   /* PWM mode 2 */
#define TIM_CCMR1_OC1PE         (1 << 3)   /* Output compare 1 preload enable */

/* TIM_CCER bit definitions */
#define TIM_CCER_CC1E           (1 << 0)   /* Capture/Compare 1 output enable */
#define TIM_CCER_CC1P           (1 << 1)   /* CC1 output polarity */
#define TIM_CCER_CC2E           (1 << 4)   /* CC2 output enable */

/* =========================================================================
 * ADC1 Register Offsets (from ADC1_BASE)
 * ========================================================================= */

#define ADC_ISR_OFFSET          0x00
#define ADC_IER_OFFSET          0x04
#define ADC_CR_OFFSET           0x08
#define ADC_CFGR_OFFSET         0x0C
#define ADC_SMPR1_OFFSET        0x14
#define ADC_SMPR2_OFFSET        0x18
#define ADC_PCSEL_OFFSET        0x1C
#define ADC_SQR1_OFFSET         0x30
#define ADC_SQR2_OFFSET         0x34
#define ADC_SQR3_OFFSET         0x38
#define ADC_SQR4_OFFSET         0x3C
#define ADC_DR_OFFSET           0x40
#define ADC_DIFSEL_OFFSET       0xC0

/* ADC_CR bit definitions */
#define ADC_CR_ADEN             (1 << 0)   /* ADC enable */
#define ADC_CR_ADDIS            (1 << 1)   /* ADC disable */
#define ADC_CR_ADSTART          (1 << 2)   /* ADC start conversion */
#define ADC_CR_ADSTP            (1 << 4)   /* ADC stop conversion */
#define ADC_CR_BOOST            (1 << 8)   /* Boost mode */

/* ADC_CFGR bit definitions */
#define ADC_CFGR_CONT           (1 << 13)  /* Continuous mode */
#define ADC_CFGR_OVRMOD         (1 << 26)  /* Overrun mode */
#define ADC_CFGR_RES_12BIT      (0 << 2)   /* 12-bit resolution */
#define ADC_CFGR_RES_10BIT      (1 << 2)   /* 10-bit */
#define ADC_CFGR_RES_8BIT       (2 << 2)   /* 8-bit */
#define ADC_CFGR_RES_6BIT       (3 << 2)   /* 6-bit */

/* =========================================================================
 * PWR Register Offsets (from PWR_BASE)
 * ========================================================================= */

#define PWR_CR1_OFFSET          0x00
#define PWR_CR2_OFFSET          0x04
#define PWR_CR3_OFFSET          0x08
#define PWR_CPUCR_OFFSET        0x0C
#define PWR_D3CR_OFFSET         0x18
#define PWR_SR_OFFSET           0x20

/* PWR_CR1 bit definitions */
#define PWR_CR1_LDOEN           (1 << 0)   /* LDO enable */
#define PWR_CR1_VOS_SCALE1      (3 << 4)   /* Voltage scaling 1 (highest) */
#define PWR_CR1_VOS_SCALE2      (2 << 4)   /* Voltage scaling 2 */
#define PWR_CR1_VOS_SCALE3      (1 << 4)   /* Voltage scaling 3 (lowest) */

/* PWR_SR bit definitions */
#define PWR_SR_VOSRDY           (1 << 4)   /* Voltage scaling ready */

/* =========================================================================
 * SYSCFG Register Offsets (from SYSCFG_BASE)
 * ========================================================================= */

#define SYSCFG_PMCR_OFFSET      0x04   /* Peripheral mode configuration */
#define SYSCFG_EXTICR1_OFFSET   0x08   /* External interrupt config 1 */
#define SYSCFG_EXTICR2_OFFSET   0x0C
#define SYSCFG_EXTICR3_OFFSET   0x10
#define SYSCFG_EXTICR4_OFFSET   0x14

/* =========================================================================
 * FLASH Register Offsets
 * ========================================================================= */

#define FLASH_BASE              0x52002000UL
#define FLASH_ACR_OFFSET        0x00   /* Access control register */

/* FLASH_ACR bit definitions */
#define FLASH_ACR_LATENCY_0     (0 << 0)
#define FLASH_ACR_LATENCY_1     (1 << 0)
#define FLASH_ACR_LATENCY_2     (2 << 0)
#define FLASH_ACR_LATENCY_3     (3 << 0)
#define FLASH_ACR_LATENCY_4     (4 << 0)
#define FLASH_ACR_LATENCY_5     (5 << 0)
#define FLASH_ACR_LATENCY_6     (6 << 0)
#define FLASH_ACR_LATENCY_7     (7 << 0)
#define FLASH_ACR_PRFTEN        (1 << 8)   /* Prefetch enable */
#define FLASH_ACR_ARTEN         (1 << 9)   /* ART accelerator enable */

/* =========================================================================
 * FPGA Register Map (accessed via UART command interface)
 * ========================================================================= */

/* These are logical addresses in the FPGA's register space,
 * not physical MMIO addresses on the STM32. */

#define FPGA_REG_ID             0x0000   /* FPGA design ID (0x4E454255 = "NEBU") */
#define FPGA_REG_VERSION        0x0004   /* Version (major.minor) */
#define FPGA_REG_STATUS         0x0008   /* Status flags */
#define FPGA_REG_CONTROL        0x000C   /* Control register */
#define FPGA_REG_MATRIX_WIDTH   0x0010   /* Active matrix width (1-256) */
#define FPGA_REG_MATRIX_HEIGHT  0x0012   /* Active matrix height (1-256) */
#define FPGA_REG_PANEL_SCAN     0x0014   /* Panel scan type */
#define FPGA_REG_PANEL_CONFIG   0x0015   /* Panel configuration */
#define FPGA_REG_BRIGHTNESS     0x0018   /* Global brightness (0-65535) */
#define FPGA_REG_CONTRAST       0x001A   /* Global contrast (0-65535) */
#define FPGA_REG_GAMMA_R_BASE   0x0020   /* Red gamma LUT base address */
#define FPGA_REG_GAMMA_G_BASE   0x0024   /* Green gamma LUT base address */
#define FPGA_REG_GAMMA_B_BASE   0x0028   /* Blue gamma LUT base address */
#define FPGA_REG_FB0_BASE       0x0030   /* Frame Buffer 0 base */
#define FPGA_REG_FB1_BASE       0x0034   /* Frame Buffer 1 base */
#define FPGA_REG_FB_ACTIVE      0x0038   /* Active frame buffer */
#define FPGA_REG_FB_SWAP        0x003C   /* Frame buffer swap trigger */
#define FPGA_REG_INPUT_SOURCE   0x0040   /* Input source select */
#define FPGA_REG_INPUT_STATUS   0x0041   /* Input source status */
#define FPGA_REG_HDMI_HACT      0x0044   /* HDMI active horizontal */
#define FPGA_REG_HDMI_VACT      0x0046   /* HDMI active vertical */
#define FPGA_REG_HDMI_FPS       0x0054   /* HDMI frame rate × 100 */
#define FPGA_REG_SCALER_CROP_X  0x0060   /* Crop X start */
#define FPGA_REG_SCALER_CROP_Y  0x0062   /* Crop Y start */
#define FPGA_REG_SCALER_CROP_W  0x0064   /* Crop width */
#define FPGA_REG_SCALER_CROP_H  0x0066   /* Crop height */
#define FPGA_REG_DITHER_MODE    0x0070   /* Dithering algorithm */
#define FPGA_REG_DITHER_STRENGTH 0x0071  /* Dithering strength */
#define FPGA_REG_HUB75E_REFRESH 0x0080   /* Output refresh rate × 100 */
#define FPGA_REG_HUB75E_FRAMES  0x0084   /* Total frames output */
#define FPGA_REG_HUB75E_DROPPED 0x0088   /* Dropped frames */
#define FPGA_REG_SPI_PIXELS_RX  0x0090   /* Pixels received via SPI */
#define FPGA_REG_SPI_FRAMES_RX  0x0094   /* Frames received via SPI */
#define FPGA_REG_SPI_ERRORS     0x0098   /* SPI error count */
#define FPGA_REG_TEMP_FPGA      0x00A0   /* FPGA temperature (XADC) */
#define FPGA_REG_TEMP_DDR_A     0x00A2   /* DDR3 Ch A temperature */
#define FPGA_REG_TEMP_DDR_B     0x00A4   /* DDR3 Ch B temperature */
#define FPGA_REG_UART_CMD_RX    0x00B0   /* UART commands received */
#define FPGA_REG_UART_CMD_ERR   0x00B4   /* UART command errors */
#define FPGA_REG_GAMMA_R_LUT    0x0100   /* Red gamma LUT (256×16-bit) */
#define FPGA_REG_GAMMA_G_LUT    0x0200   /* Green gamma LUT (256×16-bit) */
#define FPGA_REG_GAMMA_B_LUT    0x0300   /* Blue gamma LUT (256×16-bit) */

/* =========================================================================
 * Peripheral Clock Enable Macros
 * ========================================================================= */

/* These macros use the HAL-defined __HAL_RCC_* macros but are documented
 * here for reference with their underlying register/bit operations. */

/* GPIO clock enable */
#define RCC_GPIOA_CLK_ENABLE()  (RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN)
#define RCC_GPIOB_CLK_ENABLE()  (RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN)
#define RCC_GPIOC_CLK_ENABLE()  (RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN)
#define RCC_GPIOD_CLK_ENABLE()  (RCC->AHB2ENR |= RCC_AHB2ENR_GPIODEN)
#define RCC_GPIOE_CLK_ENABLE()  (RCC->AHB2ENR |= RCC_AHB2ENR_GPIOEEN)
#define RCC_GPIOH_CLK_ENABLE()  (RCC->AHB2ENR |= RCC_AHB2ENR_GPIOHEN)

/* SPI clock enable */
#define RCC_SPI1_CLK_ENABLE()   (RCC->APB2ENR |= RCC_APB2ENR_SPI1EN)
#define RCC_SPI2_CLK_ENABLE()   (RCC->APB1LENR |= RCC_APB1LENR_SPI2EN)

/* USART clock enable */
#define RCC_USART1_CLK_ENABLE() (RCC->APB2ENR |= RCC_APB2ENR_USART1EN)

/* I2C clock enable */
#define RCC_I2C2_CLK_ENABLE()   (RCC->APB1LENR |= RCC_APB1LENR_I2C2EN)
#define RCC_I2C3_CLK_ENABLE()   (RCC->APB1LENR |= RCC_APB1LENR_I2C3EN)
#define RCC_I2C4_CLK_ENABLE()   (RCC->APB4ENR |= RCC_APB4ENR_I2C4EN)

/* DMA clock enable */
#define RCC_DMA1_CLK_ENABLE()   (RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN)

/* SDMMC clock enable */
#define RCC_SDMMC1_CLK_ENABLE() (RCC->AHB3ENR |= RCC_AHB3ENR_SDMMC1EN)

/* ADC clock enable */
#define RCC_ADC12_CLK_ENABLE()  (RCC->AHB1ENR |= RCC_AHB1ENR_ADC12EN)

/* TIM clock enable */
#define RCC_TIM1_CLK_ENABLE()   (RCC->APB2ENR |= RCC_APB2ENR_TIM1EN)

/* SYSCFG clock enable */
#define RCC_SYSCFG_CLK_ENABLE() (RCC->AHB4ENR |= RCC_AHB4ENR_SYSCFGEN)

/* =========================================================================
 * NVIC Interrupt Numbers
 * ========================================================================= */

#define IRQN_DMA1_STREAM0       11
#define IRQN_DMA1_STREAM1       12
#define IRQN_DMA1_STREAM2       13
#define IRQN_DMA1_STREAM3       14
#define IRQN_DMA1_STREAM4       15
#define IRQN_DMA1_STREAM5       16
#define IRQN_DMA1_STREAM6       17
#define IRQN_DMA1_STREAM7       47
#define IRQN_EXTI0              6
#define IRQN_EXTI1              7
#define IRQN_EXTI2              8
#define IRQN_EXTI3              9
#define IRQN_EXTI4              10
#define IRQN_EXTI9_5            23
#define IRQN_EXTI15_10          40
#define IRQN_USART1             37
#define IRQN_SPI1               35
#define IRQN_SPI2               36
#define IRQN_I2C2               33
#define IRQN_I2C3               34
#define IRQN_I2C4               95
#define IRQN_SDMMC1             49
#define IRQN_ETH                61
#define IRQN_ETH_WKUP           62
#define IRQN_USB_OTG_FS         67
#define IRQN_TIM1_UP            24
#define IRQN_TIM1_CC            25
#define IRQN_ADC1               18

#endif /* REGISTERS_H */
