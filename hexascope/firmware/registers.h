/*
 * registers.h — HexaScope MMIO Register Map
 * Complete register definitions for STM32G474 peripherals and FPGA config space
 */

#ifndef REGISTERS_H
#define REGISTERS_H

#include <stdint.h>
#include <stddef.h>

/* ============================================================
 * STM32G474 Peripheral Base Addresses (from RM0440)
 * ============================================================ */

#define PERIPH_BASE              0x40000000U
#define APB1_PERIPH_BASE         PERIPH_BASE
#define APB2_PERIPH_BASE         (PERIPH_BASE + 0x00010000U)
#define AHB1_PERIPH_BASE         (PERIPH_BASE + 0x00020000U)
#define AHB2_PERIPH_BASE         (PERIPH_BASE + 0x08000000U)

/* --- GPIO --- */
#define GPIOA_BASE               (AHB2_PERIPH_BASE + 0x0000U)
#define GPIOB_BASE               (AHB2_PERIPH_BASE + 0x0400U)
#define GPIOC_BASE               (AHB2_PERIPH_BASE + 0x0800U)
#define GPIOD_BASE               (AHB2_PERIPH_BASE + 0x0C00U)

/* --- RCC --- */
#define RCC_BASE                 (AHB1_PERIPH_BASE + 0x2400U)

/* --- FLASH --- */
#define FLASH_BASE               (AHB1_PERIPH_BASE + 0x2000U)

/* --- PWR --- */
#define PWR_BASE                 (APB1_PERIPH_BASE + 0x4800U)

/* --- SPI --- */
#define SPI1_BASE                (APB2_PERIPH_BASE + 0x3000U)
#define SPI2_BASE                (APB1_PERIPH_BASE + 0x3800U)
#define SPI4_BASE                (APB2_PERIPH_BASE + 0x5000U)

/* --- I2C --- */
#define I2C1_BASE                (APB1_PERIPH_BASE + 0x5400U)
#define I2C2_BASE                (APB1_PERIPH_BASE + 0x5800U)

/* --- USART --- */
#define USART1_BASE             (APB2_PERIPH_BASE + 0x3800U)
#define USART3_BASE             (APB1_PERIPH_BASE + 0x4800U)

/* --- DMA --- */
#define DMA1_BASE                (AHB1_PERIPH_BASE + 0x6000U)
#define DMA1_Channel1_BASE       (DMA1_BASE + 0x08U)
#define DMA1_Channel2_BASE       (DMA1_BASE + 0x1CU)
#define DMA1_Channel3_BASE       (DMA1_BASE + 0x30U)
#define DMA1_Channel4_BASE       (DMA1_BASE + 0x44U)
#define DMA1_Channel5_BASE       (DMA1_BASE + 0x58U)
#define DMA1_Channel6_BASE       (DMA1_BASE + 0x6CU)
#define DMA1_Channel7_BASE       (DMA1_BASE + 0x80U)

/* --- ADC --- */
#define ADC12_BASE               (AHB1_PERIPH_BASE + 0x4200U)

/* --- TIM --- */
#define TIM2_BASE                (APB1_PERIPH_BASE + 0x0000U)
#define TIM3_BASE                (APB1_PERIPH_BASE + 0x0400U)

/* ============================================================
 * RCC Register Definitions
 * ============================================================ */
#define RCC_CR                   (*(volatile uint32_t *)(RCC_BASE + 0x00U))
#define RCC_CFGR                 (*(volatile uint32_t *)(RCC_BASE + 0x04U))
#define RCC_PLLCFGR              (*(volatile uint32_t *)(RCC_BASE + 0x08U))

/* RCC CR bits */
#define RCC_CR_HSION             (1U << 0)
#define RCC_CR_HSEON             (1U << 16)
#define RCC_CR_HSERDY            (1U << 17)
#define RCC_CR_PLLON             (1U << 24)
#define RCC_CR_PLLRDY            (1U << 25)

/* RCC CFGR bits */
#define RCC_CFGR_SW_HSI          (0U << 0)
#define RCC_CFGR_SW_HSE          (1U << 0)
#define RCC_CFGR_SW_PLL          (2U << 0)
#define RCC_CFGR_SWS_HSI         (0U << 2)
#define RCC_CFGR_SWS_HSE         (1U << 2)
#define RCC_CFGR_SWS_PLL         (2U << 2)
#define RCC_CFGR_HPRE_DIV1       (0U << 4)
#define RCC_CFGR_PPRE1_DIV1      (0U << 8)
#define RCC_CFGR_PPRE2_DIV1      (0U << 11)

/* RCC AHBENR bits */
#define RCC_AHBENR_GPIOAEN       (1U << 0)
#define RCC_AHBENR_GPIOBEN       (1U << 1)
#define RCC_AHBENR_GPIOCEN       (1U << 2)
#define RCC_AHBENR_GPIODEN       (1U << 3)

/* RCC APB1ENR1 bits */
#define RCC_APB1ENR1_SPI2EN      (1U << 14)
#define RCC_APB1ENR1_USART3EN    (1U << 18)
#define RCC_APB1ENR1_I2C1EN      (1U << 21)
#define RCC_APB1ENR1_I2C2EN      (1U << 22)

/* RCC APB2ENR bits */
#define RCC_APB2ENR_SPI1EN       (1U << 12)
#define RCC_APB2ENR_SPI4EN       (1U << 15)
#define RCC_APB2ENR_USART1EN     (1U << 4)
#define RCC_APB2ENR_TIM1EN       (1U << 0)

/* ============================================================
 * GPIO Register Layout
 * ============================================================ */
typedef struct {
    volatile uint32_t MODER;     /* 0x00: Port mode register */
    volatile uint32_t OTYPER;    /* 0x04: Port output type register */
    volatile uint32_t OSPEEDR;   /* 0x08: Port output speed register */
    volatile uint32_t PUPDR;     /* 0x0C: Port pull-up/pull-down register */
    volatile uint32_t IDR;       /* 0x10: Port input data register */
    volatile uint32_t ODR;       /* 0x14: Port output data register */
    volatile uint32_t BSRR;      /* 0x18: Port bit set/reset register */
    volatile uint32_t LCKR;      /* 0x1C: Port configuration lock register */
    volatile uint32_t AFRL;      /* 0x20: Alternate function low register */
    volatile uint32_t AFRH;       /* 0x24: Alternate function high register */
    volatile uint32_t BRR;       /* 0x28: Port bit reset register */
} GPIO_TypeDef;

#define GPIOA                    ((GPIO_TypeDef *)GPIOA_BASE)
#define GPIOB                    ((GPIO_TypeDef *)GPIOB_BASE)
#define GPIOC                    ((GPIO_TypeDef *)GPIOC_BASE)
#define GPIOD                    ((GPIO_TypeDef *)GPIOD_BASE)

/* ============================================================
 * SPI Register Layout
 * ============================================================ */
typedef struct {
    volatile uint32_t CR1;       /* 0x00: Control register 1 */
    volatile uint32_t CR2;       /* 0x04: Control register 2 */
    volatile uint32_t SR;        /* 0x08: Status register */
    volatile uint32_t DR;        /* 0x0C: Data register */
    volatile uint32_t CRCPR;     /* 0x10: CRC polynomial register */
    volatile uint32_t RXCRCR;    /* 0x14: RX CRC register */
    volatile uint32_t TXCRCR;    /* 0x18: TX CRC register */
    volatile uint32_t I2SCFGR;   /* 0x1C: I2S configuration register */
} SPI_TypeDef;

#define SPI1                     ((SPI_TypeDef *)SPI1_BASE)
#define SPI2                     ((SPI_TypeDef *)SPI2_BASE)
#define SPI4                     ((SPI_TypeDef *)SPI4_BASE)

/* SPI CR1 bits */
#define SPI_CR1_CPHA             (1U << 0)
#define SPI_CR1_CPOL             (1U << 1)
#define SPI_CR1_MSTR             (1U << 2)
#define SPI_CR1_BR_DIV2          (0U << 3)
#define SPI_CR1_BR_DIV4          (1U << 3)
#define SPI_CR1_BR_DIV8          (2U << 3)
#define SPI_CR1_BR_DIV16         (3U << 3)
#define SPI_CR1_BR_DIV32         (4U << 3)
#define SPI_CR1_BR_DIV64         (5U << 3)
#define SPI_CR1_BR_DIV128        (6U << 3)
#define SPI_CR1_BR_DIV256        (7U << 3)
#define SPI_CR1_SPE              (1U << 6)
#define SPI_CR1_LSBFIRST         (1U << 7)
#define SPI_CR1_SSI              (1U << 8)
#define SPI_CR1_SSM              (1U << 9)
#define SPI_CR1_RXONLY           (1U << 10)
#define SPI_CR1_CRCL_8BIT        (0U << 11)
#define SPI_CR1_CRCL_16BIT       (1U << 11)
#define SPI_CR1_CRCNEXT          (1U << 12)
#define SPI_CR1_CRCEN            (1U << 13)
#define SPI_CR1_BIDIMODE         (1U << 15)

/* SPI CR2 bits */
#define SPI_CR2_DS_8BIT          (7U << 8)    /* 0x7 << 8 */
#define SPI_CR2_RXNEIE          (1U << 6)
#define SPI_CR2_TXEIE           (1U << 7)
#define SPI_CR2_SSOE            (1U << 2)

/* SPI SR bits */
#define SPI_SR_RXNE              (1U << 0)
#define SPI_SR_TXE               (1U << 1)
#define SPI_SR_BSY               (1U << 7)
#define SPI_SR_OVR               (1U << 6)
#define SPI_SR_MODF              (1U << 5)
#define SPI_SR_CRCERR            (1U << 4)

/* ============================================================
 * I2C Register Layout
 * ============================================================ */
typedef struct {
    volatile uint32_t CR1;       /* 0x00 */
    volatile uint32_t CR2;       /* 0x04 */
    volatile uint32_t OAR1;      /* 0x08 */
    volatile uint32_t OAR2;      /* 0x0C */
    volatile uint32_t TIMINGR;   /* 0x10 */
    volatile uint32_t TIMEOUTR;  /* 0x14 */
    volatile uint32_t ISR;       /* 0x18 */
    volatile uint32_t ICR;       /* 0x1C */
    volatile uint32_t PECR;      /* 0x20 */
    volatile uint32_t RXDR;      /* 0x24 */
    volatile uint32_t TXDR;      /* 0x28 */
} I2C_TypeDef;

#define I2C1                     ((I2C_TypeDef *)I2C1_BASE)
#define I2C2                     ((I2C_TypeDef *)I2C2_BASE)

/* I2C CR1 bits */
#define I2C_CR1_PE               (1U << 0)
#define I2C_CR1_TXIE            (1U << 2)
#define I2C_CR1_RXIE            (1U << 3)
#define I2C_CR1_STOPIE          (1U << 5)
#define I2C_CR1_NACKIE          (1U << 4)

/* I2C CR2 bits */
#define I2C_CR2_START            (1U << 13)
#define I2C_CR2_STOP             (1U << 14)
#define I2C_CR2_NACK             (1U << 15)
#define I2C_CR2_RD_WRN          (1U << 10)

/* I2C ISR bits */
#define I2C_ISR_TXIS             (1U << 1)
#define I2C_ISR_RXNE             (1U << 2)
#define I2C_ISR_TC               (1U << 6)
#define I2C_ISR_STOPF            (1U << 5)
#define I2C_ISR_NACKF            (1U << 4)
#define I2C_ISR_ADDR             (1U << 3)

/* ============================================================
 * USART Register Layout
 * ============================================================ */
typedef struct {
    volatile uint32_t CR1;       /* 0x00 */
    volatile uint32_t CR2;       /* 0x04 */
    volatile uint32_t CR3;       /* 0x08 */
    volatile uint32_t BRR;       /* 0x0C */
    volatile uint32_t GTPR;      /* 0x10 */
    volatile uint32_t RTOR;      /* 0x14 */
    volatile uint32_t RQR;       /* 0x18 */
    volatile uint32_t ISR;       /* 0x1C */
    volatile uint32_t ICR;       /* 0x20 */
    volatile uint32_t RDR;       /* 0x24 */
    volatile uint32_t TDR;       /* 0x28 */
} USART_TypeDef;

#define USART1                    ((USART_TypeDef *)USART1_BASE)
#define USART3                    ((USART_TypeDef *)USART3_BASE)

/* USART CR1 bits */
#define USART_CR1_UE              (1U << 0)
#define USART_CR1_TE              (1U << 3)
#define USART_CR1_RE              (1U << 2)
#define USART_CR1_TXEIE          (1U << 7)
#define USART_CR1_RXNEIE         (1U << 5)

/* USART ISR bits */
#define USART_ISR_TXE             (1U << 7)
#define USART_ISR_RXNE            (1U << 5)
#define USART_ISR_TC               (1U << 6)

/* ============================================================
 * DMA Register Layout (simplified)
 * ============================================================ */
typedef struct {
    volatile uint32_t CCR;        /* Channel configuration */
    volatile uint32_t CNDTR;     /* Number of data */
    volatile uint32_t CPAR;      /* Peripheral address */
    volatile uint32_t CMAR;      /* Memory address */
    volatile uint32_t CCR2;      /* Reserved / channel 2+ */
} DMA_Channel_TypeDef;

#define DMA1_Channel1             ((DMA_Channel_TypeDef *)DMA1_Channel1_BASE)

/* DMA CCR bits */
#define DMA_CCR_EN                (1U << 0)
#define DMA_CCR_TCIE              (1U << 1)
#define DMA_CCR_HTIE              (1U << 2)
#define DMA_CCR_TEIE              (1U << 3)
#define DMA_CCR_DIR               (1U << 4)
#define DMA_CCR_CIRC              (1U << 5)
#define DMA_CCR_PINC              (1U << 6)
#define DMA_CCR_MINC              (1U << 7)
#define DMA_CCR_PSIZE_8BIT        (0U << 8)
#define DMA_CCR_PSIZE_16BIT       (1U << 8)
#define DMA_CCR_PSIZE_32BIT       (2U << 8)
#define DMA_CCR_MSIZE_8BIT        (0U << 10)
#define DMA_CCR_MSIZE_16BIT       (1U << 10)
#define DMA_CCR_MSIZE_32BIT       (2U << 10)
#define DMA_CCR_PL_LOW            (0U << 12)
#define DMA_CCR_PL_MEDIUM         (1U << 12)
#define DMA_CCR_PL_HIGH           (2U << 12)
#define DMA_CCR_PL_VERY_HIGH      (3U << 12)
#define DMA_CCR_MEM2MEM           (1U << 14)

/* ============================================================
 * FLASH Register Layout
 * ============================================================ */
#define FLASH_ACR                 (*(volatile uint32_t *)(FLASH_BASE + 0x00U))
#define FLASH_ACR_LATENCY_0WS     0U
#define FLASH_ACR_LATENCY_1WS     1U
#define FLASH_ACR_LATENCY_2WS     2U
#define FLASH_ACR_LATENCY_3WS     3U
#define FLASH_ACR_LATENCY_4WS     4U
#define FLASH_ACR_LATENCY_5WS     5U
#define FLASH_ACR_PRFTBE          (1U << 8)

/* ============================================================
 * PWR Register Definitions
 * ============================================================ */
#define PWR_CR1                   (*(volatile uint32_t *)(PWR_BASE + 0x00U))
#define PWR_CR1_VOS_0             (1U << 9)  /* Voltage scale 1 */
#define PWR_CR1_VOS_1             (1U << 10)

/* ============================================================
 * FPGA Register Space (accessed via SPI4 from STM32)
 * Full address map as defined in Phase 1
 * ============================================================ */
#define FPGA_REG_BASE             0x00000000U

/* Trigger Engine registers */
#define FPGA_TRIG_TYPE            (*(volatile uint32_t *)(FPGA_REG_BASE + 0x0000U))
#define FPGA_TRIG_THRESH_CH1      (*(volatile uint32_t *)(FPGA_REG_BASE + 0x0004U))
#define FPGA_TRIG_THRESH_CH2      (*(volatile uint32_t *)(FPGA_REG_BASE + 0x0008U))
#define FPGA_TRIG_THRESH_CH3      (*(volatile uint32_t *)(FPGA_REG_BASE + 0x000CU))
#define FPGA_TRIG_THRESH_CH4      (*(volatile uint32_t *)(FPGA_REG_BASE + 0x0010U))
#define FPGA_TRIG_HYST            (*(volatile uint32_t *)(FPGA_REG_BASE + 0x0014U))
#define FPGA_TRIG_PULSE_MIN       (*(volatile uint32_t *)(FPGA_REG_BASE + 0x0018U))
#define FPGA_TRIG_PULSE_MAX       (*(volatile uint32_t *)(FPGA_REG_BASE + 0x001CU))
#define FPGA_TRIG_TIMEOUT         (*(volatile uint32_t *)(FPGA_REG_BASE + 0x0020U))
#define FPGA_TRIG_LOGIC_MASK      (*(volatile uint32_t *)(FPGA_REG_BASE + 0x0024U))

/* Decimator registers */
#define FPGA_DEC_RATE              (*(volatile uint32_t *)(FPGA_REG_BASE + 0x10000U))
#define FPGA_DEC_MODE              (*(volatile uint32_t *)(FPGA_REG_BASE + 0x10004U))
#define FPGA_DEC_AVG_EN            (*(volatile uint32_t *)(FPGA_REG_BASE + 0x10008U))

/* Protocol Decoder registers */
#define FPGA_PROTO_SEL             (*(volatile uint32_t *)(FPGA_REG_BASE + 0x20000U))
#define FPGA_PROTO_BAUD            (*(volatile uint32_t *)(FPGA_REG_BASE + 0x20004U))
#define FPGA_PROTO_CONFIG          (*(volatile uint32_t *)(FPGA_REG_BASE + 0x20008U))

/* Channel MUX registers */
#define FPGA_MUX_CONFIG            (*(volatile uint32_t *)(FPGA_REG_BASE + 0x30000U))
#define FPGA_MUX_ANA_EN            (*(volatile uint32_t *)(FPGA_REG_BASE + 0x30004U))
#define FPGA_MUX_DIG_EN            (*(volatile uint32_t *)(FPGA_REG_BASE + 0x30008U))

/* DAC Control registers */
#define FPGA_DAC_THRESH_D1_P       (*(volatile uint32_t *)(FPGA_REG_BASE + 0x40000U))
#define FPGA_DAC_THRESH_D1_N       (*(volatile uint32_t *)(FPGA_REG_BASE + 0x40004U))
#define FPGA_DAC_THRESH_D2_P       (*(volatile uint32_t *)(FPGA_REG_BASE + 0x40008U))
#define FPGA_DAC_THRESH_D2_N       (*(volatile uint32_t *)(FPGA_REG_BASE + 0x4000CU))

/* Calibration registers */
#define FPGA_CAL_GAIN_CH1          (*(volatile uint32_t *)(FPGA_REG_BASE + 0x50000U))
#define FPGA_CAL_OFFSET_CH1        (*(volatile uint32_t *)(FPGA_REG_BASE + 0x50004U))
#define FPGA_CAL_GAIN_CH2          (*(volatile uint32_t *)(FPGA_REG_BASE + 0x50008U))
#define FPGA_CAL_OFFSET_CH2        (*(volatile uint32_t *)(FPGA_REG_BASE + 0x5000CU))
#define FPGA_CAL_GAIN_CH3          (*(volatile uint32_t *)(FPGA_REG_BASE + 0x50010U))
#define FPGA_CAL_OFFSET_CH3        (*(volatile uint32_t *)(FPGA_REG_BASE + 0x50014U))
#define FPGA_CAL_GAIN_CH4          (*(volatile uint32_t *)(FPGA_REG_BASE + 0x50018U))
#define FPGA_CAL_OFFSET_CH4        (*(volatile uint32_t *)(FPGA_REG_BASE + 0x5001CU))

/* Status registers */
#define FPGA_STATUS                (*(volatile uint32_t *)(FPGA_REG_BASE + 0x60000U))
#define FPGA_FIFO_LEVEL            (*(volatile uint32_t *)(FPGA_REG_BASE + 0x60004U))
#define FPGA_SAMPLE_COUNT          (*(volatile uint32_t *)(FPGA_REG_BASE + 0x60008U))

/* Status bit definitions */
#define FPGA_STATUS_ARMED           (1U << 0)
#define FPGA_STATUS_TRIGGERED       (1U << 1)
#define FPGA_STATUS_DONE             (1U << 2)
#define FPGA_STATUS_OVERFLOW         (1U << 3)
#define FPGA_STATUS_USB_RDY          (1U << 4)
#define FPGA_STATUS_WIFI_RDY         (1U << 5)

/* USB DMA registers */
#define FPGA_USB_DMA_LEN            (*(volatile uint32_t *)(FPGA_REG_BASE + 0x70000U))
#define FPGA_USB_DMA_EP             (*(volatile uint32_t *)(FPGA_REG_BASE + 0x70004U))
#define FPGA_USB_DMA_START           (*(volatile uint32_t *)(FPGA_REG_BASE + 0x70008U))

/* Wi-Fi DMA registers */
#define FPGA_WIFI_DMA_LEN           (*(volatile uint32_t *)(FPGA_REG_BASE + 0x80000U))
#define FPGA_WIFI_BAUD              (*(volatile uint32_t *)(FPGA_REG_BASE + 0x80004U))
#define FPGA_WIFI_DMA_START          (*(volatile uint32_t *)(FPGA_REG_BASE + 0x80008U))

/* ============================================================
 * Trigger Type Definitions (FPGA_TRIG_TYPE values)
 * ============================================================ */
#define TRIG_TYPE_EDGE_RISING       0x01U
#define TRIG_TYPE_EDGE_FALLING      0x02U
#define TRIG_TYPE_EDGE_BOTH         0x03U
#define TRIG_TYPE_PULSE_WIDTH      0x04U
#define TRIG_TYPE_TIMEOUT           0x05U
#define TRIG_TYPE_RUNT              0x06U
#define TRIG_TYPE_WINDOW             0x07U
#define TRIG_TYPE_LOGIC              0x08U
#define TRIG_TYPE_SERIAL             0x09U

/* ============================================================
 * Protocol Decoder Definitions (FPGA_PROTO_SEL values)
 * ============================================================ */
#define PROTO_UART                   0x01U
#define PROTO_SPI                    0x02U
#define PROTO_I2C                    0x03U
#define PROTO_CAN                    0x04U
#define PROTO_LIN                    0x05U

/* ============================================================
 * DA9063 PMIC Register Map (I2C device address 0x58)
 * ============================================================ */
#define DA9063_I2C_ADDR              0x58U

#define DA9063_REG_PAGE_CON           0x00U
#define DA9063_REG_STATUS_A           0x01U
#define DA9063_REG_STATUS_B           0x02U
#define DA9063_REG_STATUS_C           0x03U
#define DA9063_REG_STATUS_D           0x04U
#define DA9063_REG_FAULT_LOG          0x05U
#define DA9063_REG_EVENT_A            0x06U
#define DA9063_REG_EVENT_B            0x07U
#define DA9063_REG_EVENT_C            0x08U
#define DA9063_REG_EVENT_D            0x09U
#define DA9063_REG_IRQ_MASK_A         0x0AU
#define DA9063_REG_IRQ_MASK_B         0x0BU
#define DA9063_REG_IRQ_MASK_C         0x0CU
#define DA9063_REG_IRQ_MASK_D         0x0DU
#define DA9063_REG_CONTROL_A          0x0EU
#define DA9063_REG_CONTROL_B          0x0FU
#define DA9063_REG_CONTROL_C          0x10U
#define DA9063_REG_CONTROL_D          0x11U
#define DA9063_REG_CONTROL_E          0x12U
#define DA9063_REG_CONTROL_F          0x13U
#define DA9063_REG_BUCK1_CONT         0x20U   /* VDD_CORE (1.1V) */
#define DA9063_REG_BUCK2_CONT         0x21U   /* VDD_DDR (1.35V) */
#define DA9063_REG_BUCK3_CONT         0x22U   /* VDD_IO18 (1.8V) */
#define DA9063_REG_LDO1_CONT          0x30U   /* VDD_IO33 (3.3V) */
#define DA9063_REG_LDO2_CONT          0x31U   /* VDD_ANA (5.0V) */
#define DA9063_REG_LDO3_CONT          0x32U   /* VDD_RTC (3.3V) */

/* DA9063 BUCK configuration:
 *   BUCK1 (1.1V): VSEL = 0x3C (1.10V), enabled
 *   BUCK2 (1.35V): VSEL = 0x52 (1.35V), enabled
 *   BUCK3 (1.8V): VSEL = 0x78 (1.80V), enabled
 *   LDO1 (3.3V): VSEL = 0xCC (3.30V), enabled
 *   LDO2 (5.0V): VSEL = 0xF4 (5.00V), enabled (or use buck-boost)
 *   LDO3 (3.3V): VSEL = 0xCC (3.30V), always-on
 */

/* ============================================================
 * DAC60508 Register Map (SPI device)
 * ============================================================ */
#define DAC60508_REG_NOOP            0x00U
#define DAC60508_REG_DEVICE_ID       0x01U
#define DAC60508_REG_SYNC            0x02U
#define DAC60508_REG_CONFIG          0x03U
#define DAC60508_REG_GAIN            0x04U
#define DAC60508_REG_TRIGGER         0x05U
#define DAC60508_REG_STATUS          0x06U
#define DAC60508_REG_DAC_CH0         0x08U
#define DAC60508_REG_DAC_CH1         0x09U
#define DAC60508_REG_DAC_CH2         0x0AU
#define DAC60508_REG_DAC_CH3         0x0BU
#define DAC60508_REG_DAC_CH4         0x0CU
#define DAC60508_REG_DAC_CH5         0x0DU
#define DAC60508_REG_DAC_CH6         0x0EU
#define DAC60508_REG_DAC_CH7         0x0FU

/* DAC60508 channel mapping:
 *   CH0: CMP1_THRESH_P — Comparator 1 positive threshold
 *   CH1: CMP1_THRESH_N — Comparator 1 negative threshold
 *   CH2: CMP2_THRESH_P — Comparator 2 positive threshold
 *   CH3: CMP2_THRESH_N — Comparator 2 negative threshold
 *   CH4: ANA_CAL_REF   — Analog calibration reference
 *   CH5: ANA_OFFSET_CH1 — Channel 1 offset adjust
 *   CH6: ANA_OFFSET_CH2 — Channel 2 offset adjust
 *   CH7: ANA_GAIN_ADJ   — Gain adjustment reference
 */

/* ============================================================
 * FUSB302 Register Map (I2C device address 0x22)
 * ============================================================ */
#define FUSB302_I2C_ADDR             0x22U

#define FUSB302_REG_DEVICE_ID       0x01U
#define FUSB302_REG_SWITCHES0        0x02U
#define FUSB302_REG_SWITCHES1        0x03U
#define FUSB302_REG_MEASURE           0x04U
#define FUSB302_REG_SLICE             0x05U
#define FUSB302_REG_CONTROL0          0x06U
#define FUSB302_REG_CONTROL1          0x07U
#define FUSB302_REG_CONTROL2          0x08U
#define FUSB302_REG_CONTROL3          0x09U
#define FUSB302_REG_CONTROL4          0x0AU
#define FUSB302_REG_STATUS0           0x10U
#define FUSB302_REG_STATUS1           0x11U
#define FUSB302_REG_INTERRUPT          0x12U
#define FUSB302_REG_FIFOS             0x2CU

/* FUSB302 PD negotiation constants */
#define FUSB302_PD_REQUEST_15V       0x01U  /* Request 15V @ 0.8A */
#define FUSB302_PD_REQUEST_5V         0x02U  /* Request 5V @ 2A (fallback) */

#endif /* REGISTERS_H */