/*
 * registers.h — STM32H743 MMIO register map for Vortex-SDR
 * Direct register access without HAL dependency
 */

#ifndef VORTEX_SDR_REGISTERS_H
#define VORTEX_SDR_REGISTERS_H

#include <stdint.h>

/* ========================================================================== */
/* Memory map                                                                 */
/* ========================================================================== */
#define PERIPH_BASE         0x40000000UL
#define D1_AHB1PERIPH_BASE  0x58020000UL
#define D1_AHB2PERIPH_BASE  0x58020400UL
#define D1_AHB4PERIPH_BASE  0x58020000UL
#define D2_APB1PERIPH_BASE  0x40003000UL
#define D2_APB2PERIPH_BASE  0x40012000UL
#define D2_AHB1PERIPH_BASE  0x40020000UL
#define D2_AHB2PERIPH_BASE  0x40022000UL

/* ========================================================================== */
/* GPIO registers (each port: A, B, C, D, E)                                  */
/* ========================================================================== */
typedef struct {
    volatile uint32_t MODER;      /* Mode register */
    volatile uint32_t OTYPER;     /* Output type register */
    volatile uint32_t OSPEEDR;   /* Output speed register */
    volatile uint32_t PUPDR;      /* Pull-up/pull-down register */
    volatile uint32_t IDR;        /* Input data register */
    volatile uint32_t ODR;        /* Output data register */
    volatile uint32_t BSRR;       /* Bit set/reset register */
    volatile uint32_t LCKR;       /* Lock register */
    volatile uint32_t AFR[2];     /* Alternate function registers */
    volatile uint32_t BRR;        /* Bit reset register */
    volatile uint32_t ASCR;       /* Analog switch control register */
} GPIO_TypeDef;

#define GPIOA    ((GPIO_TypeDef *) 0x58020000UL)
#define GPIOB    ((GPIO_TypeDef *) 0x58020400UL)
#define GPIOC    ((GPIO_TypeDef *) 0x58020800UL)
#define GPIOD    ((GPIO_TypeDef *) 0x58020C00UL)
#define GPIOE    ((GPIO_TypeDef *) 0x58021000UL)

/* GPIO MODER bits */
#define GPIO_MODE_INPUT       0x00UL
#define GPIO_MODE_OUTPUT      0x01UL
#define GPIO_MODE_AF          0x02UL
#define GPIO_MODE_ANALOG      0x03UL

#define GPIO_MODE_SHIFT(pin)  ((pin) * 2)
#define GPIO_MODE_MASK(pin)   (0x03UL << GPIO_MODE_SHIFT(pin))

/* GPIO OSPEEDR bits */
#define GPIO_SPEED_LOW        0x00UL
#define GPIO_SPEED_MEDIUM     0x01UL
#define GPIO_SPEED_HIGH       0x02UL
#define GPIO_SPEED_VERY_HIGH  0x03UL

/* GPIO PUPDR bits */
#define GPIO_PULL_NO          0x00UL
#define GPIO_PULL_UP          0x01UL
#define GPIO_PULL_DOWN        0x02UL

/* GPIO AF numbers */
#define GPIO_AF0_MCO          0x00UL
#define GPIO_AF4_I2C          0x04UL
#define GPIO_AF5_SPI           0x05UL
#define GPIO_AF6_SPI3          0x06UL
#define GPIO_AF7_USART         0x07UL
#define GPIO_AF8_UART          0x08UL
#define GPIO_AF10_USB          0x0AUL

/* ========================================================================== */
/* RCC (Reset and Clock Control) registers                                    */
/* ========================================================================== */
typedef struct {
    volatile uint32_t CR;          /* Clock control register */
    volatile uint32_t HSICFGR;   /* HSI configuration */
    volatile uint32_t CRRC;       /* Clock recovery RC */
    volatile uint32_t CSICFGR;   /* CSI configuration */
    volatile uint32_t CFGR;       /* Clock configuration */
    volatile uint32_t CDCCFG;    /* Clock distribution config */
    volatile uint32_t RESERVED0;  /* Reserved */
    volatile uint32_t PLLCKSELR;  /* PLL clock selection */
    volatile uint32_t PLLCFGR;    /* PLL configuration */
    volatile uint32_t PLL1DIVR;   /* PLL1 dividers */
    volatile uint32_t PLL1FRACR;  /* PLL1 fractional */
    volatile uint32_t PLL2DIVR;   /* PLL2 dividers */
    volatile uint32_t PLL2FRACR;  /* PLL2 fractional */
    volatile uint32_t PLL3DIVR;   /* PLL3 dividers */
    volatile uint32_t PLL3FRACR;  /* PLL3 fractional */
    volatile uint32_t RESERVED1;  /* Reserved */
    volatile uint32_t D1CFGR;     /* Domain 1 config */
    volatile uint32_t D2CFGR;     /* Domain 2 config */
    volatile uint32_t D3CFGR;     /* Domain 3 config */
} RCC_TypeDef;

#define RCC    ((RCC_TypeDef *) 0x58024400UL)

/* RCC CR bits */
#define RCC_CR_HSEON       (1UL << 16)
#define RCC_CR_HSERDY      (1UL << 17)
#define RCC_CR_PLL1ON       (1UL << 24)
#define RCC_CR_PLL1RDY      (1UL << 25)
#define RCC_CR_PLL2ON       (1UL << 26)
#define RCC_CR_PLL2RDY      (1UL << 27)
#define RCC_CR_PLL3ON       (1UL << 28)
#define RCC_CR_PLL3RDY      (1UL << 29)

/* RCC CRRC bits */
#define RCC_CRRC_HSI48ON    (1UL << 0)
#define RCC_CRRC_HSI48RDY   (1UL << 1)

/* RCC PLLCKSELR bits */
#define RCC_PLLCKSELR_PLLSRC_HSE   (2UL << 0)
#define RCC_PLLCKSELR_DIVM1_Pos    4
#define RCC_PLLCKSELR_DIVM2_Pos    8
#define RCC_PLLCKSELR_DIVM3_Pos    12

/* RCC CFGR bits */
#define RCC_CFGR_SW_PLL     (3UL << 0)
#define RCC_CFGR_SWS_PLL   (3UL << 2)

/* RCC D1CFGR bits */
#define RCC_D1CFGR_HPRE_Pos    0
#define RCC_D1CFGR_D1PPRE_Pos   4

/* RCC D2CFGR bits */
#define RCC_D2CFGR_D2PPRE1_Pos  4
#define RCC_D2CFGR_D2PPRE2_Pos  8

/* RCC AHB4ENR bits */
#define RCC_AHB4ENR_GPIOAEN   (1UL << 0)
#define RCC_AHB4ENR_GPIOBEN   (1UL << 1)
#define RCC_AHB4ENR_GPIOCEN   (1UL << 2)
#define RCC_AHB4ENR_GPIODEN   (1UL << 3)
#define RCC_AHB4ENR_GPIOEEN   (1UL << 4)

/* RCC APB1LENR bits */
#define RCC_APB1LENR_USART2EN  (1UL << 17)
#define RCC_APB1LENR_SPI3EN    (1UL << 15)

/* RCC APB2ENR bits */
#define RCC_APB2ENR_USART1EN   (1UL << 4)
#define RCC_APB2ENR_SPI1EN    (1UL << 12)
#define RCC_APB2ENR_SPI4EN    (1UL << 15)
#define RCC_APB2ENR_TIM1EN    (1UL << 0)

/* RCC AHB1ENR bits */
#define RCC_AHB1ENR_DMA1EN    (1UL << 0)
#define RCC_AHB1ENR_DMA2EN    (1UL << 1)

/* RCC BDCR bits */
#define RCC_BDCR_LSEON       (1UL << 0)
#define RCC_BDCR_LSERDY      (1UL << 1)

/* ========================================================================== */
/* PWR (Power) registers                                                      */
/* ========================================================================== */
typedef struct {
    volatile uint32_t CR1;       /* Power control 1 */
    volatile uint32_t CR2;       /* Power control 2 */
    volatile uint32_t CR3;       /* Power control 3 */
    volatile uint32_t CR4;       /* Power control 4 */
    volatile uint32_t SR1;       /* Status 1 */
    volatile uint32_t SR2;       /* Status 2 */
    volatile uint32_t D3CR;      /* Domain 3 control */
} PWR_TypeDef;

#define PWR    ((PWR_TypeDef *) 0x58024800UL)

#define PWR_D3CR_VOS_Pos       14
#define PWR_D3CR_VOS_Msk        (3UL << PWR_D3CR_VOS_Pos)
#define PWR_D3CR_VOSRDY         (1UL << 13)

/* ========================================================================== */
/* FLASH registers                                                            */
/* ========================================================================== */
typedef struct {
    volatile uint32_t ACR;       /* Access control */
    volatile uint32_t RESERVED[4];
    volatile uint32_t KEYR;      /* Key register */
} FLASH_TypeDef;

#define FLASH   ((FLASH_TypeDef *) 0x52002000UL)

#define FLASH_ACR_LATENCY_Pos   0
#define FLASH_ACR_PRFTEN        (1UL << 8)
#define FLASH_ACR_ICEN          (1UL << 9)
#define FLASH_ACR_DCEN          (1UL << 10)

/* ========================================================================== */
/* SPI registers                                                              */
/* ========================================================================== */
typedef struct {
    volatile uint32_t CR1;       /* Control 1 */
    volatile uint32_t CR2;       /* Control 2 */
    volatile uint32_t SR;        /* Status */
    volatile uint32_t DR;        /* Data */
    volatile uint32_t CRCPR;     /* CRC polynomial */
    volatile uint32_t RXCRCR;    /* RX CRC */
    volatile uint32_t TXCRCR;    /* TX CRC */
    volatile uint32_t I2SCFGR;   /* I2S config */
} SPI_TypeDef;

#define SPI1    ((SPI_TypeDef *) 0x40013000UL)
#define SPI2    ((SPI_TypeDef *) 0x40003800UL)
#define SPI3    ((SPI_TypeDef *) 0x40003C00UL)

/* SPI CR1 bits */
#define SPI_CR1_CPHA        (1UL << 0)
#define SPI_CR1_CPOL        (1UL << 1)
#define SPI_CR1_MSTR        (1UL << 2)
#define SPI_CR1_BR_Pos      3
#define SPI_CR1_BR_Msk      (7UL << SPI_CR1_BR_Pos)
#define SPI_CR1_SPE         (1UL << 6)
#define SPI_CR1_LSBFIRST    (1UL << 7)
#define SPI_CR1_SSI         (1UL << 8)
#define SPI_CR1_SSM         (1UL << 9)
#define SPI_CR1_RXONLY     (1UL << 10)
#define SPI_CR1_CRCL_Pos   13
#define SPI_CR1_CRCNEXT    (1UL << 12)
#define SPI_CR1_CRCE       (1UL << 11)

/* SPI CR2 bits */
#define SPI_CR2_RXDMAEN    (1UL << 0)
#define SPI_CR2_TXDMAEN    (1UL << 1)
#define SPI_CR2_DS_Pos     8
#define SPI_CR2_DS_Msk     (0xFUL << SPI_CR2_DS_Pos)
#define SPI_CR2_FRF        (1UL << 4)
#define SPI_CR2_NSSP       (1UL << 3)
#define SPI_CR2_SSOE       (1UL << 2)

/* SPI SR bits */
#define SPI_SR_RXNE        (1UL << 0)
#define SPI_SR_TXE         (1UL << 1)
#define SPI_SR_CRCERR      (1UL << 4)
#define SPI_SR_MODF        (1UL << 5)
#define SPI_SR_OVR         (1UL << 6)
#define SPI_SR_BSY         (1UL << 7)
#define SPI_SR_FRE         (1UL << 8)

/* ========================================================================== */
/* USART registers                                                            */
/* ========================================================================== */
typedef struct {
    volatile uint32_t CR1;       /* Control 1 */
    volatile uint32_t CR2;       /* Control 2 */
    volatile uint32_t CR3;       /* Control 3 */
    volatile uint32_t BRR;       /* Baud rate */
    volatile uint32_t GTPR;       /* Guard time/prescaler */
    volatile uint32_t RTOR;      /* Receiver timeout */
    volatile uint32_t RQR;        /* Request */
    volatile uint32_t ISR;        /* Interrupt/status */
    volatile uint32_t ICR;        /* Interrupt flag clear */
    volatile uint32_t RDR;        /* Receive data */
    volatile uint32_t TDR;        /* Transmit data */
} USART_TypeDef;

#define USART1   ((USART_TypeDef *) 0x40011000UL)
#define USART2   ((USART_TypeDef *) 0x40004400UL)

/* USART CR1 bits */
#define USART_CR1_UE         (1UL << 0)
#define USART_CR1_RE          (1UL << 2)
#define USART_CR1_TE          (1UL << 3)
#define USART_CR1_RXNEIE     (1UL << 5)
#define USART_CR1_TCIE       (1UL << 6)
#define USART_CR1_TXEIE      (1UL << 7)
#define USART_CR1_OVER8      (1UL << 15)
#define USART_CR1_M_Pos      12

/* USART ISR bits */
#define USART_ISR_RXNE       (1UL << 5)
#define USART_ISR_TXE        (1UL << 7)
#define USART_ISR_TC         (1UL << 6)
#define USART_ISR_ORE        (1UL << 3)
#define USART_ISR_FE         (1UL << 1)
#define USART_ISR_NF         (1UL << 2)

/* ========================================================================== */
/* I2C registers                                                              */
/* ========================================================================== */
typedef struct {
    volatile uint32_t CR1;       /* Control 1 */
    volatile uint32_t CR2;       /* Control 2 */
    volatile uint32_t OAR1;      /* Own address 1 */
    volatile uint32_t OAR2;      /* Own address 2 */
    volatile uint32_t TIMINGR;   /* Timing */
    volatile uint32_t TIMEOUTR;   /* Timeout */
    volatile uint32_t ISR;        /* Interrupt/status */
    volatile uint32_t ICR;        /* Interrupt flag clear */
    volatile uint32_t PECR;       /* PEC */
    volatile uint32_t RXDR;       /* Receive data */
    volatile uint32_t TXDR;       /* Transmit data */
} I2C_TypeDef;

#define I2C1     ((I2C_TypeDef *) 0x40005400UL)

/* ========================================================================== */
/* DMA registers                                                              */
/* ========================================================================== */
typedef struct {
    volatile uint32_t CR;        /* Control */
    volatile uint32_t NDTR;      /* Number of data */
    volatile uint32_t PAR;       /* Peripheral address */
    volatile uint32_t M0AR;      /* Memory address 0 */
    volatile uint32_t M1AR;      /* Memory address 1 */
    volatile uint32_t FCR;       /* FIFO control */
} DMA_Stream_TypeDef;

typedef struct {
    volatile uint32_t LISR;      /* Low interrupt status */
    volatile uint32_t HISR;      /* High interrupt status */
    volatile uint32_t RESERVED[2];
    DMA_Stream_TypeDef S[8];      /* Stream 0-7 */
} DMA_TypeDef;

#define DMA1    ((DMA_TypeDef *) 0x40020000UL)
#define DMA2    ((DMA_TypeDef *) 0x40020400UL)

/* DMA Stream CR bits */
#define DMA_SxCR_CHSEL_Pos    0
#define DMA_SxCR_CHSEL_Msk    (7UL << DMA_SxCR_CHSEL_Pos)
#define DMA_SxCR_DIR_Pos      6
#define DMA_SxCR_CIRC         (1UL << 8)
#define DMA_SxCR_MINC         (1UL << 10)
#define DMA_SxCR_PINC         (1UL << 9)
#define DMA_SxCR_PL_Pos       16
#define DMA_SxCR_MSIZE_Pos    13
#define DMA_SxCR_PSIZE_Pos    11
#define DMA_SxCR_HTIE         (1UL << 3)
#define DMA_SxCR_TCIE         (1UL << 4)
#define DMA_SxCR_EN           (1UL << 0)

/* ========================================================================== */
/* Timer registers                                                            */
/* ========================================================================== */
typedef struct {
    volatile uint32_t CR1;       /* Control 1 */
    volatile uint32_t CR2;       /* Control 2 */
    volatile uint32_t DIER;      /* DMA/interrupt enable */
    volatile uint32_t SR;         /* Status */
    volatile uint32_t CNT;        /* Counter */
    volatile uint32_t PSC;        /* Prescaler */
    volatile uint32_t ARR;        /* Auto-reload */
    volatile uint32_t RESERVED[2];
    volatile uint32_t CCR[4];     /* Capture/compare */
} TIM_TypeDef;

#define TIM1    ((TIM_TypeDef *) 0x40012C00UL)
#define TIM2    ((TIM_TypeDef *) 0x40000000UL)
#define TIM6    ((TIM_TypeDef *) 0x40001000UL)
#define TIM7    ((TIM_TypeDef *) 0x40001400UL)

/* Timer CR1 bits */
#define TIM_CR1_CEN          (1UL << 0)
#define TIM_CR1_UDIS         (1UL << 1)
#define TIM_CR1_URS          (1UL << 2)

/* Timer DIER bits */
#define TIM_DIER_UIE         (1UL << 0)
#define TIM_DIER_CC1IE       (1UL << 1)

/* Timer SR bits */
#define TIM_SR_UIF           (1UL << 0)

/* ========================================================================== */
/* ADC registers                                                              */
/* ========================================================================== */
typedef struct {
    volatile uint32_t ISR;       /* Interrupt status */
    volatile uint32_t IER;       /* Interrupt enable */
    volatile uint32_t CR;        /* Control */
    volatile uint32_t CFGR;      /* Configuration */
    volatile uint32_t CFGR2;     /* Configuration 2 */
    volatile uint32_t SMPR1;     /* Sample time 1 */
    volatile uint32_t SMPR2;     /* Sample time 2 */
    volatile uint32_t RESERVED[2];
    volatile uint32_t TR1;        /* Threshold 1 */
    volatile uint32_t TR2;        /* Threshold 2 */
    volatile uint32_t TR3;        /* Threshold 3 */
    volatile uint32_t RESERVED2;
    volatile uint32_t JOFR[4];   /* Injected offset */
    volatile uint32_t AWD2CR;    /* Analog watchdog 2 */
    volatile uint32_t AWD3CR;    /* Analog watchdog 3 */
    volatile uint32_t RESERVED3[2];
    volatile uint32_t DR;         /* Data register */
    volatile uint32_t JDR[4];   /* Injected data */
} ADC_TypeDef;

#define ADC1    ((ADC_TypeDef *) 0x40022000UL)

/* ADC ISR bits */
#define ADC_ISR_ADRDY    (1UL << 0)
#define ADC_ISR_EOC       (1UL << 2)

/* ADC CR bits */
#define ADC_CR_ADEN       (1UL << 0)
#define ADC_CR_ADSTART    (1UL << 2)
#define ADC_CR_ADSTOP     (1UL << 4)

/* ========================================================================== */
/* DAC registers                                                              */
/* ========================================================================== */
typedef struct {
    volatile uint32_t CR;        /* Control */
    volatile uint32_t SWTRIGR;   /* Software trigger */
    volatile uint32_t DHR12R1;   /* 12-bit right-aligned data ch1 */
    volatile uint32_t DHR12L1;   /* 12-bit left-aligned data ch1 */
    volatile uint32_t DHR8R1;    /* 8-bit right-aligned data ch1 */
    volatile uint32_t DHR12R2;   /* 12-bit right-aligned data ch2 */
    volatile uint32_t DHR12L2;   /* 12-bit left-aligned data ch2 */
    volatile uint32_t DHR8R2;    /* 8-bit right-aligned data ch2 */
    volatile uint32_t DHR12RD;   /* Dual 12-bit right-aligned */
    volatile uint32_t DHR12LD;   /* Dual 12-bit left-aligned */
    volatile uint32_t DHR8RD;    /* Dual 8-bit right-aligned */
    volatile uint32_t DOR1;      /* Data output ch1 */
    volatile uint32_t DOR2;      /* Data output ch2 */
    volatile uint32_t SR;         /* Status */
} DAC_TypeDef;

#define DAC1    ((DAC_TypeDef *) 0x40017000UL)

#define DAC_CR_EN1         (1UL << 0)
#define DAC_CR_BOFF1       (1UL << 1)

/* ========================================================================== */
/* EXTI registers                                                             */
/* ========================================================================== */
typedef struct {
    volatile uint32_t IMR1;      /* Interrupt mask 1 */
    volatile uint32_t EMR1;      /* Event mask 1 */
    volatile uint32_t RTSR1;     /* Rising trigger 1 */
    volatile uint32_t FTSR1;     /* Falling trigger 1 */
    volatile uint32_t SWIER1;    /* Software interrupt 1 */
    volatile uint32_t PR1;       /* Pending 1 */
} EXTI_TypeDef;

#define EXTI    ((EXTI_TypeDef *) 0x40000000UL)

/* ========================================================================== */
/* Independent watchdog                                                        */
/* ========================================================================== */
typedef struct {
    volatile uint32_t KR;        /* Key register */
    volatile uint32_t PR;        /* Prescaler */
    volatile uint32_t RLR;       /* Reload */
    volatile uint32_t SR;        /* Status */
    volatile uint32_t WINR;      /* Window */
} IWDG_TypeDef;

#define IWDG    ((IWDG_TypeDef *) 0x58004C00UL)

#define IWDG_KR_ENABLE      0xCCCCUL
#define IWDG_KR_WRITE       0x5555UL
#define IWDG_SR_PVU          (1UL << 0)
#define IWDG_SR_RVU          (1UL << 1)

/* ========================================================================== */
/* RNG registers                                                              */
/* ========================================================================== */
typedef struct {
    volatile uint32_t CR;        /* Control */
    volatile uint32_t SR;        /* Status */
    volatile uint32_t DR;        /* Data */
} RNG_TypeDef;

#define RNG     ((RNG_TypeDef *) 0x48060800UL)

#define RNG_CR_RNGEN    (1UL << 2)
#define RNG_SR_DRDY     (1UL << 0)

/* ========================================================================== */
/* USB OTG FS registers (simplified)                                           */
/* ========================================================================== */
typedef struct {
    volatile uint32_t GOTGCTL;    /* OTG control */
    volatile uint32_t GOTGINT;    /* OTG interrupt */
    volatile uint32_t GAHBCFG;    /* AHB config */
    volatile uint32_t GUSBCFG;    /* USB config */
    volatile uint32_t GRSTCTL;    /* Reset control */
    volatile uint32_t GINTSTS;    /* Interrupt status */
    volatile uint32_t GINTMSK;    /* Interrupt mask */
    volatile uint32_t GRXSTSR;    /* RX status read */
    volatile uint32_t GRXSTSP;    /* RX status pop */
    volatile uint32_t GRXFSIZ;    /* RX FIFO size */
    volatile uint32_t GNPTXFSIZ;  /* Non-periodic TX FIFO size */
    volatile uint32_t RESERVED[2];
    volatile uint32_t GCCFG;      /* General config */
    volatile uint32_t CID;        /* Core ID */
} USB_OTG_GlobalTypeDef;

typedef struct {
    volatile uint32_t DCFG;       /* Device config */
    volatile uint32_t DCTL;       /* Device control */
    volatile uint32_t DSTS;       /* Device status */
    volatile uint32_t RESERVED;
    volatile uint32_t DIEPMSK;    /* IN endpoint mask */
    volatile uint32_t DOEPMSK;    /* OUT endpoint mask */
    volatile uint32_t DAINT;      /* All endpoint interrupt */
    volatile uint32_t DAINTMSK;   /* All endpoint mask */
} USB_OTG_DeviceTypeDef;

#define USB_OTG_FS_GLOBAL    ((USB_OTG_GlobalTypeDef *) 0x40080000UL)
#define USB_OTG_FS_DEVICE    ((USB_OTG_DeviceTypeDef *) 0x40080800UL)

/* ========================================================================== */
/* CRC registers (for BLE protocol)                                           */
/* ========================================================================== */
typedef struct {
    volatile uint32_t DR;        /* Data register */
    volatile uint32_t IDR;       /* Independent data */
    volatile uint32_t CR;        /* Control */
    volatile uint32_t RESERVED;
    volatile uint32_t INIT;      /* Initial value */
    volatile uint32_t POL;       /* Polynomial */
} CRC_TypeDef;

#define CRC     ((CRC_TypeDef *) 0x58024C00UL)

#endif /* VORTEX_SDR_REGISTERS_H */