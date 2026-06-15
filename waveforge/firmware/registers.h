/* ============================================
 * registers.h — WaveForge MMIO Register Map
 * ============================================ */

#ifndef WAVEFORGE_REGISTERS_H
#define WAVEFORGE_REGISTERS_H

#include <stdint.h>

/* ---- Base Addresses ---- */
#define PERIPH_BASE         0x40000000UL
#define AHB4PERIPH_BASE     (PERIPH_BASE + 0x18000000UL)
#define APB1PERIPH_BASE     (PERIPH_BASE + 0x00000000UL)
#define APB2PERIPH_BASE     (PERIPH_BASE + 0x00010000UL)
#define AHB1PERIPH_BASE     (PERIPH_BASE + 0x20000000UL)

/* ---- GPIO Registers ---- */
typedef struct {
    volatile uint32_t MODER;
    volatile uint32_t OTYPER;
    volatile uint32_t OSPEEDR;
    volatile uint32_t PUPDR;
    volatile uint32_t IDR;
    volatile uint32_t ODR;
    volatile uint32_t BSRR;
    volatile uint32_t LCKR;
    volatile uint32_t AFRL;
    volatile uint32_t AFRH;
} GPIO_TypeDef;

#define GPIOA_BASE  (AHB4PERIPH_BASE + 0x0000UL)
#define GPIOB_BASE  (AHB4PERIPH_BASE + 0x0400UL)
#define GPIOC_BASE  (AHB4PERIPH_BASE + 0x0800UL)
#define GPIOE_BASE  (AHB4PERIPH_BASE + 0x1000UL)

#define GPIOA ((GPIO_TypeDef *)GPIOA_BASE)
#define GPIOB ((GPIO_TypeDef *)GPIOB_BASE)
#define GPIOC ((GPIO_TypeDef *)GPIOC_BASE)
#define GPIOE ((GPIO_TypeDef *)GPIOE_BASE)

/* ---- RCC Registers ---- */
typedef struct {
    volatile uint32_t CR;
    volatile uint32_t HSICFGR;
    volatile uint32_t CRRCR;
    volatile uint32_t CSICFGR;
    volatile uint32_t CFGR;
    volatile uint32_t RESERVED1;
    volatile uint32_t D1CFGR;
    volatile uint32_t D2CFGR;
    volatile uint32_t D3CFGR;
    volatile uint32_t RESERVED2[2];
    volatile uint32_t PLLCKSELR;
    volatile uint32_t PLLCFGR;
    volatile uint32_t PLL1DIVR;
    volatile uint32_t PLL1FRACR;
    volatile uint32_t PLL2DIVR;
    volatile uint32_t PLL2FRACR;
    volatile uint32_t PLL3DIVR;
    volatile uint32_t PLL3FRACR;
    volatile uint32_t RESERVED3;
    volatile uint32_t GCR;
    volatile uint32_t RESERVED4[2];
    volatile uint32_t D1CCIPR;
    volatile uint32_t D2CCIP1R;
    volatile uint32_t D2CCIP2R;
    volatile uint32_t D3CCIPR;
    volatile uint32_t RESERVED5[6];
    volatile uint32_t AHB1ENR;
    volatile uint32_t AHB2ENR;
    volatile uint32_t AHB3ENR;
    volatile uint32_t AHB4ENR;
    volatile uint32_t APB1LENR;
    volatile uint32_t APB1HENR;
    volatile uint32_t APB2ENR;
    volatile uint32_t APB3ENR;
    volatile uint32_t APB4ENR;
} RCC_TypeDef;

#define RCC_BASE (AHB1PERIPH_BASE + 0x4400UL)
#define RCC ((RCC_TypeDef *)RCC_BASE)

/* RCC CR bits */
#define RCC_CR_HSEON     (1 << 16)
#define RCC_CR_HSERDY    (1 << 17)
#define RCC_CR_PLL1ON    (1 << 24)
#define RCC_CR_PLL1RDY   (1 << 25)
#define RCC_CR_PLL2ON    (1 << 26)
#define RCC_CR_PLL2RDY   (1 << 27)

/* ---- SPI1 Registers ---- */
typedef struct {
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t SR;
    volatile uint32_t DR;
    volatile uint32_t CRCPR;
    volatile uint32_t RXCRCR;
    volatile uint32_t TXCRCR;
} SPI_TypeDef;

#define SPI1_BASE (APB2PERIPH_BASE + 0x4000UL)
#define SPI1 ((SPI_TypeDef *)SPI1_BASE)

/* SPI SR bits */
#define SPI_SR_RXNE   (1 << 0)
#define SPI_SR_TXE    (1 << 1)
#define SPI_SR_BSY    (1 << 7)

/* ---- I2C1 Registers ---- */
typedef struct {
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t OAR1;
    volatile uint32_t OAR2;
    volatile uint32_t TIMINGR;
    volatile uint32_t TIMEOUTR;
    volatile uint32_t ISR;
    volatile uint32_t ICR;
    volatile uint32_t PECR;
    volatile uint32_t RXDR;
    volatile uint32_t TXDR;
} I2C_TypeDef;

#define I2C1_BASE (APB1PERIPH_BASE + 0x5400UL)
#define I2C1 ((I2C_TypeDef *)I2C1_BASE)

/* I2C ISR bits */
#define I2C_ISR_TXE     (1 << 0)
#define I2C_ISR_TXIS    (1 << 1)
#define I2C_ISR_RXNE    (1 << 2)
#define I2C_ISR_TC      (1 << 6)
#define I2C_ISR_STOPF   (1 << 5)
#define I2C_ISR_NACKF   (1 << 4)
#define I2C_ISR_BUSY    (1 << 15)

/* I2C CR1 bits */
#define I2C_CR1_PE      (1 << 0)
#define I2C_CR1_TXIE    (1 << 2)
#define I2C_CR1_RXIE    (1 << 3)
#define I2C_CR1_STOPIE  (1 << 5)
#define I2C_CR1_NACKIE  (1 << 4)

/* I2C CR2 bits */
#define I2C_CR2_START   (1 << 13)
#define I2C_CR2_STOP    (1 << 14)
#define I2C_CR2_RD_WRN  (1 << 10)

/* I2C ICR bits */
#define I2C_ICR_STOPCF  (1 << 5)
#define I2C_ICR_NACKCF  (1 << 4)

/* ---- I2S Registers (shared with SPI) ---- */
#define SPI_I2SCFGR   (*(volatile uint32_t *)(SPI1_BASE + 0x28UL))
#define SPI_I2SPR     (*(volatile uint32_t *)(SPI1_BASE + 0x2CUL))

/* ---- DMA Registers (Stream-based) ---- */
typedef struct {
    volatile uint32_t CR;
    volatile uint32_t NDTR;
    volatile uint32_t PAR;
    volatile uint32_t M0AR;
    volatile uint32_t M1AR;
    volatile uint32_t FCR;
} DMA_Stream_TypeDef;

#define DMA1_BASE      (AHB1PERIPH_BASE + 0x6000UL)
#define DMA1_Stream5   ((DMA_Stream_TypeDef *)(DMA1_BASE + 0x58UL + 0x18UL * 5))

/* ---- ADC1 Registers ---- */
typedef struct {
    volatile uint32_t ISR;
    volatile uint32_t IER;
    volatile uint32_t CR;
    volatile uint32_t CFGR;
    volatile uint32_t CFGR2;
    volatile uint32_t SMPR1;
    volatile uint32_t SMPR2;
    volatile uint32_t RESERVED1;
    volatile uint32_t TR1;
    volatile uint32_t TR2;
    volatile uint32_t TR3;
    volatile uint32_t RESERVED2;
    volatile uint32_t JDR1;
    volatile uint32_t JDR2;
    volatile uint32_t JDR3;
    volatile uint32_t JDR4;
    volatile uint32_t DR;
} ADC_TypeDef;

#define ADC1_BASE (AHB4PERIPH_BASE + 0x2000UL)
#define ADC1 ((ADC_TypeDef *)ADC1_BASE)

/* ADC ISR bits */
#define ADC_ISR_EOC (1 << 2)

/* ADC CR bits */
#define ADC_CR_ADEN (1 << 0)

/* ---- UART4 Registers ---- */
typedef struct {
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t CR3;
    volatile uint32_t BRR;
    volatile uint32_t RESERVED1;
    volatile uint32_t RQR;
    volatile uint32_t ISR;
    volatile uint32_t ICR;
    volatile uint32_t RDR;
    volatile uint32_t TDR;
} UART_TypeDef;

#define UART4_BASE (APB1PERIPH_BASE + 0x4C00UL)
#define UART4 ((UART_TypeDef *)UART4_BASE)

/* UART ISR bits */
#define USART_ISR_RXNE  (1 << 5)
#define USART_ISR_TXE   (1 << 7)
#define USART_ISR_TC    (1 << 6)

/* ---- FLASH Registers ---- */
typedef struct {
    volatile uint32_t ACR;
    volatile uint32_t KEYR;
    volatile uint32_t OPTKEYR;
    volatile uint32_t SR;
    volatile uint32_t CR;
    volatile uint32_t ECCR;
    volatile uint32_t RESERVED1;
    volatile uint32_t OPTR;
    volatile uint32_t PCROP1SR;
    volatile uint32_t PCROP1ER;
    volatile uint32_t WRP1AR;
    volatile uint32_t WRP1BR;
} FLASH_TypeDef;

#define FLASH_BASE (0x52002000UL)
#define FLASH ((FLASH_TypeDef *)FLASH_BASE)

/* ---- SCB (System Control Block) ---- */
#define SCB_CPACR  (*(volatile uint32_t *)0xE000ED88UL)
#define SCB_ICIALLU (*(volatile uint32_t *)0xE000EF50UL)

static inline void SCB_EnableICache(void) {
    /* Enable I-Cache */
    __asm volatile("dsb" ::: "memory");
    __asm volatile("isb" ::: "memory");
    SCB_ICIALLU = 0;
    uint32_t *SCB_CCR = (uint32_t *)0xE000ED14UL;
    *SCB_CCR |= (1 << 17);  /* Instruction cache enable */
    __asm volatile("dsb" ::: "memory");
    __asm volatile("isb" ::: "memory");
}

static inline void SCB_EnableDCache(void) {
    uint32_t *SCB_CCR = (uint32_t *)0xE000ED14UL;
    *SCB_CCR |= (1 << 16);  /* Data cache enable */
    __asm volatile("dsb" ::: "memory");
    __asm volatile("isb" ::: "memory");
}

/* ---- SysTick ---- */
#define SysTick_BASE   0xE000E010UL
#define SysTick_LOAD   (*(volatile uint32_t *)(SysTick_BASE + 0x00UL))
#define SysTick_VAL    (*(volatile uint32_t *)(SysTick_BASE + 0x04UL))
#define SysTick_CTRL   (*(volatile uint32_t *)(SysTick_BASE + 0x08UL))

static inline uint32_t SysTick_Config(uint32_t ticks) {
    SysTick_LOAD = ticks - 1;
    SysTick_VAL = 0;
    SysTick_CTRL = (1 << 2)   /* CLKSOURCE = processor clock */
                 | (1 << 1)   /* TICKINT = enable interrupt */
                 | (1 << 0);  /* ENABLE = start */
    return 0;
}

/* ---- WM8778 Codec I2C Register Map ---- */
#define WM8778_REG_LEFT_IN0      0x00
#define WM8778_REG_RIGHT_IN0     0x01
#define WM8778_REG_LEFT_IN1      0x02
#define WM8778_REG_RIGHT_IN1     0x03
#define WM8778_REG_LEFT_ADC      0x04
#define WM8778_REG_RIGHT_ADC     0x05
#define WM8778_REG_DAC_CTRL1     0x06
#define WM8778_REG_DAC_CTRL2     0x07
#define WM8778_REG_DAC_CTRL3     0x08
#define WM8778_REG_LEFT_DAC      0x09
#define WM8778_REG_RIGHT_DAC     0x0A
#define WM8778_REG_LEFT_HP       0x0B
#define WM8778_REG_RIGHT_HP     0x0C
#define WM8778_REG_ADC_MODE     0x0D
#define WM8778_REG_ADC_HPF      0x0E
#define WM8778_REG_GPIO         0x0F
#define WM8778_REG_RESET        0x17

/* WM8778 I2C Address */
#define WM8778_I2C_ADDR       0x1A

/* ---- TPA6130A2 I2C Register Map ---- */
#define TPA6130_REG_VERSION   0x01
#define TPA6130_REG_CTRL      0x02
#define TPA6130_REG_LEFT_VOL 0x03
#define TPA6130_REG_RIGHT_VOL 0x04
#define TPA6130_REG_MUTE      0x06

#define TPA6130_I2C_ADDR     0x1C

/* ---- 24C256 EEPROM I2C ---- */
#define EEPROM_24C256_ADDR   0x50

/* ---- W25Q256 SPI Flash Commands ---- */
#define W25Q256_CMD_READ      0x03
#define W25Q256_CMD_FAST_READ  0x0B
#define W25Q256_CMD_WRITE_ENABLE  0x06
#define W25Q256_CMD_PAGE_PROGRAM  0x02
#define W25Q256_CMD_SECTOR_ERASE  0x20
#define W25Q256_CMD_BLOCK_ERASE_32K 0x52
#define W25Q256_CMD_BLOCK_ERASE_64K 0xD8
#define W25Q256_CMD_CHIP_ERASE  0xC7
#define W25Q256_CMD_READ_STATUS  0x05
#define W25Q256_CMD_READ_ID     0x9F
#define W25Q256_CMD_RELEASE_PWRDN 0xAB

#endif /* WAVEFORGE_REGISTERS_H */