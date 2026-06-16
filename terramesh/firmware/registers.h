/**
 * @file    registers.h
 * @brief   Terramesh geotechnical node — memory-mapped register definitions
 * @author  jayis1
 * @copyright Copyright © 2026 jayis1
 * @license GPL-2.0
 *
 * This file provides register-level access macros for the STM32U5A5ZJT6Q
 * peripherals used by the Terramesh firmware. All register addresses are
 * derived from the STM32U5A5 reference manual (RM0456).
 */

#ifndef REGISTERS_H
#define REGISTERS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* ======================================================================== *
 *  Bit-band / Register Access Helpers                                        *
 * ======================================================================== */

/** Read a 32-bit register */
#define REG_READ(addr)          (*(volatile uint32_t *)(addr))

/** Write a 32-bit register */
#define REG_WRITE(addr, val)    ((*(volatile uint32_t *)(addr)) = (val))

/** Set bits in a register (read-modify-write) */
#define REG_SET_BITS(addr, mask)    (REG_WRITE(addr, REG_READ(addr) | (mask)))

/** Clear bits in a register (read-modify-write) */
#define REG_CLR_BITS(addr, mask)    (REG_WRITE(addr, REG_READ(addr) & ~(mask)))

/** Read a bitfield: extract bits [high:low] */
#define REG_READ_BF(addr, high, low) \
    ((REG_READ(addr) >> (low)) & ((1UL << ((high) - (low) + 1)) - 1))

/** Write a bitfield: set bits [high:low] to value */
#define REG_WRITE_BF(addr, high, low, val) do { \
    uint32_t _mask = ((1UL << ((high) - (low) + 1)) - 1) << (low); \
    REG_WRITE(addr, (REG_READ(addr) & ~_mask) | (((uint32_t)(val) << (low)) & _mask)); \
} while (0)

/** Wait for a bit to be set (with timeout in iterations) */
#define REG_WAIT_BIT_SET(addr, mask, timeout) ({ \
    uint32_t _t = (timeout); \
    while (!(REG_READ(addr) & (mask)) && _t--) { \
        __asm__("nop"); \
    } \
    _t; \
})

/** Wait for a bit to be cleared (with timeout in iterations) */
#define REG_WAIT_BIT_CLR(addr, mask, timeout) ({ \
    uint32_t _t = (timeout); \
    while ((REG_READ(addr) & (mask)) && _t--) { \
        __asm__("nop"); \
    } \
    _t; \
})

/* ======================================================================== *
 *  RCC — Reset and Clock Control                                             *
 * ======================================================================== */
#define RCC_BASE                0x40021000UL

/* RCC registers (offsets from base) */
#define RCC_CR                  (RCC_BASE + 0x000)  /* Clock control */
#define RCC_CFGR                (RCC_BASE + 0x004)  /* Clock configuration */
#define RCC_PLLCFGR             (RCC_BASE + 0x008)  /* PLL configuration */
#define RCC_AHB1ENR             (RCC_BASE + 0x048)  /* AHB1 peripheral clock enable */
#define RCC_AHB2ENR             (RCC_BASE + 0x04C)  /* AHB2 peripheral clock enable */
#define RCC_APB1ENR1            (RCC_BASE + 0x050)  /* APB1 peripheral clock enable 1 */
#define RCC_APB1ENR2            (RCC_BASE + 0x054)  /* APB1 peripheral clock enable 2 */
#define RCC_APB2ENR             (RCC_BASE + 0x058)  /* APB2 peripheral clock enable */
#define RCC_APB3ENR             (RCC_BASE + 0x05C)  /* APB3 peripheral clock enable */
#define RCC_CSR                 (RCC_BASE + 0x074)  /* Control/status */
#define RCC_BDCR                (RCC_BASE + 0x078)  /* Backup domain control */

/* RCC_CR bit definitions */
#define RCC_CR_HSI16ON          (1UL << 0)
#define RCC_CR_HSI16RDYF        (1UL << 1)
#define RCC_CR_MSION            (1UL << 8)
#define RCC_CR_MSIRDY           (1UL << 9)
#define RCC_CR_PLLON            (1UL << 24)
#define RCC_CR_PLLRDY           (1UL << 25)

/* RCC_CFGR bit definitions */
#define RCC_CFGR_SW_MSI         (0UL << 0)
#define RCC_CFGR_SW_HSI16       (1UL << 0)
#define RCC_CFGR_SW_PLL         (3UL << 0)
#define RCC_CFGR_SWS_PLL        (3UL << 2)

/* RCC_PLLCFGR bit definitions */
#define RCC_PLLCFGR_PLLSRC_HSI16 (1UL << 0)
#define RCC_PLLCFGR_PLLM_POS    4
#define RCC_PLLCFGR_PLLM_MASK  (0x3FUL << RCC_PLLCFGR_PLLM_POS)
#define RCC_PLLCFGR_PLLN_POS    10
#define RCC_PLLCFGR_PLLN_MASK  (0x1FFUL << RCC_PLLCFGR_PLLN_POS)
#define RCC_PLLCFGR_PLLP_POS    22
#define RCC_PLLCFGR_PLLP_MASK  (0x3UL << RCC_PLLCFGR_PLLP_POS)
#define RCC_PLLCFGR_PLLP_DIV4  (2UL << RCC_PLLCFGR_PLLP_POS)
#define RCC_PLLCFGR_PLLQ_POS    24
#define RCC_PLLCFGR_PLLQ_MASK  (0x7UL << RCC_PLLCFGR_PLLQ_POS)

/* Peripheral clock enable bits */
#define RCC_AHB1ENR_DMA1EN     (1UL << 0)
#define RCC_AHB1ENR_DMA2EN     (1UL << 1)
#define RCC_AHB1ENR_FLASHEN    (1UL << 8)
#define RCC_AHB1ENR_CRCEN      (1UL << 12)

#define RCC_AHB2ENR_GPIOAEN    (1UL << 0)
#define RCC_AHB2ENR_GPIOBEN    (1UL << 1)
#define RCC_AHB2ENR_GPIOCEN    (1UL << 2)
#define RCC_AHB2ENR_GPIODEN    (1UL << 3)
#define RCC_AHB2ENR_ADCEN      (1UL << 13)

#define RCC_APB1ENR1_TIM2EN    (1UL << 0)
#define RCC_APB1ENR1_TIM6EN    (1UL << 4)
#define RCC_APB1ENR1_SPI2EN    (1UL << 14)
#define RCC_APB1ENR1_USART2EN  (1UL << 17)
#define RCC_APB1ENR1_I2C1EN    (1UL << 21)
#define RCC_APB1ENR1_I2C2EN    (1UL << 22)
#define RCC_APB1ENR1_LPUART1EN (1UL << 28)

#define RCC_APB1ENR2_LPTIM2EN  (1UL << 2)

#define RCC_APB2ENR_SPI1EN     (1UL << 12)

/* ======================================================================== *
 *  GPIO — General Purpose I/O                                                *
 * ======================================================================== */
#define GPIOA_BASE              0x42020000UL
#define GPIOB_BASE              0x42020400UL
#define GPIOC_BASE              0x42020800UL
#define GPIOD_BASE              0x42020C00UL

/* GPIO register offsets */
#define GPIO_MODER_OFF          0x00  /* Mode register */
#define GPIO_OTYPER_OFF         0x04  /* Output type */
#define GPIO_OSPEEDR_OFF        0x08  /* Output speed */
#define GPIO_PUPDR_OFF          0x0C  /* Pull-up/pull-down */
#define GPIO_IDR_OFF            0x10  /* Input data */
#define GPIO_ODR_OFF            0x14  /* Output data */
#define GPIO_BSRR_OFF           0x18  /* Bit set/reset */
#define GPIO_LCKR_OFF           0x1C  /* Lock */
#define GPIO_AFRL_OFF           0x20  /* Alternate function low */
#define GPIO_AFRH_OFF           0x24  /* Alternate function high */

/* GPIO_MODER modes */
#define GPIO_MODE_INPUT         0
#define GPIO_MODE_OUTPUT        1
#define GPIO_MODE_AF            2
#define GPIO_MODE_ANALOG        3

/* GPIO_OTYPER */
#define GPIO_OTYPE_PP           0
#define GPIO_OTYPE_OD            1

/* GPIO_OSPEEDR */
#define GPIO_SPEED_LOW          0
#define GPIO_SPEED_MEDIUM       1
#define GPIO_SPEED_HIGH         2
#define GPIO_SPEED_VERY_HIGH    3

/* GPIO_PUPDR */
#define GPIO_PUPD_NONE          0
#define GPIO_PUPD_PULLUP        1
#define GPIO_PUPD_PULLDOWN      2

/* GPIO_BSRR */
#define GPIO_BSRR_BS_POS        0
#define GPIO_BSRR_BR_POS        16

/* Helper macros for GPIO access */
#define GPIO_MODER(port)        (port + GPIO_MODER_OFF)
#define GPIO_OTYPER(port)       (port + GPIO_OTYPER_OFF)
#define GPIO_OSPEEDR(port)      (port + GPIO_OSPEEDR_OFF)
#define GPIO_PUPDR(port)        (port + GPIO_PUPDR_OFF)
#define GPIO_IDR(port)          (port + GPIO_IDR_OFF)
#define GPIO_ODR(port)          (port + GPIO_ODR_OFF)
#define GPIO_BSRR(port)         (port + GPIO_BSRR_OFF)
#define GPIO_AFRL(port)         (port + GPIO_AFRL_OFF)
#define GPIO_AFRH(port)         (port + GPIO_AFRH_OFF)

/* Set a GPIO pin high */
#define GPIO_SET_PIN(port, pin)  REG_WRITE(GPIO_BSRR(port), (pin) << GPIO_BSRR_BS_POS)

/* Set a GPIO pin low */
#define GPIO_CLR_PIN(port, pin)  REG_WRITE(GPIO_BSRR(port), (pin) << GPIO_BSRR_BR_POS)

/* Read a GPIO pin */
#define GPIO_READ_PIN(port, pin) ((REG_READ(GPIO_IDR(port)) & (pin)) != 0)

/* Toggle a GPIO pin */
#define GPIO_TOGGLE_PIN(port, pin) do { \
    if (GPIO_READ_PIN(port, pin)) { \
        GPIO_CLR_PIN(port, pin); \
    } else { \
        GPIO_SET_PIN(port, pin); \
    } \
} while (0)

/* Configure GPIO pin mode */
#define GPIO_SET_MODE(port, pin, mode) do { \
    uint32_t _pos = __builtin_ctz(pin) * 2; \
    REG_WRITE_BF(GPIO_MODER(port), _pos + 1, _pos, mode); \
} while (0)

/* Configure GPIO pin alternate function */
#define GPIO_SET_AF(port, pin, af) do { \
    uint32_t _pos = __builtin_ctz(pin); \
    if (_pos < 8) { \
        REG_WRITE_BF(GPIO_AFRL(port), _pos * 4 + 3, _pos * 4, af); \
    } else { \
        REG_WRITE_BF(GPIO_AFRH(port), (_pos - 8) * 4 + 3, (_pos - 8) * 4, af); \
    } \
} while (0)

/* ======================================================================== *
 *  SPI — Serial Peripheral Interface                                          *
 * ======================================================================== */
#define SPI1_BASE               0x40013000UL
#define SPI2_BASE               0x40003800UL

/* SPI register offsets */
#define SPI_CR1_OFF             0x00
#define SPI_CR2_OFF             0x04
#define SPI_SR_OFF              0x08
#define SPI_DR_OFF              0x0C
#define SPI_CRCPR_OFF           0x10
#define SPI_RXCRCR_OFF          0x14
#define SPI_TXCRCR_OFF          0x18
#define SPI_I2SCFGR_OFF         0x1C
#define SPI_I2SPR_OFF           0x20

/* SPI_CR1 bits */
#define SPI_CR1_CPHA            (1UL << 0)
#define SPI_CR1_CPOL            (1UL << 1)
#define SPI_CR1_MSTR            (1UL << 2)
#define SPI_CR1_BR_POS          3
#define SPI_CR1_BR_MASK         (7UL << SPI_CR1_BR_POS)
#define SPI_CR1_SPE             (1UL << 6)
#define SPI_CR1_LSBFIRST        (1UL << 7)
#define SPI_CR1_SSI             (1UL << 8)
#define SPI_CR1_SSM             (1UL << 9)
#define SPI_CR1_RXONLY          (1UL << 10)
#define SPI_CR1_DFF             (1UL << 11)
#define SPI_CR1_CRCNEXT         (1UL << 12)
#define SPI_CR1_CRCEN           (1UL << 13)
#define SPI_CR1_BIDIOE          (1UL << 14)
#define SPI_CR1_BIDIMODE        (1UL << 15)

/* SPI_SR bits */
#define SPI_SR_RXNE             (1UL << 0)
#define SPI_SR_TXE              (1UL << 1)
#define SPI_SR_BSY              (1UL << 7)
#define SPI_SR_FRE              (1UL << 8)

/* SPI_CR2 bits */
#define SPI_CR2_RXDMAEN         (1UL << 0)
#define SPI_CR2_TXDMAEN         (1UL << 1)
#define SPI_CR2_SSOE            (1UL << 2)
#define SPI_CR2_FRF             (1UL << 4)
#define SPI_CR2_DS_POS          8
#define SPI_CR2_DS_MASK         (0xFUL << SPI_CR2_DS_POS)
#define SPI_CR2_DS_8BIT         (7UL << SPI_CR2_DS_POS)
#define SPI_CR2_DS_16BIT        (15UL << SPI_CR2_DS_POS)

/* SPI register access macros */
#define SPI_CR1(spi)            ((spi) + SPI_CR1_OFF)
#define SPI_CR2(spi)            ((spi) + SPI_CR2_OFF)
#define SPI_SR(spi)             ((spi) + SPI_SR_OFF)
#define SPI_DR(spi)             ((spi) + SPI_DR_OFF)

/* SPI baud rate prescaler values (for CR1[5:3]) */
#define SPI_BR_DIV2             0
#define SPI_BR_DIV4             1
#define SPI_BR_DIV8             2
#define SPI_BR_DIV16            3
#define SPI_BR_DIV32            4
#define SPI_BR_DIV64            5
#define SPI_BR_DIV128           6
#define SPI_BR_DIV256           7

/* ======================================================================== *
 *  I²C — Inter-Integrated Circuit                                            *
 * ======================================================================== */
#define I2C1_BASE               0x40005400UL
#define I2C2_BASE               0x40005800UL

/* I²C register offsets */
#define I2C_CR1_OFF             0x00
#define I2C_CR2_OFF             0x04
#define I2C_OAR1_OFF            0x08
#define I2C_OAR2_OFF            0x0C
#define I2C_TIMINGR_OFF         0x10
#define I2C_TIMEOUTR_OFF        0x14
#define I2C_ISR_OFF             0x18
#define I2C_ICR_OFF             0x1C
#define I2C_DR_OFF              0x20

/* I²C_CR1 bits */
#define I2C_CR1_PE              (1UL << 0)
#define I2C_CR1_TXIE            (1UL << 1)
#define I2C_CR1_RXIE            (1UL << 2)
#define I2C_CR1_TCIE            (1UL << 4)
#define I2C_CR1_STOPIE          (1UL << 5)
#define I2C_CR1_NACKIE          (1UL << 6)
#define I2C_CR1_ERRIE           (1UL << 7)
#define I2C_CR1_ANFOFF          (1UL << 12)
#define I2C_CR1_TXDMAEN         (1UL << 14)
#define I2C_CR1_RXDMAEN         (1UL << 15)

/* I²C_CR2 bits */
#define I2C_CR2_SADD_POS        0
#define I2C_CR2_SADD_MASK       (0x3FFUL << I2C_CR2_SADD_POS)
#define I2C_CR2_RD_WRN          (1UL << 10)
#define I2C_CR2_START           (1UL << 13)
#define I2C_CR2_STOP            (1UL << 14)
#define I2C_CR2_NBYTES_POS      16
#define I2C_CR2_NBYTES_MASK     (0xFFUL << I2C_CR2_NBYTES_POS)
#define I2C_CR2_AUTOEND         (1UL << 25)

/* I²C_ISR bits */
#define I2C_ISR_TXE             (1UL << 0)
#define I2C_ISR_RXNE            (1UL << 1)
#define I2C_ISR_TC              (1UL << 6)
#define I2C_ISR_TCR             (1UL << 7)
#define I2C_ISR_NACKF           (1UL << 9)
#define I2C_ISR_BUSY            (1UL << 15)

/* I²C register access macros */
#define I2C_CR1(i2c)            ((i2c) + I2C_CR1_OFF)
#define I2C_CR2(i2c)            ((i2c) + I2C_CR2_OFF)
#define I2C_ISR(i2c)            ((i2c) + I2C_ISR_OFF)
#define I2C_ICR(i2c)            ((i2c) + I2C_ICR_OFF)
#define I2C_DR(i2c)             ((i2c) + I2C_DR_OFF)
#define I2C_TIMINGR(i2c)        ((i2c) + I2C_TIMINGR_OFF)

/* ======================================================================== *
 *  USART — Universal Synchronous Asynchronous Receiver Transmitter            *
 * ======================================================================== */
#define USART2_BASE             0x40004400UL
#define LPUART1_BASE            0x40008000UL

/* USART register offsets */
#define USART_CR1_OFF           0x00
#define USART_CR2_OFF           0x04
#define USART_CR3_OFF           0x08
#define USART_BRR_OFF           0x0C
#define USART_ISR_OFF           0x1C
#define USART_ICR_OFF           0x20
#define USART_RDR_OFF           0x24
#define USART_TDR_OFF           0x28

/* USART_CR1 bits */
#define USART_CR1_UE            (1UL << 0)
#define USART_CR1_UESM          (1UL << 1)
#define USART_CR1_RE            (1UL << 2)
#define USART_CR1_TE            (1UL << 3)
#define USART_CR1_RXNEIE        (1UL << 5)
#define USART_CR1_TCIE          (1UL << 6)
#define USART_CR1_TXEIE         (1UL << 7)
#define USART_CR1_M1            (1UL << 12)
#define USART_CR1_M0            (1UL << 28)

/* USART_ISR bits */
#define USART_ISR_RXNE          (1UL << 5)
#define USART_ISR_TC            (1UL << 6)
#define USART_ISR_TXE           (1UL << 7)
#define USART_ISR_BUSY          (1UL << 16)

/* USART register access macros */
#define USART_CR1(uart)         ((uart) + USART_CR1_OFF)
#define USART_CR2(uart)         ((uart) + USART_CR2_OFF)
#define USART_CR3(uart)         ((uart) + USART_CR3_OFF)
#define USART_BRR(uart)         ((uart) + USART_BRR_OFF)
#define USART_ISR(uart)         ((uart) + USART_ISR_OFF)
#define USART_ICR(uart)         ((uart) + USART_ICR_OFF)
#define USART_RDR(uart)         ((uart) + USART_RDR_OFF)
#define USART_TDR(uart)         ((uart) + USART_TDR_OFF)

/* ======================================================================== *
 *  EXTI — External Interrupts                                                 *
 * ======================================================================== */
#define EXTI_BASE               0x46001000UL

#define EXTI_RTSR1              (EXTI_BASE + 0x000)
#define EXTI_FTSR1              (EXTI_BASE + 0x004)
#define EXTI_SWIER1             (EXTI_BASE + 0x008)
#define EXTI_PR1                (EXTI_BASE + 0x00C)
#define EXTI_EXTICR1            (EXTI_BASE + 0x060)
#define EXTI_EXTICR2            (EXTI_BASE + 0x064)
#define EXTI_EXTICR3            (EXTI_BASE + 0x068)
#define EXTI_EXTICR4            (EXTI_BASE + 0x06C)
#define EXTI_IMR1               (EXTI_BASE + 0x080)

/* ======================================================================== *
 *  NVIC — Nested Vectored Interrupt Controller                                *
 * ======================================================================== */
#define NVIC_ISER0              0xE000E100UL
#define NVIC_ICER0              0xE000E180UL
#define NVIC_ISPR0              0xE000E200UL
#define NVIC_ICPR0              0xE000E280UL
#define NVIC_IPR_BASE           0xE000E400UL

#define NVIC_ENABLE_IRQ(n)      REG_WRITE(NVIC_ISER0, 1UL << (n))
#define NVIC_DISABLE_IRQ(n)     REG_WRITE(NVIC_ICER0, 1UL << (n))
#define NVIC_SET_PENDING(n)     REG_WRITE(NVIC_ISPR0, 1UL << (n))
#define NVIC_CLEAR_PENDING(n)   REG_WRITE(NVIC_ICPR0, 1UL << (n))
#define NVIC_SET_PRIORITY(n, p) REG_WRITE(NVIC_IPR_BASE + ((n) * 4), (p) << 4)

/* IRQ numbers (STM32U5A5) */
#define IRQ_WWDG                0
#define IRQ_PVD_PVM             1
#define IRQ_RTC_TAMP            2
#define IRQ_RTC_WKUP            3
#define IRQ_FLASH_ITF           4
#define IRQ_RCC                 5
#define IRQ_EXTI0               6
#define IRQ_EXTI1               7
#define IRQ_EXTI2               8
#define IRQ_EXTI3               9
#define IRQ_EXTI4               10
#define IRQ_DMA1_CH1            11
#define IRQ_DMA1_CH2            12
#define IRQ_DMA1_CH3            13
#define IRQ_DMA1_CH4            14
#define IRQ_DMA1_CH5            15
#define IRQ_DMA1_CH6            16
#define IRQ_DMA1_CH7            17
#define IRQ_ADC1                18
#define IRQ_TIM1_BRK            19
#define IRQ_TIM1_UP             20
#define IRQ_TIM1_TRG_COM        21
#define IRQ_TIM1_CC             22
#define IRQ_TIM2                23
#define IRQ_TIM3                24
#define IRQ_TIM4                25
#define IRQ_I2C1_EV             27
#define IRQ_I2C1_ER             28
#define IRQ_I2C2_EV             29
#define IRQ_I2C2_ER             30
#define IRQ_SPI1                31
#define IRQ_SPI2                32
#define IRQ_USART1              33
#define IRQ_USART2              34
#define IRQ_LPUART1             35
#define IRQ_EXTI9_5             36
#define IRQ_TIM15               37
#define IRQ_TIM16               38
#define IRQ_TIM17               39
#define IRQ_DMA2_CH1            40
#define IRQ_DMA2_CH2            41
#define IRQ_DMA2_CH3            42
#define IRQ_DMA2_CH4            43
#define IRQ_DMA2_CH5            44
#define IRQ_DMA2_CH6            45
#define IRQ_DMA2_CH7            46
#define IRQ_EXTI15_10           47
#define IRQ_RTC_ALARM           48
#define IRQ_LPTIM1              49
#define IRQ_LPTIM2              50

/* ======================================================================== *
 *  SysTick — System Tick Timer                                                *
 * ======================================================================== */
#define SYSTICK_BASE            0xE000E010UL
#define SYSTICK_CSR             (SYSTICK_BASE + 0x00)
#define SYSTICK_RVR             (SYSTICK_BASE + 0x04)
#define SYSTICK_CVR             (SYSTICK_BASE + 0x08)

#define SYSTICK_CSR_ENABLE      (1UL << 0)
#define SYSTICK_CSR_TICKINT     (1UL << 1)
#define SYSTICK_CSR_CLKSOURCE   (1UL << 2)
#define SYSTICK_CSR_COUNTFLAG   (1UL << 16)

/* ======================================================================== *
 *  SCB — System Control Block                                                 *
 * ======================================================================== */
#define SCB_BASE                0xE000ED00UL
#define SCB_SCR                 (SCB_BASE + 0x10)
#define SCB_SLEEPDEEP           (1UL << 2)
#define SCB_SLEEPONEXIT         (1UL << 1)

/* ======================================================================== *
 *  PWR — Power Controller                                                     *
 * ======================================================================== */
#define PWR_BASE                0x46002000UL
#define PWR_CR1                 (PWR_BASE + 0x00)
#define PWR_CR2                 (PWR_BASE + 0x04)
#define PWR_CR3                 (PWR_BASE + 0x08)
#define PWR_CR4                 (PWR_BASE + 0x0C)
#define PWR_SR1                 (PWR_BASE + 0x10)
#define PWR_SR2                 (PWR_BASE + 0x14)
#define PWR_SCR                 (PWR_BASE + 0x18)

/* PWR_CR1 bits */
#define PWR_CR1_LPR             (1UL << 14)
#define PWR_CR1_STOP2           (1UL << 17)

/* PWR_SR2 bits */
#define PWR_SR2_STOPF           (1UL << 2)
#define PWR_SR2_SBF             (1UL << 3)

/* ======================================================================== *
 *  IWDG — Independent Watchdog                                                 *
 * ======================================================================== */
#define IWDG_BASE               0x40002800UL
#define IWDG_KR                 (IWDG_BASE + 0x00)
#define IWDG_PR                 (IWDG_BASE + 0x04)
#define IWDG_RLR                (IWDG_BASE + 0x08)
#define IWDG_SR                 (IWDG_BASE + 0x0C)

#define IWDG_KR_KEY_RELOAD      0xAAAA
#define IWDG_KR_KEY_ENABLE      0xCCCC
#define IWDG_KR_KEY_ACCESS      0x5555

/* ======================================================================== *
 *  FLASH — Flash Interface                                                    *
 * ======================================================================== */
#define FLASH_IF_BASE           0x40022000UL
#define FLASH_ACR               (FLASH_IF_BASE + 0x00)
#define FLASH_KEYR              (FLASH_IF_BASE + 0x04)
#define FLASH_SR                (FLASH_IF_BASE + 0x10)
#define FLASH_CR                (FLASH_IF_BASE + 0x14)

#define FLASH_ACR_LATENCY_0WS   (0UL << 0)
#define FLASH_ACR_LATENCY_1WS   (1UL << 0)
#define FLASH_ACR_LATENCY_2WS   (2UL << 0)
#define FLASH_ACR_LATENCY_3WS   (3UL << 0)
#define FLASH_ACR_LATENCY_4WS   (4UL << 0)
#define FLASH_ACR_PRFTEN        (1UL << 8)
#define FLASH_ACR_ICEN          (1UL << 9)
#define FLASH_ACR_DCEN          (1UL << 10)

#define FLASH_SR_BSY            (1UL << 0)
#define FLASH_SR_EOP            (1UL << 1)
#define FLASH_SR_WRPERR         (1UL << 8)
#define FLASH_SR_PGERR          (1UL << 9)

#define FLASH_CR_PG             (1UL << 0)
#define FLASH_CR_SER            (1UL << 1)
#define FLASH_CR_SNB_POS        3
#define FLASH_CR_SNB_MASK       (0x3FUL << FLASH_CR_SNB_POS)
#define FLASH_CR_START          (1UL << 16)
#define FLASH_CR_LOCK           (1UL << 31)

/* ======================================================================== *
 *  CRC — Cyclic Redundancy Check                                              *
 * ======================================================================== */
#define CRC_BASE                0x42023000UL
#define CRC_DR                  (CRC_BASE + 0x00)
#define CRC_IDR                 (CRC_BASE + 0x04)
#define CRC_CR                  (CRC_BASE + 0x08)
#define CRC_INIT                (CRC_BASE + 0x10)
#define CRC_POL                 (CRC_BASE + 0x14)

#define CRC_CR_RESET            (1UL << 0)
#define CRC_CR_REV_IN           (1UL << 5)
#define CRC_CR_REV_OUT          (1UL << 7)

#ifdef __cplusplus
}
#endif

#endif /* REGISTERS_H */
