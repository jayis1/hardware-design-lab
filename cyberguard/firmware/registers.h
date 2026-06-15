/*
 * registers.h - CyberGuard MMIO Register Definitions
 * STM32L562QE Application MCU
 *
 * Reference: STM32L5 Reference Manual RM0438
 */

#ifndef REGISTERS_H
#define REGISTERS_H

#include <stdint.h>

/* ============================================================
 * Memory Map
 * ============================================================ */
#define FLASH_BASE_ADDR         0x08000000UL
#define SRAM_BASE_ADDR          0x20000000UL
#define PERIPH_BASE_ADDR        0x40000000UL
#define AHB1PERIPH_BASE         (PERIPH_BASE_ADDR + 0x00020000UL)
#define AHB2PERIPH_BASE         (PERIPH_BASE_ADDR + 0x08000000UL)
#define APB1PERIPH_BASE         (PERIPH_BASE_ADDR + 0x00000000UL)
#define APB2PERIPH_BASE         (PERIPH_BASE_ADDR + 0x00010000UL)

/* ============================================================
 * System Control Registers
 * ============================================================ */
#define SCB_BASE                0xE000ED00UL

typedef struct {
    volatile uint32_t CPUID;
    volatile uint32_t ICSR;
    volatile uint32_t VTOR;
    volatile uint32_t AIRCR;
    volatile uint32_t SCR;
    volatile uint32_t CCR;
    volatile uint32_t SHCSR;
    volatile uint32_t CFSR;
    volatile uint32_t HFSR;
    volatile uint32_t DFSR;
    volatile uint32_t MMFAR;
    volatile uint32_t BFAR;
} SCB_TypeDef;

#define SCB ((SCB_TypeDef *)SCB_BASE)

/* ============================================================
 * GPIO Registers (STM32L562)
 * ============================================================ */
#define GPIOA_BASE              (AHB2PERIPH_BASE + 0x00000000UL)
#define GPIOB_BASE              (AHB2PERIPH_BASE + 0x00000400UL)
#define GPIOC_BASE              (AHB2PERIPH_BASE + 0x00000800UL)
#define GPIOH_BASE              (AHB2PERIPH_BASE + 0x00001C00UL)

typedef struct {
    volatile uint32_t MODER;      /* Port mode register */
    volatile uint32_t OTYPER;     /* Port output type register */
    volatile uint32_t OSPEEDR;    /* Port output speed register */
    volatile uint32_t PUPDR;      /* Port pull-up/pull-down register */
    volatile uint32_t IDR;        /* Port input data register */
    volatile uint32_t ODR;        /* Port output data register */
    volatile uint32_t BSRR;       /* Port bit set/reset register */
    volatile uint32_t LCKR;       /* Port configuration lock register */
    volatile uint32_t AFR[2];     /* Alternate function registers [LOW, HIGH] */
    volatile uint32_t BRR;        /* Port bit reset register */
    volatile uint32_t SECCFGR;    /* Security configuration register */
} GPIO_TypeDef;

#define GPIOA ((GPIO_TypeDef *)GPIOA_BASE)
#define GPIOB ((GPIO_TypeDef *)GPIOB_BASE)
#define GPIOC ((GPIO_TypeDef *)GPIOC_BASE)
#define GPIOH ((GPIO_TypeDef *)GPIOH_BASE)

/* GPIO MODER values (2 bits per pin) */
#define GPIO_MODE_INPUT          0U
#define GPIO_MODE_OUTPUT         1U
#define GPIO_MODE_AF             2U
#define GPIO_MODE_ANALOG         3U

/* GPIO OSPEEDR values (2 bits per pin) */
#define GPIO_SPEED_LOW           0U
#define GPIO_SPEED_MEDIUM       1U
#define GPIO_SPEED_HIGH          2U
#define GPIO_SPEED_VERY_HIGH    3U

/* GPIO PUPDR values (2 bits per pin) */
#define GPIO_PULL_NONE           0U
#define GPIO_PULL_UP            1U
#define GPIO_PULL_DOWN           2U

/* GPIO OTYPER values (1 bit per pin) */
#define GPIO_OTYPE_PP           0U
#define GPIO_OTYPE_OD           1U

/* Alternate function numbers */
#define GPIO_AF0_MCO             0U
#define GPIO_AF4_I2C1            4U
#define GPIO_AF4_I2C2            4U
#define GPIO_AF5_SPI1            5U
#define GPIO_AF7_USART1          7U
#define GPIO_AF7_USART2          7U
#define GPIO_AF10_USB            10U

/* ============================================================
 * RCC Registers
 * ============================================================ */
#define RCC_BASE                (AHB1PERIPH_BASE + 0x00001000UL)

typedef struct {
    volatile uint32_t CR;         /* Clock control register */
    volatile uint32_t ICSCR;     /* Internal clock sources calibration */
    volatile uint32_t CFGR;      /* Clock configuration register */
    volatile uint32_t PLLCFGR;   /* PLL configuration register */
    volatile uint32_t RESERVED0;
    volatile uint32_t CIER;      /* Clock interrupt enable */
    volatile uint32_t CIFR;      /* Clock interrupt flags */
    volatile uint32_t CICR;      /* Clock interrupt clear */
    volatile uint32_t RESERVED1;
    volatile uint32_t AHB1ENR;   /* AHB1 peripheral clock enable */
    volatile uint32_t AHB2ENR;   /* AHB2 peripheral clock enable */
    volatile uint32_t AHB3ENR;   /* AHB3 peripheral clock enable */
    volatile uint32_t RESERVED2;
    volatile uint32_t APB1ENR1;  /* APB1 peripheral clock enable 1 */
    volatile uint32_t APB1ENR2;  /* APB1 peripheral clock enable 2 */
    volatile uint32_t APB2ENR;   /* APB2 peripheral clock enable */
    volatile uint32_t RESERVED3[8];
    volatile uint32_t AHB1SMENR;  /* AHB1 peripheral clock enable in Sleep */
    volatile uint32_t AHB2SMENR;
    volatile uint32_t AHB3SMENR;
    volatile uint32_t RESERVED4;
    volatile uint32_t APB1SMENR1;
    volatile uint32_t APB1SMENR2;
    volatile uint32_t APB2SMENR;
    volatile uint32_t RESERVED5[8];
    volatile uint32_t CCIPR1;    /* Clock configuration per IP 1 */
    volatile uint32_t CCIPR2;    /* Clock configuration per IP 2 */
    volatile uint32_t SECCFGR;   /* Security configuration */
} RCC_TypeDef;

#define RCC ((RCC_TypeDef *)RCC_BASE)

/* RCC CR bits */
#define RCC_CR_HSION          (1U << 8)
#define RCC_CR_HSIRDY         (1U << 10)
#define RCC_CR_MSION          (1U << 0)
#define RCC_CR_MSIRDY         (1U << 2)
#define RCC_CR_HSEON          (1U << 16)
#define RCC_CR_HSERDY         (1U << 17)
#define RCC_CR_PLLON          (1U << 24)
#define RCC_CR_PLLRDY         (1U << 25)

/* RCC CFGR bits */
#define RCC_CFGR_SW_HSI       (0U << 0)
#define RCC_CFGR_SW_MSI       (1U << 0)
#define RCC_CFGR_SW_HSE       (2U << 0)
#define RCC_CFGR_SW_PLL       (3U << 0)
#define RCC_CFGR_SWS_MASK     (3U << 2)
#define RCC_CFGR_SWS_PLL      (3U << 2)

/* RCC PLLCFGR bits for 110 MHz from HSI16 */
/* PLLM=1, PLLN=55, PLLP=7, PLLQ=5, PLLR=2 => 16MHz/1*55/2=440MHz VCO, /4=110MHz */
#define RCC_PLLCFGR_PLLM_1    (1U << 4)
#define RCC_PLLCFGR_PLLN_55   (55U << 8)
#define RCC_PLLCFGR_PLLP_7    (7U << 27)
#define RCC_PLLCFGR_PLLQ_5    (5U << 21)
#define RCC_PLLCFGR_PLLR_2    (2U << 16)
#define RCC_PLLCFGR_PLLSRC_HSI (2U << 0)

/* RCC AHB2ENR bits */
#define RCC_AHB2ENR_GPIOAEN   (1U << 0)
#define RCC_AHB2ENR_GPIOBEN   (1U << 1)
#define RCC_AHB2ENR_GPIOCEN   (1U << 2)
#define RCC_AHB2ENR_GPIOHEN   (1U << 7)
#define RCC_AHB2ENR_OTGFSEN   (1U << 12)

/* RCC APB1ENR1 bits */
#define RCC_APB1ENR1_USART2EN (1U << 17)
#define RCC_APB1ENR1_SPI2EN   (1U << 14)
#define RCC_APB1ENR1_I2C1EN   (1U << 21)
#define RCC_APB1ENR1_I2C2EN   (1U << 22)

/* RCC APB2ENR bits */
#define RCC_APB2ENR_USART1EN  (1U << 14)
#define RCC_APB2ENR_SPI1EN    (1U << 12)
#define RCC_APB2ENR_SYSCFGEN  (1U << 0)

/* ============================================================
 * USART Registers
 * ============================================================ */
#define USART1_BASE             (APB2PERIPH_BASE + 0x00004000UL)
#define USART2_BASE             (APB1PERIPH_BASE + 0x00004400UL)

typedef struct {
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t CR3;
    volatile uint32_t BRR;
    volatile uint32_t GTPR;
    volatile uint32_t RTOR;
    volatile uint32_t RQR;
    volatile uint32_t ISR;
    volatile uint32_t ICR;
    volatile uint32_t RDR;
    volatile uint32_t TDR;
} USART_TypeDef;

#define USART1 ((USART_TypeDef *)USART1_BASE)
#define USART2 ((USART_TypeDef *)USART2_BASE)

#define USART_CR1_UE           (1U << 0)
#define USART_CR1_RE           (1U << 2)
#define USART_CR1_TE           (1U << 3)
#define USART_CR1_RXNEIE       (1U << 5)
#define USART_CR1_TXEIE        (1U << 7)
#define USART_CR1_M0          (1U << 12)
#define USART_CR1_OVER8       (1U << 15)

#define USART_ISR_TXE         (1U << 7)
#define USART_ISR_RXNE        (1U << 5)
#define USART_ISR_TC          (1U << 6)
#define USART_ISR_ORE         (1U << 3)

/* ============================================================
 * SPI Registers
 * ============================================================ */
#define SPI1_BASE              (APB2PERIPH_BASE + 0x00003C00UL)
#define SPI2_BASE              (APB1PERIPH_BASE + 0x00003800UL)

typedef struct {
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t SR;
    volatile uint32_t DR;
    volatile uint32_t CRCPR;
    volatile uint32_t RXCRCR;
    volatile uint32_t TXCRCR;
    volatile uint32_t I2SCFGR;
    volatile uint32_t I2SPR;
} SPI_TypeDef;

#define SPI1 ((SPI_TypeDef *)SPI1_BASE)
#define SPI2 ((SPI_TypeDef *)SPI2_BASE)

#define SPI_CR1_CPHA          (1U << 0)
#define SPI_CR1_CPOL          (1U << 1)
#define SPI_CR1_MSTR          (1U << 2)
#define SPI_CR1_BR_DIV8       (2U << 3)
#define SPI_CR1_BR_DIV16      (3U << 3)
#define SPI_CR1_SPE           (1U << 6)
#define SPI_CR1_SSM           (1U << 9)
#define SPI_CR1_SSI           (1U << 8)

#define SPI_CR2_DS_8BIT       (7U << 8)
#define SPI_CR2_SSOE          (1U << 2)
#define SPI_CR2_RXNEIE        (1U << 6)
#define SPI_CR2_TXEIE         (1U << 7)

#define SPI_SR_RXNE           (1U << 0)
#define SPI_SR_TXE            (1U << 1)
#define SPI_SR_BSY            (1U << 7)

/* ============================================================
 * I2C Registers
 * ============================================================ */
#define I2C1_BASE              (APB1PERIPH_BASE + 0x00005400UL)
#define I2C2_BASE              (APB1PERIPH_BASE + 0x00005800UL)

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

#define I2C1 ((I2C_TypeDef *)I2C1_BASE)
#define I2C2 ((I2C_TypeDef *)I2C2_BASE)

#define I2C_CR1_PE            (1U << 0)
#define I2C_CR1_TXIE          (1U << 1)
#define I2C_CR1_RXIE          (1U << 2)
#define I2C_CR1_NACKIE        (1U << 4)
#define I2C_CR1_STOPIE        (1U << 5)

#define I2C_ISR_TXE           (1U << 0)
#define I2C_ISR_RXNE          (1U << 2)
#define I2C_ISR_NACKF         (1U << 4)
#define I2C_ISR_STOPF         (1U << 5)
#define I2C_ISR_TC            (1U << 6)
#define I2C_ISR_BUSY          (1U << 15)

/* I2C TIMINGR for 400kHz @ 110MHz APB1 */
/* PRESC=0, SCLDEL=4, SDADEL=2, SCLH=87, SCLL=143 */
#define I2C_TIMINGR_400K      0x00B05D8FUL

/* ============================================================
 * Power Registers
 * ============================================================ */
#define PWR_BASE               (APB1PERIPH_BASE + 0x00000000UL)

typedef struct {
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t CR3;
    volatile uint32_t CR4;
    volatile uint32_t SR1;
    volatile uint32_t SR2;
    volatile uint32_t SCR;
    volatile uint32_t RESERVED;
    volatile uint32_t PUCRA;
    volatile uint32_t PDCRA;
    volatile uint32_t PUCRB;
    volatile uint32_t PDCRB;
    volatile uint32_t PUCRC;
    volatile uint32_t PDCRC;
    volatile uint32_t PUCRH;
    volatile uint32_t PDCRH;
} PWR_TypeDef;

#define PWR ((PWR_TypeDef *)PWR_BASE)

/* PWR CR1 bits */
#define PWR_CR1_LPMS_STOP2    (2U << 0)

/* ============================================================
 * Flash Registers
 * ============================================================ */
#define FLASH_BASE_ADDR        (PERIPH_BASE_ADDR + 0x00002000UL)

typedef struct {
    volatile uint32_t ACR;
    volatile uint32_t PDKEYR;
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
    volatile uint32_t RESERVED2[4];
    volatile uint32_t PCROP2SR;
    volatile uint32_t PCROP2ER;
    volatile uint32_t WRP2AR;
    volatile uint32_t WRP2BR;
    volatile uint32_t SECR;
} FLASH_TypeDef;

#define FLASH ((FLASH_TypeDef *)FLASH_BASE_ADDR)

/* Flash key values */
#define FLASH_KEY1             0x45670123UL
#define FLASH_KEY2             0xCDEF89ABUL

/* ============================================================
 * SysTick Timer
 * ============================================================ */
#define SysTick_BASE           0xE000E010UL

typedef struct {
    volatile uint32_t CTRL;
    volatile uint32_t LOAD;
    volatile uint32_t VAL;
    volatile uint32_t CALIB;
} SysTick_TypeDef;

#define SysTick ((SysTick_TypeDef *)SysTick_BASE)

#define SysTick_CTRL_ENABLE    (1U << 0)
#define SysTick_CTRL_TICKINT   (1U << 1)
#define SysTick_CTRL_CLKSOURCE (1U << 2)

/* ============================================================
 * NVIC
 * ============================================================ */
#define NVIC_BASE_ADDR         0xE000E100UL

typedef struct {
    volatile uint32_t ISER[8];
    volatile uint32_t RESERVED0[24];
    volatile uint32_t ICER[8];
    volatile uint32_t RESERVED1[24];
    volatile uint32_t ISPR[8];
    volatile uint32_t RESERVED2[24];
    volatile uint32_t ICPR[8];
    volatile uint32_t RESERVED3[24];
    volatile uint32_t IABR[8];
    volatile uint32_t RESERVED4[56];
    volatile uint32_t IPR[60];
} NVIC_TypeDef;

#define NVIC ((NVIC_TypeDef *)NVIC_BASE_ADDR)

/* ============================================================
 * Interrupt Numbers
 * ============================================================ */
#define USART1_IRQn             37
#define USART2_IRQn             38
#define SPI1_IRQn               35
#define I2C1_IRQn               31
#define I2C2_IRQn               32
#define EXTI0_IRQn              6
#define EXTI5_9_IRQn            23
#define EXTI10_15_IRQn          40
#define OTG_FS_IRQn             67
#define DMA1_Channel1_IRQn      11

/* ============================================================
 * DMA Registers
 * ============================================================ */
#define DMA1_BASE               (AHB1PERIPH_BASE + 0x00000000UL)

typedef struct {
    volatile uint32_t CR;
    volatile uint32_t NDTR;
    volatile uint32_t PAR;
    volatile uint32_t MAR;
    volatile uint32_t RESERVED;
} DMA_Channel_TypeDef;

#define DMA1_CH1 ((DMA_Channel_TypeDef *)(DMA1_BASE + 0x08UL))
#define DMA1_CH2 ((DMA_Channel_TypeDef *)(DMA1_BASE + 0x1CUL))
#define DMA1_CH3 ((DMA_Channel_TypeDef *)(DMA1_BASE + 0x30UL))
#define DMA1_CH4 ((DMA_Channel_TypeDef *)(DMA1_BASE + 0x44UL))

/* ============================================================
 * A71CH Secure Element I2C Address
 * ============================================================ */
#define A71CH_I2C_ADDR_7BIT     0x48
#define A71CH_I2C_ADDR_8BIT    (A71CH_I2C_ADDR_7BIT << 1)

/* A71CH Command IDs */
#define A71CH_CMD_INIT           0x10
#define A71CH_CMD_GET_INFO      0x11
#define A71CH_CMD_GEN_KEYPAIR   0x20
#define A71CH_CMD_SIGN          0x21
#define A71CH_CMD_VERIFY        0x22
#define A71CH_CMD_STORE_CERT    0x30
#define A71CH_CMD_READ_CERT     0x31
#define A71CH_CMD_DELETE        0x40

/* ============================================================
 * STPMIC1 PMIC I2C Address and Registers
 * ============================================================ */
#define STPMIC1_I2C_ADDR_7BIT   0x08
#define STPMIC1_I2C_ADDR_8BIT   (STPMIC1_I2C_ADDR_7BIT << 1)

#define STPMIC1_REG_MAIN_CTRL   0x10
#define STPMIC1_REG_BUCK_CTRL   0x11
#define STPMIC1_REG_LDO1_CTRL   0x12
#define STPMIC1_REG_LDO2_CTRL   0x13
#define STPMIC1_REG_BUCK_OUT    0x20
#define STPMIC1_REG_LDO1_OUT    0x21
#define STPMIC1_REG_LDO2_OUT    0x22

/* STPMIC1 control bits */
#define STPMIC1_BUCK_EN         (1U << 0)
#define STPMIC1_LDO1_EN         (1U << 0)
#define STPMIC1_LDO2_EN         (1U << 0)

/* ============================================================
 * PN7150 NFC Controller SPI Protocol
 * ============================================================ */
#define PN7150_SPI_PREAMBLE     0x00
#define PN7150_SPI_STARTCODE    0xFF
#define PN7150_SPI_ACK          0x01
#define PN7150_SPI_NACK         0x00
#define PN7150_SPI_ERR          0x02

/* ============================================================
 * FPC1025 Fingerprint Sensor UART Protocol
 * ============================================================ */
#define FPC1025_BAUD            115200UL
#define FPC1025_CMD_HW_ID      0x10
#define FPC1025_CMD_INIT        0x01
#define FPC1025_CMD_CAPTURE     0x03
#define FPC1025_CMD_ENROLL      0x06
#define FPC1025_CMD_VERIFY      0x07
#define FPC1025_CMD_DELETE      0x08
#define FPC1025_RSP_ACK         0xA0
#define FPC1025_RSP_NACK        0xA1

/* ============================================================
 * MX25R1635F QSPI Flash Commands
 * ============================================================ */
#define MX25_CMD_READ           0x03
#define MX25_CMD_FAST_READ      0x0B
#define MX25_CMD_WRITE_ENABLE   0x06
#define MX25_CMD_PAGE_PROGRAM   0x02
#define MX25_CMD_SECTOR_ERASE   0x20
#define MX25_CMD_CHIP_ERASE     0xC7
#define MX25_CMD_READ_ID        0x9F
#define MX25_CMD_READ_STATUS    0x05
#define MX25_CMD_WRITE_STATUS   0x01
#define MX25_CMD_ENTER_QPI      0x38
#define MX25_CMD_EXIT_QPI       0xFF
#define MX25_CMD_RSTEN          0x66
#define MX25_CMD_RST            0x99

#endif /* REGISTERS_H */