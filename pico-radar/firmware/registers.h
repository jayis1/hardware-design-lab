/*
 * registers.h — STM32H743VIT6 MMIO register map for PicoRadar
 *
 * Base addresses and register offsets for all peripherals used.
 * All registers are volatile — use these macros for direct MMIO access.
 */

#ifndef REGISTERS_H
#define REGISTERS_H

#include <stdint.h>

/* ===================================================================
 * System Control
 * =================================================================== */

#define PERIPH_BASE             0x40000000UL

/* ---- Reset and Clock Control (RCC) ---- */
#define RCC_BASE                0x58024400UL

#define RCC_CR                  (*(volatile uint32_t *)(RCC_BASE + 0x00))
#define RCC_HSICFGR             (*(volatile uint32_t *)(RCC_BASE + 0x04))
#define RCC_D1CFGR              (*(volatile uint32_t *)(RCC_BASE + 0x018))
#define RCC_D2CFGR              (*(volatile uint32_t *)(RCC_BASE + 0x01C))
#define RCC_D3CFGR              (*(volatile uint32_t *)(RCC_BASE + 0x020))
#define RCC_PLLCKSELR            (*(volatile uint32_t *)(RCC_BASE + 0x028))
#define RCC_PLL1DIVR            (*(volatile uint32_t *)(RCC_BASE + 0x030))
#define RCC_PLL1FRACR           (*(volatile uint32_t *)(RCC_BASE + 0x034))
#define RCC_PLL2DIVR            (*(volatile uint32_t *)(RCC_BASE + 0x038))
#define RCC_CFGR                (*(volatile uint32_t *)(RCC_BASE + 0x010))
#define RCC_AHB1ENR             (*(volatile uint32_t *)(RCC_BASE + 0x0D8))
#define RCC_AHB2ENR             (*(volatile uint32_t *)(RCC_BASE + 0x0DC))
#define RCC_AHB4ENR             (*(volatile uint32_t *)(RCC_BASE + 0x0E0))
#define RCC_APB1LENR            (*(volatile uint32_t *)(RCC_BASE + 0x0E8))
#define RCC_APB1HENR            (*(volatile uint32_t *)(RCC_BASE + 0x0EC))
#define RCC_APB2ENR             (*(volatile uint32_t *)(RCC_BASE + 0x0F0))
#define RCC_APB4ENR             (*(volatile uint32_t *)(RCC_BASE + 0x0F4))

/* ---- Power Controller (PWR) ---- */
#define PWR_BASE                0x58024800UL
#define PWR_CR1                 (*(volatile uint32_t *)(PWR_BASE + 0x00))
#define PWR_CSR1                (*(volatile uint32_t *)(PWR_BASE + 0x14))

/* ---- System Configuration (SYSCFG) ---- */
#define SYSCFG_BASE             0x58000400UL
#define SYSCFG_PMCR             (*(volatile uint32_t *)(SYSCFG_BASE + 0x00))
#define SYSCFG_EXTICR1          (*(volatile uint32_t *)(SYSCFG_BASE + 0x08))
#define SYSCFG_EXTICR2          (*(volatile uint32_t *)(SYSCFG_BASE + 0x0C))
#define SYSCFG_EXTICR3          (*(volatile uint32_t *)(SYSCFG_BASE + 0x10))
#define SYSCFG_EXTICR4          (*(volatile uint32_t *)(SYSCFG_BASE + 0x14))

/* ---- Flash Controller ---- */
#define FLASH_BASE_REG          0x52002000UL
#define FLASH_ACR               (*(volatile uint32_t *)(FLASH_BASE_REG + 0x00))

/* ---- Nested Vectored Interrupt Controller (NVIC) ---- */
#define NVIC_BASE               0xE000E100UL
#define NVIC_ISER0              (*(volatile uint32_t *)(NVIC_BASE + 0x00))
#define NVIC_ISER1              (*(volatile uint32_t *)(NVIC_BASE + 0x04))
#define NVIC_ISER2              (*(volatile uint32_t *)(NVIC_BASE + 0x08))
#define NVIC_ICER0              (*(volatile uint32_t *)(NVIC_BASE + 0x80))
#define NVIC_ISPR0              (*(volatile uint32_t *)(NVIC_BASE + 0x100))
#define NVIC_ICPR0              (*(volatile uint32_t *)(NVIC_BASE + 0x180))

/* ---- System Tick (SysTick) ---- */
#define SYSTICK_BASE            0xE000E010UL
#define SYSTICK_CTRL             (*(volatile uint32_t *)(SYSTICK_BASE + 0x00))
#define SYSTICK_LOAD            (*(volatile uint32_t *)(SYSTICK_BASE + 0x04))
#define SYSTICK_VAL             (*(volatile uint32_t *)(SYSTICK_BASE + 0x08))
#define SYSTICK_CALIB           (*(volatile uint32_t *)(SYSTICK_BASE + 0x0C))

/* ---- SCB (System Control Block) ---- */
#define SCB_BASE                0xE000ED00UL
#define SCB_CPUID               (*(volatile uint32_t *)(SCB_BASE + 0x00))
#define SCB_ICSR                (*(volatile uint32_t *)(SCB_BASE + 0x04))
#define SCB_VTOR                (*(volatile uint32_t *)(SCB_BASE + 0x08))
#define SCB_AIRCR               (*(volatile uint32_t *)(SCB_BASE + 0x0C))
#define SCB_SCR                 (*(volatile uint32_t *)(SCB_BASE + 0x10))

/* ===================================================================
 * GPIO
 * =================================================================== */

#define GPIOA_BASE              0x58020000UL
#define GPIOB_BASE              0x58020400UL
#define GPIOC_BASE              0x58020800UL
#define GPIOD_BASE              0x58020C00UL
#define GPIOE_BASE              0x58021000UL
#define GPIOF_BASE              0x58021400UL
#define GPIOG_BASE              0x58021800UL
#define GPIOH_BASE              0x58021C00UL

#define GPIO_MODER(gpio)        (*(volatile uint32_t *)((gpio) + 0x00))
#define GPIO_OTYPER(gpio)       (*(volatile uint32_t *)((gpio) + 0x04))
#define GPIO_OSPEEDR(gpio)      (*(volatile uint32_t *)((gpio) + 0x08))
#define GPIO_PUPDR(gpio)        (*(volatile uint32_t *)((gpio) + 0x0C))
#define GPIO_IDR(gpio)          (*(volatile uint32_t *)((gpio) + 0x10))
#define GPIO_ODR(gpio)          (*(volatile uint32_t *)((gpio) + 0x14))
#define GPIO_BSRR(gpio)         (*(volatile uint32_t *)((gpio) + 0x18))
#define GPIO_LCKR(gpio)         (*(volatile uint32_t *)((gpio) + 0x1C))
#define GPIO_AFRL(gpio)         (*(volatile uint32_t *)((gpio) + 0x20))
#define GPIO_AFRH(gpio)         (*(volatile uint32_t *)((gpio) + 0x24))

/* ===================================================================
 * USART
 * =================================================================== */

#define USART1_BASE             0x40011000UL
#define USART2_BASE             0x40004400UL

#define USART_CR1(usart)        (*(volatile uint32_t *)((usart) + 0x00))
#define USART_CR2(usart)        (*(volatile uint32_t *)((usart) + 0x04))
#define USART_CR3(usart)        (*(volatile uint32_t *)((usart) + 0x08))
#define USART_BRR(usart)        (*(volatile uint32_t *)((usart) + 0x0C))
#define USART_GTPR(usart)       (*(volatile uint32_t *)((usart) + 0x10))
#define USART_RTOR(usart)       (*(volatile uint32_t *)((usart) + 0x14))
#define USART_RQR(usart)        (*(volatile uint32_t *)((usart) + 0x18))
#define USART_ISR(usart)        (*(volatile uint32_t *)((usart) + 0x1C))
#define USART_ICR(usart)        (*(volatile uint32_t *)((usart) + 0x20))
#define USART_RDR(usart)        (*(volatile uint32_t *)((usart) + 0x24))
#define USART_TDR(usart)        (*(volatile uint32_t *)((usart) + 0x28))

/* USART_CR1 bits */
#define USART_CR1_UE            (1 << 0)   /* USART enable */
#define USART_CR1_UESM          (1 << 1)   /* USART enable in Stop mode */
#define USART_CR1_RE            (1 << 2)   /* Receiver enable */
#define USART_CR1_TE            (1 << 3)   /* Transmitter enable */
#define USART_CR1_IDLEIE        (1 << 4)   /* IDLE interrupt enable */
#define USART_CR1_RXNEIE        (1 << 5)   /* RXNE interrupt enable */
#define USART_CR1_TCIE          (1 << 6)   /* TC interrupt enable */
#define USART_CR1_TXEIE         (1 << 7)   /* TXE interrupt enable */
#define USART_CR1_OVER8         (1 << 15)  /* Oversampling by 8 */

/* USART_ISR bits */
#define USART_ISR_TXE           (1 << 7)   /* TX data register empty */
#define USART_ISR_TC            (1 << 6)   /* Transmission complete */
#define USART_ISR_RXNE          (1 << 5)   /* RX data register not empty */
#define USART_ISR_IDLE          (1 << 4)   /* Idle line detected */
#define USART_ISR_ORE           (1 << 3)   /* Overrun error */
#define USART_ISR_FE            (1 << 1)   /* Framing error */

/* ===================================================================
 * SPI
 * =================================================================== */

#define SPI1_BASE               0x40013000UL

#define SPI_CR1(spi)            (*(volatile uint32_t *)((spi) + 0x00))
#define SPI_CR2(spi)            (*(volatile uint32_t *)((spi) + 0x04))
#define SPI_SR(spi)             (*(volatile uint32_t *)((spi) + 0x08))
#define SPI_DR(spi)             (*(volatile uint32_t *)((spi) + 0x0C))
#define SPI_CRCPR(spi)          (*(volatile uint32_t *)((spi) + 0x10))
#define SPI_RXCRCR(spi)         (*(volatile uint32_t *)((spi) + 0x14))
#define SPI_TXCRCR(spi)         (*(volatile uint32_t *)((spi) + 0x18))

/* SPI_SR bits */
#define SPI_SR_RXP              (1 << 0)   /* RX packet available */
#define SPI_SR_TXP              (1 << 1)   /* TX packet space available */
#define SPI_SR_BSY              (1 << 7)   /* Busy flag */
#define SPI_SR_OVR              (1 << 6)   /* Overrun flag */

/* ===================================================================
 * I2C
 * =================================================================== */

#define I2C1_BASE               0x40005400UL

#define I2C_CR1(i2c)            (*(volatile uint32_t *)((i2c) + 0x00))
#define I2C_CR2(i2c)            (*(volatile uint32_t *)((i2c) + 0x04))
#define I2C_OAR1(i2c)           (*(volatile uint32_t *)((i2c) + 0x08))
#define I2C_OAR2(i2c)           (*(volatile uint32_t *)((i2c) + 0x0C))
#define I2C_TIMINGR(i2c)        (*(volatile uint32_t *)((i2c) + 0x10))
#define I2C_TIMEOUTR(i2c)       (*(volatile uint32_t *)((i2c) + 0x14))
#define I2C_ISR(i2c)            (*(volatile uint32_t *)((i2c) + 0x18))
#define I2C_ICR(i2c)            (*(volatile uint32_t *)((i2c) + 0x1C))
#define I2C_PECR(i2c)           (*(volatile uint32_t *)((i2c) + 0x20))
#define I2C_RXDR(i2c)           (*(volatile uint32_t *)((i2c) + 0x24))
#define I2C_TXDR(i2c)           (*(volatile uint32_t *)((i2c) + 0x28))

/* I2C_ISR bits */
#define I2C_ISR_TXE             (1 << 0)   /* TXIS */
#define I2C_ISR_TXIS            (1 << 1)
#define I2C_ISR_RXNE            (1 << 2)
#define I2C_ISR_ADDR            (1 << 3)
#define I2C_ISR_NACKF           (1 << 4)
#define I2C_ISR_STOPF           (1 << 5)
#define I2C_ISR_TC              (1 << 6)
#define I2C_ISR_TCR             (1 << 7)
#define I2C_ISR_BERR            (1 << 8)
#define I2C_ISR_ARLO            (1 << 9)
#define I2C_ISR_OVR             (1 << 10)
#define I2C_ISR_BUSY            (1 << 15)

/* ===================================================================
 * DMA
 * =================================================================== */

#define DMA1_BASE               0x40020000UL
#define DMA2_BASE               0x40020400UL

#define DMA_SPAR(dma, stream)    (*(volatile uint32_t *)((dma) + 0x10 + (stream) * 0x18 + 0x10))
#define DMA_SM0AR(dma, stream)   (*(volatile uint32_t *)((dma) + 0x10 + (stream) * 0x18 + 0x14))
#define DMA_SNDTR(dma, stream)   (*(volatile uint32_t *)((dma) + 0x10 + (stream) * 0x18 + 0x0C))
#define DMA_SCR(dma, stream)     (*(volatile uint32_t *)((dma) + 0x10 + (stream) * 0x18 + 0x00))

/* ===================================================================
 * OctoSPI (QSPI mode)
 * =================================================================== */

#define OCTOSPI_BASE            0x52005000UL
#define OCTOSPI_CR              (*(volatile uint32_t *)(OCTOSPI_BASE + 0x00))
#define OCTOSPI_DCR1            (*(volatile uint32_t *)(OCTOSPI_BASE + 0x04))
#define OCTOSPI_DCR2            (*(volatile uint32_t *)(OCTOSPI_BASE + 0x08))
#define OCTOSPI_DCR3            (*(volatile uint32_t *)(OCTOSPI_BASE + 0x0C))
#define OCTOSPI_DCR4            (*(volatile uint32_t *)(OCTOSPI_BASE + 0x10))
#define OCTOSPI_CCR             (*(volatile uint32_t *)(OCTOSPI_BASE + 0x14))
#define OCTOSPI_TCR             (*(volatile uint32_t *)(OCTOSPI_BASE + 0x18))
#define OCTOSPI_SR              (*(volatile uint32_t *)(OCTOSPI_BASE + 0x1C))
#define OCTOSPI_IR              (*(volatile uint32_t *)(OCTOSPI_BASE + 0x20))
#define OCTOSPI_AR              (*(volatile uint32_t *)(OCTOSPI_BASE + 0x24))
#define OCTOSPI_DR              (*(volatile uint32_t *)(OCTOSPI_BASE + 0x28))

/* ===================================================================
 * Ethernet MAC
 * =================================================================== */

#define ETH_BASE                0x40028000UL
#define ETH_MACCR               (*(volatile uint32_t *)(ETH_BASE + 0x00))
#define ETH_MACFFR              (*(volatile uint32_t *)(ETH_BASE + 0x04))
#define ETH_MACHTHR             (*(volatile uint32_t *)(ETH_BASE + 0x08))
#define ETH_MACHTLR             (*(volatile uint32_t *)(ETH_BASE + 0x0C))
#define ETH_MACMIIAR            (*(volatile uint32_t *)(ETH_BASE + 0x10))
#define ETH_MACMIIDR            (*(volatile uint32_t *)(ETH_BASE + 0x14))
#define ETH_MACFCR             (*(volatile uint32_t *)(ETH_BASE + 0x18))
#define ETH_MACVLANTR          (*(volatile uint32_t *)(ETH_BASE + 0x1C))
#define ETH_MACPFR             (*(volatile uint32_t *)(ETH_BASE + 0x38))
#define ETH_MACWTR             (*(volatile uint32_t *)(ETH_BASE + 0x44))
#define ETH_DMABMR             (*(volatile uint32_t *)(ETH_BASE + 0x1000))
#define ETH_DMATPDR            (*(volatile uint32_t *)(ETH_BASE + 0x1004))
#define ETH_DMARPDR            (*(volatile uint32_t *)(ETH_BASE + 0x1008))
#define ETH_DMASR             (*(volatile uint32_t *)(ETH_BASE + 0x1014))
#define ETH_DMAOMR             (*(volatile uint32_t *)(ETH_BASE + 0x1018))

/* ===================================================================
 * USB OTG FS
 * =================================================================== */

#define USB_OTG_FS_BASE         0x40080000UL
#define USB_OTG_GOTGCTL         (*(volatile uint32_t *)(USB_OTG_FS_BASE + 0x000))
#define USB_OTG_GOTGINT         (*(volatile uint32_t *)(USB_OTG_FS_BASE + 0x004))
#define USB_OTG_GAHBCFG         (*(volatile uint32_t *)(USB_OTG_FS_BASE + 0x008))
#define USB_OTG_GUSBCFG         (*(volatile uint32_t *)(USB_OTG_FS_BASE + 0x00C))
#define USB_OTG_GRSTCTL         (*(volatile uint32_t *)(USB_OTG_FS_BASE + 0x010))
#define USB_OTG_GINTSTS         (*(volatile uint32_t *)(USB_OTG_FS_BASE + 0x014))
#define USB_OTG_GINTMSK         (*(volatile uint32_t *)(USB_OTG_FS_BASE + 0x018))
#define USB_OTG_GRXSTSP         (*(volatile uint32_t *)(USB_OTG_FS_BASE + 0x020))
#define USB_OTG_GRXSTSD         (*(volatile uint32_t *)(USB_OTG_FS_BASE + 0x024))
#define USB_OTG_GCCFG           (*(volatile uint32_t *)(USB_OTG_FS_BASE + 0x038))

#endif /* REGISTERS_H */