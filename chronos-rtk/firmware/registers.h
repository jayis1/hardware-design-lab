/*
 * registers.h — STM32G474 MMIO register map for Chronos-RTK
 * Base addresses and bit definitions for all peripherals used
 */

#ifndef REGISTERS_H
#define REGISTERS_H

#include <stdint.h>

/* ========================================================================== */
/* Peripheral base addresses (STM32G474)                                       */
/* ========================================================================== */
#define PERIPH_BASE          0x40000000UL
#define APB1PERIPH_BASE      PERIPH_BASE
#define APB2PERIPH_BASE      (PERIPH_BASE + 0x00010000UL)
#define AHB1PERIPH_BASE      (PERIPH_BASE + 0x00020000UL)
#define AHB2PERIPH_BASE      (PERIPH_BASE + 0x08000000UL)

/* --- TIM --- */
#define TIM2_BASE            (APB1PERIPH_BASE + 0x0000UL)
#define TIM3_BASE            (APB1PERIPH_BASE + 0x0400UL)
#define TIM6_BASE            (APB1PERIPH_BASE + 0x1000UL)
#define TIM7_BASE            (APB1PERIPH_BASE + 0x1400UL)
#define TIM15_BASE           (APB1PERIPH_BASE + 0x4000UL)
#define TIM16_BASE           (APB1PERIPH_BASE + 0x4400UL)
#define TIM17_BASE           (APB1PERIPH_BASE + 0x4800UL)

/* --- USART --- */
#define USART1_BASE          (APB1PERIPH_BASE + 0x3800UL)
#define USART2_BASE          (APB1PERIPH_BASE + 0x4400UL)
#define USART3_BASE          (APB1PERIPH_BASE + 0x4800UL)

/* --- SPI --- */
#define SPI1_BASE            (APB2PERIPH_BASE + 0x3000UL)
#define SPI2_BASE            (APB1PERIPH_BASE + 0x3800UL)  /* APB1 */
#define SPI3_BASE            (APB1PERIPH_BASE + 0x3C00UL)

/* --- I2C --- */
#define I2C1_BASE            (APB1PERIPH_BASE + 0x5400UL)
#define I2C2_BASE            (APB1PERIPH_BASE + 0x5800UL)
#define I2C3_BASE            (APB1PERIPH_BASE + 0x5C00UL)
#define I2C4_BASE            (APB1PERIPH_BASE + 0x6000UL)

/* --- ADC --- */
#define ADC1_BASE            (APB2PERIPH_BASE + 0x4800UL)
#define ADC12_COMMON_BASE    (APB2PERIPH_BASE + 0x4900UL)

/* --- DMA --- */
#define DMA1_BASE            (AHB1PERIPH_BASE + 0x0000UL)
#define DMA2_BASE            (AHB1PERIPH_BASE + 0x0400UL)
#define DMAMUX1_BASE         (AHB1PERIPH_BASE + 0x0800UL)

/* --- USB --- */
#define USB_BASE             (APB1PERIPH_BASE + 0x5C00UL)  /* USB_FS base */

/* --- SYSCFG --- */
#define SYSCFG_BASE          (APB2PERIPH_BASE + 0x0000UL)

/* --- EXTI --- */
#define EXTI_BASE            (APB2PERIPH_BASE + 0x0000UL)  /* Same as SYSCFG on G4 */

/* --- CRC --- */
#define CRC_BASE             (AHB1PERIPH_BASE + 0x3000UL)

/* --- GPIO --- */
#define GPIOA_BASE           (AHB2PERIPH_BASE + 0x0000UL)
#define GPIOB_BASE           (AHB2PERIPH_BASE + 0x0400UL)
#define GPIOC_BASE           (AHB2PERIPH_BASE + 0x0800UL)
#define GPIOD_BASE           (AHB2PERIPH_BASE + 0x0C00UL)
#define GPIOE_BASE           (AHB2PERIPH_BASE + 0x1000UL)
#define GPIOF_BASE           (AHB2PERIPH_BASE + 0x1400UL)

/* --- PWR --- */
#define PWR_BASE             (APB1PERIPH_BASE + 0x7000UL)

/* ========================================================================== */
/* Register map structures (typedef'd for pointer access)                      */
/* ========================================================================== */

typedef struct {
    volatile uint32_t MODER;      /* Port mode register */
    volatile uint32_t OTYPER;     /* Output type register */
    volatile uint32_t OSPEEDR;    /* Output speed register */
    volatile uint32_t PUPDR;      /* Pull-up/pull-down register */
    volatile uint32_t IDR;        /* Input data register */
    volatile uint32_t ODR;        /* Output data register */
    volatile uint32_t BSRR;       /* Bit set/reset register */
    volatile uint32_t LCKR;       /* Lock register */
    volatile uint32_t AFRL;       /* Alternate function low */
    volatile uint32_t AFRH;       /* Alternate function high */
    volatile uint32_t BRR;        /* Bit reset register */
    uint32_t RESERVED0;
    volatile uint32_t SECCFGR;    /* Security configuration */
} GPIO_TypeDef;

typedef struct {
    volatile uint32_t CR1;        /* Control register 1 */
    volatile uint32_t CR2;        /* Control register 2 */
    volatile uint32_t SR;         /* Status register */
    volatile uint32_t DR;         /* Data register */
    volatile uint32_t CRCPR;      /* CRC polynomial */
    volatile uint32_t RXCRCR;     /* RX CRC */
    volatile uint32_t TXCRCR;     /* TX CRC */
    volatile uint32_t I2SCFGR;    /* I2S config (not used) */
} SPI_TypeDef;

typedef struct {
    volatile uint32_t CR1;        /* Control register 1 */
    volatile uint32_t CR2;        /* Control register 2 */
    volatile uint32_t CR3;        /* Control register 3 */
    volatile uint32_t BRR;        /* Baud rate register */
    volatile uint32_t GTPR;       /* Guard time / prescaler */
    volatile uint32_t RTOR;       /* Receiver timeout */
    volatile uint32_t RQR;        /* Request register */
    volatile uint32_t ISR;        /* Interrupt status */
    volatile uint32_t ICR;        /* Interrupt flag clear */
    volatile uint32_t RDR;        /* Receive data */
    volatile uint32_t TDR;        /* Transmit data */
} USART_TypeDef;

typedef struct {
    volatile uint32_t CR1;        /* Control register 1 */
    volatile uint32_t CR2;        /* Control register 2 */
    volatile uint32_t OAR1;       /* Own address 1 */
    volatile uint32_t OAR2;       /* Own address 2 */
    volatile uint32_t TIMINGR;    /* Timing register */
    volatile uint32_t TIMEOUTR;   /* Timeout register */
    volatile uint32_t ISR;        /* Interrupt status */
    volatile uint32_t ICR;        /* Interrupt flag clear */
    volatile uint32_t PECR;       /* PEC register */
    volatile uint32_t RXDR;      /* Receive data */
    volatile uint32_t TXDR;      /* Transmit data */
} I2C_TypeDef;

typedef struct {
    volatile uint32_t ISR;        /* Interrupt status */
    volatile uint32_t ICR;        /* Interrupt flag clear */
    volatile uint32_t FCSR;       /* Frame control status */
    volatile uint32_t CR;         /* Control */
    volatile uint32_t CFGR1;      /* Configuration 1 */
    volatile uint32_t CFGR2;      /* Configuration 2 */
    volatile uint32_t SMCR;       /* Slave mode control */
    volatile uint32_t DIER;       /* DMA/interrupt enable */
    volatile uint32_t CNT;        /* Counter */
    volatile uint32_t PSC;        /* Prescaler */
    volatile uint32_t ARR;       /* Auto-reload */
    volatile uint32_t RCNT;       /* Repetition counter (not on all TIMs) */
} TIM_TypeDef;

typedef struct {
    volatile uint32_t DR;         /* Data register */
    volatile uint32_t IDR;        /* Independent data */
    volatile uint32_t CR;         /* Control register */
    uint32_t RESERVED0;
    volatile uint32_t INIT;       /* Initial CRC value */
    volatile uint32_t POL;        /* CRC polynomial */
} CRC_TypeDef;

typedef struct {
    volatile uint32_t CR;         /* Control register */
    volatile uint32_t CFGR;       /* Configuration register */
    volatile uint32_t CIR;        /* Clock interrupt register */
    volatile uint32_t AHBRSTR;    /* AHB reset */
    volatile uint32_t APB1RSTR;   /* APB1 reset */
    volatile uint32_t APB2RSTR;   /* APB2 reset */
    volatile uint32_t AHBENR;     /* AHB enable */
    volatile uint32_t APB1ENR;    /* APB1 enable */
    volatile uint32_t APB2ENR;    /* APB2 enable */
    volatile uint32_t AHBLPENR;   /* AHB low-power enable */
    volatile uint32_t APB1LPENR;  /* APB1 low-power enable */
    volatile uint32_t APB2LPENR;  /* APB2 low-power enable */
} RCC_TypeDef;

#define RCC_BASE              (0x40021000UL)
#define RCC                   ((RCC_TypeDef *)RCC_BASE)

typedef struct {
    volatile uint32_t ISR;        /* Interrupt status register */
    volatile uint32_t ICR;        /* Interrupt flag clear register */
    volatile uint32_t IPR1;       /* Interrupt priority register 1 */
    volatile uint32_t IPR2;       /* Interrupt priority register 2 */
    volatile uint32_t IPR3;       /* Interrupt priority register 3 */
} NVIC_TypeDef;

#define NVIC_BASE             (0xE000E100UL)
#define NVIC                  ((NVIC_TypeDef *)NVIC_BASE)

typedef struct {
    volatile uint32_t STIR;       /* Software trigger interrupt register */
} NVIC_STIR_TypeDef;

#define SCB_BASE              (0xE000ED00UL)

typedef struct {
    volatile uint32_t CPUID;
    volatile uint32_t ICSR;       /* Interrupt control/state */
    volatile uint32_t RES1;
    volatile uint32_t AIRCR;      /* Application interrupt/reset control */
    volatile uint32_t SCR;        /* System control */
    volatile uint32_t CCR;        /* Config control */
    volatile uint32_t SHCSR;      /* System handler control */
} SCB_TypeDef;

#define SCB                   ((SCB_TypeDef *)SCB_BASE)

typedef struct {
    volatile uint32_t DR;         /* Data register */
    volatile uint32_t RDR;        /* Receive data register */
    volatile uint32_t SR;         /* Status register */
    volatile uint32_t CR1;        /* Control register 1 */
    volatile uint32_t CR2;        /* Control register 2 */
    volatile uint32_t CR3;        /* Control register 3 */
} USB_TypeDef;

#define USB                   ((USB_TypeDef *)USB_BASE)

typedef struct {
    volatile uint32_t MEMRMP;     /* Memory remap */
    volatile uint32_t CFGR1;      /* Configuration register 1 */
    volatile uint32_t EXTICR[4];   /* External interrupt config */
    volatile uint32_t SCSR;        /* Security control */
    volatile uint32_t CFGR2;       /* Configuration register 2 */
    volatile uint32_t COMP1_CTRL;  /* Comparator 1 control */
    volatile uint32_t COMP2_CTRL;  /* Comparator 2 control */
    volatile uint32_t COMP3_CTRL;  /* Comparator 3 control */
    volatile uint32_t COMP4_CTRL;  /* Comparator 4 control */
    volatile uint32_t COMP5_CTRL;  /* Comparator 5 control */
    volatile uint32_t COMP6_CTRL;  /* Comparator 6 control */
    volatile uint32_t COMP7_CTRL;  /* Comparator 7 control */
    uint32_t RESERVED0[2];
    volatile uint32_t OPERATION;   /* Operation register */
} SYSCFG_TypeDef;

#define SYSCFG                ((SYSCFG_TypeDef *)SYSCFG_BASE)

typedef struct {
    volatile uint32_t IMR1;       /* Interrupt mask register 1 */
    volatile uint32_t EMR1;       /* Event mask register 1 */
    volatile uint32_t RTSR1;      /* Rising trigger selection 1 */
    volatile uint32_t FTSR1;      /* Falling trigger selection 1 */
    volatile uint32_t SWIER1;     /* Software interrupt event 1 */
    volatile uint32_t PR1;        /* Pending register 1 */
    volatile uint32_t IMR2;       /* Interrupt mask register 2 */
    volatile uint32_t EMR2;       /* Event mask register 2 */
    volatile uint32_t RTSR2;      /* Rising trigger selection 2 */
    volatile uint32_t FTSR2;      /* Falling trigger selection 2 */
    volatile uint32_t SWIER2;    /* Software interrupt event 2 */
    volatile uint32_t PR2;        /* Pending register 2 */
} EXTI_TypeDef;

#define EXTI                  ((EXTI_TypeDef *)EXTI_BASE)

typedef struct {
    volatile uint32_t CR1;        /* ADC control register 1 */
    volatile uint32_t CR2;        /* ADC control register 2 */
    volatile uint32_t ISR;        /* Interrupt status register */
    volatile uint32_t IER;        /* Interrupt enable register */
    volatile uint32_t SMPR1;      /* Sample time register 1 */
    volatile uint32_t SMPR2;      /* Sample time register 2 */
    uint32_t RESERVED0;
    volatile uint32_t TR1;        /* Watchdog threshold 1 */
    volatile uint32_t TR2;        /* Watchdog threshold 2 */
    volatile uint32_t TR3;        /* Watchdog threshold 3 */
    uint32_t RESERVED1;
    volatile uint32_t SQR1;       /* Regular sequence register 1 */
    volatile uint32_t SQR2;       /* Regular sequence register 2 */
    volatile uint32_t SQR3;       /* Regular sequence register 3 */
    volatile uint32_t SQR4;       /* Regular sequence register 4 */
    volatile uint32_t DR;         /* Data register */
    uint32_t RESERVED2;
    volatile uint32_t AWD2CR;     /* Analog watchdog 2 config */
    volatile uint32_t AWD3CR;     /* Analog watchdog 3 config */
    uint32_t RESERVED3[2];
    volatile uint32_t JSQR;       /* Injected sequence register */
    uint32_t RESERVED4[4];
    volatile uint32_t OFR1;       /* Offset register 1 */
    volatile uint32_t OFR2;       /* Offset register 2 */
    volatile uint32_t OFR3;       /* Offset register 3 */
    volatile uint32_t OFR4;       /* Offset register 4 */
    uint32_t RESERVED5[4];
    volatile uint32_t JDR1;       /* Injected data 1 */
    volatile uint32_t JDR2;       /* Injected data 2 */
    volatile uint32_t JDR3;       /* Injected data 3 */
    volatile uint32_t JDR4;       /* Injected data 4 */
} ADC_TypeDef;

#define ADC1                  ((ADC_TypeDef *)ADC1_BASE)

/* ========================================================================== */
/* DMA register map                                                            */
/* ========================================================================== */
typedef struct {
    volatile uint32_t ISR;        /* Interrupt status */
    volatile uint32_t IFCR;       /* Interrupt flag clear */
} DMA_Common_TypeDef;

typedef struct {
    volatile uint32_t CCR;        /* Channel config */
    volatile uint32_t CNDTR;     /* Number of data */
    volatile uint32_t CPAR;      /* Peripheral address */
    volatile uint32_t CMAR;      /* Memory address */
} DMA_Channel_TypeDef;

#define DMA1                  ((DMA_Common_TypeDef *)DMA1_BASE)
#define DMA1_CH1              ((DMA_Channel_TypeDef *)(DMA1_BASE + 0x08UL))
#define DMA1_CH2              ((DMA_Channel_TypeDef *)(DMA1_BASE + 0x1CUL))
#define DMA1_CH3              ((DMA_Channel_TypeDef *)(DMA1_BASE + 0x30UL))
#define DMA1_CH4              ((DMA_Channel_TypeDef *)(DMA1_BASE + 0x44UL))
#define DMA1_CH5              ((DMA_Channel_TypeDef *)(DMA1_BASE + 0x58UL))
#define DMA1_CH6              ((DMA_Channel_TypeDef *)(DMA1_BASE + 0x6CUL))
#define DMA1_CH7              ((DMA_Channel_TypeDef *)(DMA1_BASE + 0x80UL))
#define DMA1_CH8              ((DMA_Channel_TypeDef *)(DMA1_BASE + 0x94UL))

/* ========================================================================== */
/* RCC bit definitions (key registers)                                         */
/* ========================================================================== */
#define RCC_CR_HSION          (1U << 0)
#define RCC_CR_HSERDY         (1U << 17)
#define RCC_CR_HSEON          (1U << 16)
#define RCC_CR_PLLON          (1U << 24)
#define RCC_CR_PLLRDY         (1U << 25)
#define RCC_CR_HSIRDY         (1U << 1)

#define RCC_CFGR_SW_HSI       (0U << 0)
#define RCC_CFGR_SW_PLL       (2U << 0)
#define RCC_CFGR_SWS          (3U << 2)
#define RCC_CFGR_SWS_PLL      (2U << 2)

#define RCC_PLLCFGR_PLLSRC_HSE (2U << 0)
#define RCC_PLLCFGR_PLLM_Pos  4
#define RCC_PLLCFGR_PLLN_Pos  8
#define RCC_PLLCFGR_PLLP_Pos  21

#define RCC_AHB2ENR_GPIOAEN   (1U << 0)
#define RCC_AHB2ENR_GPIOBEN   (1U << 1)
#define RCC_AHB2ENR_GPIOCEN   (1U << 2)
#define RCC_AHB2ENR_GPIODEN   (1U << 3)
#define RCC_AHB2ENR_ADC12EN   (1U << 13)

#define RCC_APB1ENR1_USART1EN (1U << 14)
#define RCC_APB1ENR1_SPI2EN   (1U << 15)
#define RCC_APB1ENR1_I2C1EN   (1U << 21)
#define RCC_APB1ENR1_TIM2EN   (1U << 0)
#define RCC_APB1ENR1_TIM3EN   (1U << 1)
#define RCC_APB1ENR1_TIM6EN   (1U << 4)

#define RCC_APB1ENR2_USBEN    (1U << 13)

#define RCC_APB2ENR_SPI1EN    (1U << 12)
#define RCC_APB2ENR_SYSCFGEN  (1U << 0)
#define RCC_APB2ENR_ADCEN     (1U << 13)

#define RCC_AHB1ENR_DMA1EN    (1U << 0)
#define RCC_AHB1ENR_DMA2EN    (1U << 1)
#define RCC_AHB1ENR_CRCEN     (1U << 12)

/* ========================================================================== */
/* FLASH registers                                                             */
/* ========================================================================== */
#define FLASH_BASE            (0x40022000UL)
#define FLASH                 ((FLASH_TypeDef *)FLASH_BASE)

typedef struct {
    volatile uint32_t ACR;        /* Access control register */
    volatile uint32_t PDKEYR;    /* Power-down key */
    volatile uint32_t KEYR;       /* Key register */
    volatile uint32_t OPTKEYR;    /* Option key */
    volatile uint32_t SR;         /* Status register */
    volatile uint32_t CR;         /* Control register */
    volatile uint32_t ECCR;       /* ECC and error correction */
    volatile uint32_t RESERVED0;
    volatile uint32_t OPTR;       /* Option register */
    volatile uint32_t PCROP1SR;   /* PCROP start */
    volatile uint32_t PCROP1ER;   /* PCROP end */
    volatile uint32_t WRP1AR;    /* Write protection 1A */
    volatile uint32_t WRP1BR;    /* Write protection 1B */
} FLASH_TypeDef;

#define FLASH_ACR_LATENCY_Pos  0
#define FLASH_ACR_DCEN         (1U << 3)
#define FLASH_ACR_ICEN         (1U << 4)
#define FLASH_ACR_PRFTEN       (1U << 8)

/* ========================================================================== */
/* USART bit definitions                                                       */
/* ========================================================================== */
#define USART_CR1_UE           (1U << 0)
#define USART_CR1_TE           (1U << 3)
#define USART_CR1_RE           (1U << 2)
#define USART_CR1_RXNEIE       (1U << 5)
#define USART_CR1_IDLEIE       (1U << 4)
#define USART_ISR_TXE          (1U << 7)
#define USART_ISR_TC           (1U << 6)
#define USART_ISR_RXNE         (1U << 5)
#define USART_ISR_IDLE         (1U << 4)
#define USART_ISR_ORE          (1U << 3)
#define USART_ICR_ORECF        (1U << 3)

/* ========================================================================== */
/* SPI bit definitions                                                         */
/* ========================================================================== */
#define SPI_CR1_SPE            (1U << 6)
#define SPI_CR1_MSTR           (1U << 2)
#define SPI_CR1_BR_Pos         3
#define SPI_CR1_CPHA           (1U << 0)
#define SPI_CR1_CPOL           (1U << 1)
#define SPI_CR1_SSI            (1U << 8)
#define SPI_CR1_SSM            (1U << 9)
#define SPI_CR2_RXNEIE         (1U << 6)
#define SPI_CR2_TXEIE          (1U << 7)
#define SPI_SR_TXE             (1U << 1)
#define SPI_SR_RXNE            (1U << 0)
#define SPI_SR_BSY             (1U << 7)
#define SPI_CR2_DS_Pos         8
#define SPI_CR2_FRXTH          (1U << 2)

/* ========================================================================== */
/* I2C bit definitions                                                         */
/* ========================================================================== */
#define I2C_CR1_PE             (1U << 0)
#define I2C_CR2_START          (1U << 13)
#define I2C_CR2_STOP           (1U << 14)
#define I2C_CR2_NACK           (1U << 15)
#define I2C_CR2_RD_WRN         (1U << 10)
#define I2C_ISR_TXE            (1U << 0)
#define I2C_ISR_RXNE           (1U << 2)
#define I2C_ISR_TC             (1U << 6)
#define I2C_ISR_STOPF          (1U << 5)
#define I2C_ISR_NACKF          (1U << 4)
#define I2C_ISR_BERR           (1U << 8)
#define I2C_ISR_ARLO           (1U << 9)

/* ========================================================================== */
/* CRC register (used for RTCM CRC-24Q)                                       */
/* ========================================================================== */
#define CRC_CR_RESET           (1U << 0)
#define CRC_CR_POLYSIZE_Pos    3
#define CRC_POL_POL32          0x04C11DB7UL   /* CRC-32 polynomial */

#endif /* REGISTERS_H */