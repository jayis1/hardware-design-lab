/*
 * registers.h — CMSIS-style register map for the STM32H743 subsets used by
 * Stridophone. This deliberately does NOT include the full ST HAL (which is
 * enormous); it maps only the registers the firmware touches and provides
 * lightweight bit helpers. The aim is a self-contained, reviewable file.
 *
 * Author : jayis1
 * License: MIT
 */
#ifndef STRIDOPHONE_REGISTERS_H
#define STRIDOPHONE_REGISTERS_H

#include <stdint.h>

/* ---- Base addresses ---------------------------------------------- */
#define PERIPH_BASE        0x40000000UL
#define D1_AHB1PERI_BASE   (PERIPH_BASE + 0x12000000UL)
#define D1_APB1PERI_BASE   (PERIPH_BASE + 0x10000000UL)
#define D1_APB2PERI_BASE   (PERIPH_BASE + 0x04000000UL)
#define D3_AHB1PERI_BASE   (PERIPH_BASE + 0x18000000UL)
#define D3_APB1PERI_BASE   (PERIPH_BASE + 0x1C000000UL)
#define D3_APB2PERI_BASE   (PERIPH_BASE + 0x1D000000UL)

/* ---- RCC ---------------------------------------------------------- */
#define RCC_BASE           (D1_AHB1PERI_BASE + 0x4400UL)
typedef struct {
    volatile uint32_t CR;       /* 0x00 */
    volatile uint32_t HSICFGR;  /* 0x04 */
    volatile uint32_t CRRCR;    /* 0x08 */
    volatile uint32_t CSICFGR;  /* 0x0C */
    volatile uint32_t CFGR;     /* 0x10 */
    volatile uint32_t RCC_RESERVED1;
    volatile uint32_t D1CFGR;   /* 0x18 D1 domain: SYS/HPRE/D1PPRE */
    volatile uint32_t D2CFGR;   /* 0x1C D2 domain: D2PPRE1/D2PPRE2 */
    volatile uint32_t D3CFGR;   /* 0x20 D3 domain: D3PPRE */
    /* PLL1 config */
    volatile uint32_t PLLCKSELR;/* 0x28 */
    volatile uint32_t PLLCFGR;  /* 0x2C */
    volatile uint32_t PLL1FRACR;/* 0x30 */
    volatile uint32_t PLL1DIVR; /* 0x34 */
    volatile uint32_t PLL2FRACR;/* 0x38 */
    volatile uint32_t PLL2DIVR; /* 0x3C */
    volatile uint32_t PLL3FRACR;/* 0x40 */
    volatile uint32_t PLL3DIVR; /* 0x44 */
    /* Clock enables ... */
    volatile uint32_t D1CCIPR;  /* 0x4C */
    volatile uint32_t D2CCIP1R; /* 0x50 */
    volatile uint32_t D2CCIP2R; /* 0x54 */
    volatile uint32_t D3CCIPR;  /* 0x58 */
    volatile uint32_t CKGA_BRSTR;/*0x5C */
    volatile uint32_t AHB1RSTR; /* 0x60 */
    volatile uint32_t AHB2RSTR; /* 0x64 */
    volatile uint32_t AHB3RSTR; /* 0x68 */
    volatile uint32_t AHB4RSTR; /* 0x6C */
    volatile uint32_t APB1LRSTR;/* 0x70 */
    volatile uint32_t APB1HRSTR;/* 0x74 */
    volatile uint32_t APB2RSTR; /* 0x78 */
    volatile uint32_t APB3RSTR; /* 0x7C */
    volatile uint32_t APB4RSTR; /* 0x80 */
    volatile uint32_t AHB1ENR;  /* 0x88 (GPDMA1 clock gated separately) */
    volatile uint32_t AHB2ENR;  /* 0x8C */
    volatile uint32_t AHB3ENR;  /* 0x90 */
    volatile uint32_t AHB4ENR;  /* 0x94 */
    volatile uint32_t APB1LENR; /* 0x98 */
    volatile uint32_t APB1HENR; /* 0x9C */
    volatile uint32_t APB2ENR;  /* 0xA0 */
    volatile uint32_t APB3ENR;  /* 0xA4 */
    volatile uint32_t APB4ENR;  /* 0xA8 */
} RCC_t;
#define RCC ((RCC_t *)RCC_BASE)

/* RCC bit definitions we use */
#define RCC_CR_HSEON     (1U<<16)
#define RCC_CR_HSERDY    (1U<<17)
#define RCC_CR_HSECSSON  (1U<<18)
#define RCC_CR_PLL1ON    (1U<<24)
#define RCC_CR_PLL1RDY   (1U<<25)
#define RCC_CR_CSIDIV    (1U<<19)
#define RCC_CFGR_SW_PLL1 (3U<<0)
#define RCC_CFGR_SWS_PLL1 (3U<<3)

/* AHB4ENR GPIOx bits */
#define RCC_AHB4ENR_GPIOA  (1U<<0)
#define RCC_AHB4ENR_GPIOB  (1U<<1)
#define RCC_AHB4ENR_GPIOC  (1U<<2)
#define RCC_AHB4ENR_GPIOD  (1U<<3)
#define RCC_AHB4ENR_GPIOE  (1U<<4)

/* APB1LENR bits */
#define RCC_APB1LENR_USART3 (1U<<18)
#define RCC_APB1LENR_SPI2   (1U<<14)
/* APB4ENR bits */
#define RCC_APB4ENR_I2C4   (1U<<2)
#define RCC_APB4ENR_SDMMC1 (1U<<12)
/* APB2ENR bits */
#define RCC_APB2ENR_SAI1   (1U<<0)

/* ---- PWR ---------------------------------------------------------- */
#define PWR_BASE           (D1_APB1PERI_BASE + 0x7000UL)
typedef struct {
    volatile uint32_t CR1;   /* 0x00 */
    volatile uint32_t CSR1;  /* 0x04 */
    volatile uint32_t CR2;   /* 0x08 */
    volatile uint32_t CR3;   /* 0x0C */
    volatile uint32_t CPUCR; /* 0x10 */
} PWR_t;
#define PWR ((PWR_t *)PWR_BASE)
#define PWR_CR1_VOS_SCALE1 (3U<<15)

/* ---- GPIO (simplified) ------------------------------------------- */
#define GPIOA_BASE         (D1_AHB1PERI_BASE + 0x0000UL)
#define GPIOB_BASE         (D1_AHB1PERI_BASE + 0x0400UL)
#define GPIOC_BASE         (D1_AHB1PERI_BASE + 0x0800UL)
#define GPIOD_BASE         (D1_AHB1PERI_BASE + 0x0C00UL)
#define GPIOE_BASE         (D1_AHB1PERI_BASE + 0x1000UL)

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
    volatile uint32_t SECCFGR;  /* 0x30 */
} GPIO_t;
#define GPIOA ((GPIO_t *)GPIOA_BASE)
#define GPIOB ((GPIO_t *)GPIOB_BASE)
#define GPIOC ((GPIO_t *)GPIOC_BASE)
#define GPIOD ((GPIO_t *)GPIOD_BASE)
#define GPIOE ((GPIO_t *)GPIOE_BASE)

#define GPIO_MODE_IN    0
#define GPIO_MODE_OUT   1
#define GPIO_MODE_AF    2
#define GPIO_MODE_AN    3
#define GPIO_PUPD_NONE  0
#define GPIO_PUPD_PU    1
#define GPIO_PUPD_PD    2
#define GPIO_OSPEED_LOW 0
#define GPIO_OSPEED_HI  3

static inline void gpio_set_mode(GPIO_t *g, int pad, int mode) {
    g->MODER = (g->MODER & ~(3U<<(pad*2))) | ((uint32_t)mode<<(pad*2));
}
static inline void gpio_set_pupd(GPIO_t *g, int pad, int pupd) {
    g->PUPDR = (g->PUPDR & ~(3U<<(pad*2))) | ((uint32_t)pupd<<(pad*2));
}
static inline void gpio_set_af(GPIO_t *g, int pad, int af) {
    volatile uint32_t *r = (pad < 8) ? &g->AFRL : &g->AFRH;
    int sh = (pad & 7) * 4;
    *r = (*r & ~(15U<<sh)) | ((uint32_t)(af & 15)<<sh);
}
static inline void gpio_set_speed(GPIO_t *g, int pad, int spd) {
    g->OSPEEDR = (g->OSPEEDR & ~(3U<<(pad*2))) | ((uint32_t)spd<<(pad*2));
}
static inline void gpio_set(GPIO_t *g, int pad)    { g->BSRR = 1U<<pad; }
static inline void gpio_clr(GPIO_t *g, int pad)    { g->BSRR = 1U<<(pad+16); }
static inline int  gpio_read(GPIO_t *g, int pad)   { return (g->IDR>>pad)&1; }

/* ---- SAI1 (audio) ------------------------------------------------- */
#define SAI1_BASE          (D1_APB2PERI_BASE + 0x2000UL)
typedef struct {
    volatile uint32_t GCR;       /* 0x00 */
    volatile uint32_t PADDING[3];
    /* Block A */
    volatile uint32_t A_CR1;     /* 0x04 */
    volatile uint32_t A_CR2;     /* 0x08 */
    volatile uint32_t A_FRCR;    /* 0x0C */
    volatile uint32_t A_SLOTR;   /* 0x10 */
    volatile uint32_t A_IM;      /* 0x14 */
    volatile uint32_t A_SR;      /* 0x18 */
    volatile uint32_t A_CLRFR;   /* 0x1C */
    volatile uint32_t A_DR;      /* 0x20 (RX reads here) */
    /* Block B follows at +0x24 */
    volatile uint32_t B_CR1;
    volatile uint32_t B_CR2;
    volatile uint32_t B_FRCR;
    volatile uint32_t B_SLOTR;
    volatile uint32_t B_IM;
    volatile uint32_t B_SR;
    volatile uint32_t B_CLRFR;
    volatile uint32_t B_DR;
} SAI_t;
#define SAI1 ((SAI_t *)SAI1_BASE)

#define SAI_CR1_SAIEN     (1U<<0)
#define SAI_CR1_DMAEN     (1U<<17)
#define SAI_CR1_MCKDIV_Pos 20
#define SAI_SR_FREQ       (1U<<2)

/* ---- SPI2 --------------------------------------------------------- */
#define SPI2_BASE          (D1_APB1PERI_BASE + 0x3800UL)
typedef struct {
    volatile uint32_t CR1;     /* 0x00 */
    volatile uint32_t CR2;     /* 0x04 */
    volatile uint32_t CFG1;    /* 0x08 */
    volatile uint32_t CFG2;    /* 0x0C */
    volatile uint32_t IER;     /* 0x10 */
    volatile uint32_t SR;      /* 0x14 */
    volatile uint32_t IFCR;    /* 0x18 */
    volatile uint32_t TXDR;    /* 0x20 */
    volatile uint32_t RXDR;    /* 0x24 */
    volatile uint32_t I2SCFGR; /* 0x28 */
} SPI_t;
#define SPI2 ((SPI_t *)SPI2_BASE)
#define SPI_CFG1_MASTER    (1U<<22)
#define SPI_CFG1_TXDMAEN   (1U<<15)
#define SPI_CFG1_RXDMAEN   (1U<<14)
#define SPI_CR1_SPE        (1U<<0)
#define SPI_SR_TXP         (1U<<1)
#define SPI_SR_RXP         (1U<<0)
#define SPI_SR_EOT         (1U<<3)

/* ---- USART3 ------------------------------------------------------- */
#define USART3_BASE         (D1_APB1PERI_BASE + 0x4800UL)
typedef struct {
    volatile uint32_t CR1;    /* 0x00 */
    volatile uint32_t CR2;    /* 0x04 */
    volatile uint32_t CR3;    /* 0x08 */
    volatile uint32_t BRR;    /* 0x0C */
    volatile uint32_t GTDR;   /* 0x10 */
    volatile uint32_t RTOR;   /* 0x14 */
    volatile uint32_t RQR;    /* 0x18 */
    volatile uint32_t ISR;    /* 0x1C */
    volatile uint32_t ICR;    /* 0x20 */
    volatile uint32_t TDR;    /* 0x28 */
    volatile uint32_t RDR;    /* 0x2C */
} USART_t;
#define USART3 ((USART_t *)USART3_BASE)
#define USART_CR1_UE    (1U<<0)
#define USART_CR1_TE    (1U<<3)
#define USART_CR1_RE    (1U<<2)
#define USART_CR1_RXNEIE (1U<<5)
#define USART_ISR_TXE  (1U<<7)
#define USART_ISR_RXNE (1U<<5)

/* ---- I2C4 --------------------------------------------------------- */
#define I2C4_BASE          (D3_APB2PERI_BASE + 0x0000UL)
typedef struct {
    volatile uint32_t CR1;     /* 0x00 */
    volatile uint32_t CR2;     /* 0x04 */
    volatile uint32_t OAR1;    /* 0x08 */
    volatile uint32_t OAR2;    /* 0x0C */
    volatile uint32_t TIMINGR; /* 0x10 */
    volatile uint32_t TIMEOUTR;/* 0x14 */
    volatile uint32_t ISR;     /* 0x18 */
    volatile uint32_t ICR;     /* 0x1C */
    volatile uint32_t PECR;    /* 0x20 */
    volatile uint32_t RXDR;    /* 0x24 */
    volatile uint32_t TXDR;    /* 0x28 */
} I2C_t;
#define I2C4 ((I2C_t *)I2C4_BASE)
#define I2C_CR1_PE       (1U<<0)
#define I2C_CR2_START    (1U<<13)
#define I2C_CR2_STOP     (1U<<14)
#define I2C_CR2_NBYTES_Pos 16
#define I2C_ISR_TXE     (1U<<0)
#define I2C_ISR_RXNE    (1U<<2)
#define I2C_ISR_TC      (1U<<6)
#define I2C_ISR_NACKF   (1U<<12)

/* ---- BDMA (for SAI1 audio RX) ------------------------------------- */
#define BDMA_BASE          (D3_AHB1PERI_BASE + 0x1000UL)
typedef struct {
    volatile uint32_t ISR;      /* 0x00 */
    volatile uint32_t RESERVED0;
    volatile uint32_t IFCR;     /* 0x08 */
    volatile uint32_t RESERVED1;
    /* channel 0 config */
    volatile uint32_t C0PAR;    /* 0x10 */
    volatile uint32_t C0MAR;    /* 0x14 */
    volatile uint32_t C0NDTR;   /* 0x18 */
    volatile uint32_t C0CR;     /* 0x1C */
} BDMA_t;
#define BDMA ((BDMA_t *)BDMA_BASE)
#define BDMA_IFCR_CGIF0  (1U<<0)
#define BDMA_IFCR_CTCIF0 (1U<<1)
#define BDMA_IFCR_CHTIF0 (1U<<2)
#define BDMA_CR_EN       (1U<<0)
#define BDMA_CR_TCIE     (1U<<1)
#define BDMA_CR_HTIE     (1U<<2)
#define BDMA_CR_MINC     (1U<<10)
#define BDMA_CR_PINC     (1U<<9)
#define BDMA_CR_CIRC     (1U<<8)
#define BDMA_CR_P2M      (2U<<4)
#define BDMA_CR_PSIZE_32 (2U<<9)
#define BDMA_CR_MSIZE_32 (1U<<11)

/* ---- SysTick ------------------------------------------------------ */
#define SYSTICK_BASE       (0xE000E010UL)
typedef struct {
    volatile uint32_t CSR;
    volatile uint32_t LOAD;
    volatile uint32_t VAL;
    volatile uint32_t CALIB;
} SysTick_t;
#define SysTick ((SysTick_t *)SYSTICK_BASE)
#define SYSTICK_CSR_ENABLE (1U<<0)
#define SYSTICK_CSR_TICKINT (1U<<1)
#define SYSTICK_CSR_CLKSRC  (1U<<2)

/* ---- NVIC --------------------------------------------------------- */
#define NVIC_BASE          0xE000E100UL
#define NVIC_ISER0         (*(volatile uint32_t *)(NVIC_BASE+0x000))
#define NVIC_ICER0         (*(volatile uint32_t *)(NVIC_BASE+0x080))
#define NVIC_ISPR0         (*(volatile uint32_t *)(NVIC_BASE+0x100))
#define NVIC_ICPR0         (*(volatile uint32_t *)(NVIC_BASE+0x180))
#define NVIC_IPR_BASE      (NVIC_BASE + 0x300UL)

static inline void nvic_enable(int irq) {
    NVIC_ISER0 = 1U << (irq & 31);
}
static inline void nvic_clear(int irq) {
    NVIC_ICPR0 = 1U << (irq & 31);
}

/* IRQ numbers (H7) */
#define BDMA_CH0_IRQ       0   /* BDMA Channel 0 */
#define USART3_IRQ         41
#define SAI1_IRQ           85

/* ---- Flash ACR (latency) ----------------------------------------- */
#define FLASH_ACR          (*(volatile uint32_t *)(0x52002000UL + 0x00))
#define FLASH_ACR_LATENCY_Pos 0
#define FLASH_ACR_LATENCY_MASK 0xF
#define FLASH_ACR_WRHIGHFREQ_Pos 8

#endif /* STRIDOPHONE_REGISTERS_H */