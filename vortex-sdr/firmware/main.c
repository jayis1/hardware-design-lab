/*
 * main.c — Vortex-SDR main firmware entry point
 * STM32H743ZIT6 @ 480 MHz, Cortex-M7, dual-conversion SDR spectrum analyzer
 */

#include <string.h>
#include <stdint.h>
#include "board.h"
#include "registers.h"
#include "usb_descriptors.h"
#include "drivers/adf4351.h"
#include "drivers/ad9645.h"
#include "drivers/ili9341.h"

/* ========================================================================== */
/* DMA buffers                                                                 */
/* ========================================================================== */
#define SPI_DMA_BUF_SIZE     4096
#define UART_RX_BUF_SIZE     256
#define UART_TX_BUF_SIZE     256
#define USB_RX_BUF_SIZE      512
#define USB_TX_BUF_SIZE      512

static volatile uint8_t spi1_dma_buf[SPI_DMA_BUF_SIZE] __attribute__((section(".dma_buf")));
static volatile uint8_t spi2_dma_buf[SPI_DMA_BUF_SIZE] __attribute__((section(".dma_buf")));
static volatile uint8_t spi3_dma_buf[256] __attribute__((section(".dma_buf")));
static volatile uint8_t uart2_rx_buf[UART_RX_BUF_SIZE] __attribute__((section(".dma_buf")));
static volatile uint8_t uart2_tx_buf[UART_TX_BUF_SIZE] __attribute__((section(".dma_buf")));

/* FFT bin data buffer (in AXI SRAM for speed) */
static volatile int16_t fft_bins[FFT_MAX_SIZE] __attribute__((section(".axi_sram")));
/* Peak data */
static volatile peak_data_t peak_data __attribute__((section(".axi_sram")));
/* Display buffer */
static volatile display_buf_t disp_buf __attribute__((section(".axi_sram")));

/* System state */
static volatile sweep_state_t g_sweep;
static volatile uint8_t g_fft_data_ready = 0;
static volatile uint8_t g_display_update = 0;
static volatile uint8_t g_ble_rx_ready = 0;
static volatile uint8_t g_usb_rx_ready = 0;
static volatile uint8_t g_ui_event = 0;
static volatile uint32_t g_button_state = 0;
static volatile uint32_t g_tick_ms = 0;
static volatile int16_t g_agc_voltage = 0;    /* DAC output value (0-4095) */
static volatile uint8_t g_log_pending = 0;
static volatile uint8_t g_flash_busy = 0;

/* Fault info for hard fault handler */
typedef struct {
    uint32_t r0, r1, r2, r3;
    uint32_t pc, lr, psr;
} fault_info_t;

static fault_info_t g_fault_info __attribute__((section(".backup_sram")));

/* ========================================================================== */
/* Clock initialization (HSE 8 MHz -> PLL1 480 MHz SYSCLK)                   */
/* ========================================================================== */
static void clock_init(void)
{
    /* Step 1: Enable HSE (8 MHz crystal) */
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY))
        ;

    /* Step 2: Configure voltage scaling for 480 MHz (VOS0) */
    PWR->CR3 &= ~PWR_CR3_SCUEN;
    MODIFY_REG(PWR->D3CR, PWR_D3CR_VOS_Msk, 3UL << PWR_D3CR_VOS_Pos);
    while (!(PWR->D3CR & PWR_D3CR_VOSRDY))
        ;

    /* Step 3: Configure PLL1 (480 MHz SYSCLK)
     * HSE = 8 MHz, PLL1M = 4, PLL1N = 240, PLL1P = 2
     * VCO = 8/4 * 240 = 480 MHz, SYSCLK = 480/2 = 480 MHz
     * PLL1Q = 8 => 8/4 * 240/8 = 60 MHz (APB1)
     */
    RCC->PLLCKSELR = (4UL << RCC_PLLCKSELR_DIVM1_Pos)
                    | RCC_PLLCKSELR_PLLSRC_HSE;

    RCC->PLL1DIVR = (239UL << 0)    /* PLL1N = 240 (N-1) */
                  | (1UL << 9)       /* PLL1P = 2 (P-1) */
                  | (7UL << 16)      /* PLL1Q = 8 (Q-1) */
                  | (7UL << 24);     /* PLL1R = 8 (R-1) */

    RCC->PLLCFGR = (3UL << 0)        /* PLL1 VCO wide range */
                 | (0UL << 4)         /* PLL1 input 4-8 MHz */
                 | (1UL << 16)        /* PLL1 FRACEN */
                 | (0UL << 20)        /* PLL2 input same as PLL1 */
                 | (0UL << 24);       /* PLL3 input same as PLL1 */

    /* Enable PLL1 */
    RCC->CR |= RCC_CR_PLL1ON;
    while (!(RCC->CR & RCC_CR_PLL1RDY))
        ;

    /* Step 4: Configure PLL2 (200 MHz for SPI)
     * PLL2M = 4, PLL2N = 100, PLL2P = 5
     * VCO = 8/4 * 100 = 200 MHz, PLL2Q = 40 MHz (SPI clocks)
     */
    RCC->PLL2DIVR = (99UL << 0)     /* PLL2N = 100 (N-1) */
                  | (4UL << 9)       /* PLL2P = 5 (P-1) */
                  | (4UL << 16)      /* PLL2Q = 5 (Q-1) */
                  | (9UL << 24);     /* PLL2R = 10 (R-1) */

    RCC->CR |= RCC_CR_PLL2ON;
    while (!(RCC->CR & RCC_CR_PLL2RDY))
        ;

    /* Step 5: Configure PLL3 (48 MHz for USB)
     * PLL3M = 4, PLL3N = 24, PLL3P = 2
     * VCO = 8/4 * 24 = 48 MHz, PLL3Q = 48 MHz (USB)
     */
    RCC->PLL3DIVR = (23UL << 0)      /* PLL3N = 24 (N-1) */
                  | (1UL << 9)       /* PLL3P = 2 (P-1) */
                  | (1UL << 16)      /* PLL3Q = 2 (Q-1) */
                  | (1UL << 24);     /* PLL3R = 2 (R-1) */

    RCC->CR |= RCC_CR_PLL3ON;
    while (!(RCC->CR & RCC_CR_PLL3RDY))
        ;

    /* Step 6: Configure flash latency for 480 MHz at VOS0 */
    FLASH->ACR = (4UL << FLASH_ACR_LATENCY_Pos)
               | FLASH_ACR_PRFTEN
               | FLASH_ACR_DCEN
               | FLASH_ACR_ICEN;

    /* Step 7: Switch SYSCLK to PLL1 */
    MODIFY_REG(RCC->CFGR, 0x3UL, 0x3UL);  /* SW = PLL1 */
    while ((RCC->CFGR & 0xCUL) != 0xCUL)   /* Wait for PLL1 as SYSCLK */
        ;

    /* Step 8: Configure bus prescalers */
    /* AHB = SYSCLK/2 = 240 MHz (HPRE = 2) */
    MODIFY_REG(RCC->D1CFGR, 0xFUL << RCC_D1CFGR_HPRE_Pos, 8UL << RCC_D1CFGR_HPRE_Pos);
    /* APB2 = AHB/2 = 120 MHz (D1PPRE = 4) */
    MODIFY_REG(RCC->D1CFGR, 0x7UL << RCC_D1CFGR_D1PPRE_Pos, 4UL << RCC_D1CFGR_D1PPRE_Pos);
    /* APB1 = AHB/4 = 60 MHz (D2PPRE1 = 5) */
    MODIFY_REG(RCC->D2CFGR, 0x7UL << RCC_D2CFGR_D2PPRE1_Pos, 5UL << RCC_D2CFGR_D2PPRE1_Pos);
    /* APB2 (domain 2) = AHB/2 = 120 MHz */
    MODIFY_REG(RCC->D2CFGR, 0x7UL << RCC_D2CFGR_D2PPRE2_Pos, 4UL << RCC_D2CFGR_D2PPRE2_Pos);
    /* APB4 (domain 3) = AHB/4 = 60 MHz */
    MODIFY_REG(RCC->D3CFGR, 0x7UL << RCC_D3CFGR_D3PPRE_Pos, 5UL << RCC_D3CFGR_D3PPRE_Pos);

    /* Step 9: Enable peripheral clocks */
    RCC->AHB4ENR |= RCC_AHB4ENR_GPIOAEN | RCC_AHB4ENR_GPIOBEN
                  | RCC_AHB4ENR_GPIOCEN | RCC_AHB4ENR_GPIODEN
                  | RCC_AHB4ENR_GPIOEEN;
    RCC->APB1LENR |= RCC_APB1LENR_USART2EN | RCC_APB1LENR_SPI3EN;
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN | RCC_APB2ENR_SPI1EN;
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN | RCC_AHB1ENR_DMA2EN;

    /* Step 10: Enable HSI48 for USB */
    RCC->CRRC |= RCC_CRRC_HSI48ON;
    while (!(RCC->CRRC & RCC_CRRC_HSI48RDY))
        ;

    /* Step 11: Enable LSE for RTC */
    RCC->BDCR |= RCC_BDCR_LSEON;
    while (!(RCC->BDCR & RCC_BDCR_LSERDY))
        ;
}

/* ========================================================================== */
/* GPIO initialization                                                        */
/* ========================================================================== */
static void gpio_init(void)
{
    /* Port A — SPI1, USART2, USB, DAC, analog */
    GPIOA->MODER = (0UL << 0)    /* PA0: Input (ADC_IRQ) */
                 | (1UL << 2)    /* PA1: Output (LNA_EN) */
                 | (2UL << 4)    /* PA2: AF7 (USART2_TX) */
                 | (2UL << 6)    /* PA3: AF7 (USART2_RX) */
                 | (3UL << 8)    /* PA4: Analog (DAC1) */
                 | (2UL << 10)   /* PA5: AF5 (SPI1_SCK) */
                 | (2UL << 12)   /* PA6: AF5 (SPI1_MISO) */
                 | (2UL << 14)   /* PA7: AF5 (SPI1_MOSI) */
                 | (2UL << 16)   /* PA8: AF0 (MCO1) */
                 | (1UL << 18)   /* PA9: Input (USB_VBUS) */
                 | (1UL << 20)   /* PA10: Input (USB_ID) */
                 | (2UL << 22)   /* PA11: AF10 (USB_DM) */
                 | (2UL << 24)   /* PA12: AF10 (USB_DP) */
                 | (2UL << 26)   /* PA13: AF0 (SWDIO) */
                 | (2UL << 28)   /* PA14: AF0 (SWCLK) */
                 | (1UL << 30);  /* PA15: Output (FPGA_CDONE) */
    GPIOA->OSPEEDR = 0xAAAAA800UL;
    GPIOA->PUPDR = (1UL << 0) | (1UL << 30);

    /* Port B — SPI2, SPI3, I2C1, PLL, LCD */
    GPIOB->MODER = (1UL << 0)    /* PB0: Output (MIXER_EN) */
                 | (1UL << 2)    /* PB1: Output (ATTEN_SEL) */
                 | (2UL << 4)    /* PB2: AF5 (SPI2_SCK) */
                 | (2UL << 6)    /* PB3: AF5 (SPI2_MOSI) */
                 | (1UL << 8)    /* PB4: Output (LCD_DC) */
                 | (1UL << 10)   /* PB5: Output (LCD_CS) */
                 | (1UL << 12)   /* PB6: Output (LCD_RST) */
                 | (0UL << 14)   /* PB7: Input (TOUCH_IRQ) */
                 | (2UL << 16)   /* PB8: AF4 (I2C1_SCL) */
                 | (2UL << 18)   /* PB9: AF4 (I2C1_SDA) */
                 | (1UL << 20)   /* PB10: Output (PLL_LE) */
                 | (1UL << 22)   /* PB11: Output (PLL_CE) */
                 | (2UL << 24)   /* PB12: AF6 (SPI3_SCK) */
                 | (2UL << 26)   /* PB13: AF6 (SPI3_MISO) */
                 | (2UL << 28)   /* PB14: AF6 (SPI3_MOSI) */
                 | (1UL << 30);  /* PB15: Output (FLASH_CS) */
    GPIOB->OSPEEDR = 0xAAA8AA00UL;
    GPIOB->PUPDR = (1UL << 14);

    /* Port C — FPGA, touch, LEDs, charger */
    GPIOC->MODER = (1UL << 12)   /* PC6: Output (FPGA_RST) */
                 | (1UL << 14)   /* PC7: Output (FPGA_CS) */
                 | (1UL << 16)   /* PC8: Output (TOUCH_CS) */
                 | (1UL << 18)   /* PC9: Output (LED_STATUS) */
                 | (1UL << 20)   /* PC10: Output (LED_ERROR) */
                 | (0UL << 22)   /* PC11: Input (CHG_STAT1) */
                 | (0UL << 24);  /* PC12: Input (CHG_STAT2) */
    GPIOC->OSPEEDR = 0x00550000UL;

    /* Port D — FPGA config, analog, mux, USART1 */
    GPIOD->MODER = (1UL << 0)    /* PD0: Output (FPGA_SCK) */
                 | (1UL << 2)    /* PD1: Output (FPGA_SI) */
                 | (1UL << 4)    /* PD2: Output (FPGA_SS) */
                 | (3UL << 6)    /* PD3: Analog (BAT_SENSE) */
                 | (3UL << 8)    /* PD4: Analog (TEMP_SENSE) */
                 | (1UL << 10)   /* PD5: Output (MUX_SEL0) */
                 | (1UL << 12)   /* PD6: Output (MUX_SEL1) */
                 | (1UL << 14)   /* PD7: Output (MUX_EN) */
                 | (2UL << 16)   /* PD8: AF7 (USART1_TX) */
                 | (2UL << 18);  /* PD9: AF7 (USART1_RX) */
    GPIOD->OSPEEDR = 0x00050005UL;

    /* Port E — Buttons */
    GPIOE->MODER = 0x00000000UL;  /* All inputs */
    GPIOE->PUPDR = (1UL << 0)     /* PE0: Pull-up (BTN_CENTER) */
                 | (1UL << 2)     /* PE1: Pull-up (BTN_UP) */
                 | (1UL << 4)     /* PE2: Pull-up (BTN_DOWN) */
                 | (1UL << 6)     /* PE3: Pull-up (BTN_LEFT) */
                 | (1UL << 8)     /* PE4: Pull-up (BTN_RIGHT) */
                 | (1UL << 10);  /* PE5: Pull-up (BTN_MENU) */

    /* Set initial output states */
    GPIOA->ODR = (0UL << 1);              /* LNA off */
    GPIOB->ODR = (1UL << 15);             /* FLASH_CS high (deselected) */
    GPIOB->ODR |= (1UL << 5);             /* LCD_CS high (deselected) */
    GPIOC->ODR = (1UL << 6)               /* FPGA held in reset */
              | (1UL << 7)                /* FPGA_CS high */
              | (1UL << 8);              /* TOUCH_CS high */
    GPIOD->ODR = (1UL << 2);             /* FPGA_SS high (deselected) */
}

/* ========================================================================== */
/* SPI1 initialization (FPGA data readback, 50 MHz)                           */
/* ========================================================================== */
static void spi1_init(void)
{
    /* Enable SPI1 clock */
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

    /* Configure SPI1: Master, 16-bit, CPOL=0, CPHA=0, prescaler /4 = 12.5 MHz initially */
    SPI1->CR1 = SPI_CR1_MSTR            /* Master mode */
              | (2UL << SPI_CR1_BR_Pos)  /* BR = /8 = 15 MHz */
              | SPI_CR1_SSM              /* Software slave management */
              | SPI_CR1_SSI;             /* Internal slave select */

    SPI1->CR2 = (0xFUL << SPI_CR2_DS_Pos) /* 16-bit data size */
              | SPI_CR2_SSOE               /* NSS output enable */
              | SPI_CR2_RXDMAEN            /* RX DMA enable */
              | SPI_CR2_TXDMAEN;           /* TX DMA enable */

    /* Enable SPI1 */
    SPI1->CR1 |= SPI_CR1_SPE;
}

/* ========================================================================== */
/* SPI2 initialization (Display, 30 MHz)                                      */
/* ========================================================================== */
static void spi2_init(void)
{
    /* Enable SPI2 clock (APB1 domain) */
    RCC->APB1LENR |= RCC_APB1LENR_SPI3EN;  /* Note: SPI2 on APB1L */
    RCC->APB1LENR |= (1UL << 14);           /* SPI2EN */

    /* Configure SPI2: Master, 8-bit, CPOL=0, CPHA=0 */
    SPI2->CR1 = SPI_CR1_MSTR
              | (3UL << SPI_CR1_BR_Pos)  /* BR = /16 ≈ 7.5 MHz (initial) */
              | SPI_CR1_SSM
              | SPI_CR1_SSI;

    SPI2->CR2 = (0x7UL << SPI_CR2_DS_Pos) /* 8-bit data size */
              | SPI_CR2_SSOE
              | SPI_CR2_TXDMAEN;

    SPI2->CR1 |= SPI_CR1_SPE;
}

/* ========================================================================== */
/* SPI3 initialization (Flash storage, 50 MHz)                               */
/* ========================================================================== */
static void spi3_init(void)
{
    /* Enable SPI3 clock */
    RCC->APB1LENR |= RCC_APB1LENR_SPI3EN;

    /* Configure SPI3: Master, 8-bit, CPOL=0, CPHA=0 */
    SPI3->CR1 = SPI_CR1_MSTR
              | (0UL << SPI_CR1_BR_Pos)  /* BR = /2 = 30 MHz */
              | SPI_CR1_SSM
              | SPI_CR1_SSI;

    SPI3->CR2 = (0x7UL << SPI_CR2_DS_Pos) /* 8-bit data size */
              | SPI_CR2_SSOE;

    SPI3->CR1 |= SPI_CR1_SPE;
}

/* ========================================================================== */
/* USART1 initialization (Debug console, 115200 baud)                         */
/* ========================================================================== */
static void usart1_init(void)
{
    /* Enable USART1 clock */
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;

    /* Baud rate: 115200 @ APB2 = 120 MHz
     * BRR = 120000000 / 115200 = 1041.67 ≈ 1042
     * With OVER8=0: BRR = 120000000 / 115200 = 1042
     */
    USART1->BRR = 1042UL;
    USART1->CR1 = USART_CR1_UE    /* USART enable */
               | USART_CR1_TE     /* Transmitter enable */
               | USART_CR1_RE;    /* Receiver enable */
}

/* ========================================================================== */
/* USART2 initialization (BLE module, 921600 baud)                            */
/* ========================================================================== */
static void usart2_init(void)
{
    /* Enable USART2 clock */
    RCC->APB1LENR |= RCC_APB1LENR_USART2EN;

    /* Baud rate: 921600 @ APB1 = 60 MHz
     * BRR = 60000000 / 921600 = 65.1 ≈ 65
     * With OVER8=1: BRR = (60000000 * 2) / (921600 * 2) = 65
     */
    USART2->CR1 = USART_CR1_OVER8;  /* OVER8 = 1 for high baud */
    USART2->BRR = 65UL;
    USART2->CR1 |= USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

/* ========================================================================== */
/* Timer initialization (1 ms system tick + 10ms FFT poll + 50ms display)     */
/* ========================================================================== */
static void timer_init(void)
{
    /* TIM6: 1 ms system tick (480 MHz / 240 = 2 MHz, /2000 = 1 kHz = 1 ms) */
    TIM6->PSC = 239;       /* Prescaler: 2 MHz */
    TIM6->ARR = 1999;      /* Auto-reload: 2 MHz / 2000 = 1 kHz */
    TIM6->DIER = TIM_DIER_UIE;  /* Update interrupt enable */
    TIM6->CR1 = TIM_CR1_CEN;    /* Enable timer */

    /* TIM7: 10 ms FFT poll timer */
    TIM7->PSC = 239;
    TIM7->ARR = 19999;     /* 2 MHz / 20000 = 100 Hz = 10 ms */
    TIM7->DIER = TIM_DIER_UIE;
    TIM7->CR1 = TIM_CR1_CEN;

    /* TIM1: 50 ms display update */
    TIM1->PSC = 4799;      /* Prescaler: 480 MHz / 4800 = 100 kHz */
    TIM1->ARR = 4999;      /* 100 kHz / 5000 = 20 Hz = 50 ms */
    TIM1->DIER = TIM_DIER_UIE;
    TIM1->CR1 = TIM_CR1_CEN;

    /* Enable TIM6, TIM7, TIM1 interrupts in NVIC */
    /* NVIC_EnableIRQ(TIM6_DAC_IRQn); */
    /* NVIC_EnableIRQ(TIM7_IRQn); */
    /* NVIC_EnableIRQ(TIM1_UP_IRQn); */
}

/* ========================================================================== */
/* DAC initialization (AGC voltage control, 0-3.3V = 0-4095)                 */
/* ========================================================================== */
static void dac_init(void)
{
    /* Enable DAC clock */
    RCC->APB1LENR |= (1UL << 16);  /* DAC1EN */

    /* Enable DAC channel 1 */
    DAC1->CR = DAC_CR_EN1;  /* Enable DAC1, buffer enabled */

    /* Set initial AGC voltage to mid-scale (1.65V) */
    DAC1->DHR12R1 = 2048;   /* 50% = ~1.65V */
}

/* ========================================================================== */
/* ADC initialization (battery + temperature)                                  */
/* ========================================================================== */
static void adc_init(void)
{
    /* Enable ADC clock */
    RCC->APB1HENR |= RCC_APB1HENR_ADC12EN;

    /* Exit from deep power-down */
    ADC1->CR = 0UL;

    /* Calibrate ADC */
    ADC1->CR |= (1UL << 16);  /* ADCAL */
    while (ADC1->CR & (1UL << 16))
        ;

    /* Enable ADC */
    ADC1->ISR = ADC_ISR_ADRDY;
    ADC1->CR |= ADC_CR_ADEN;
    while (!(ADC1->ISR & ADC_ISR_ADRDY))
        ;

    /* Configure channel for battery (PD3 = ADC1_IN4) */
    ADC1->SMPR1 = (4UL << 0);  /* 4 = 47.5 cycles for channel 4 */
}

/* ========================================================================== */
/* Power-on sequence for RF front-end                                          */
/* ========================================================================== */
static void power_init(void)
{
    /* Enable LNA */
    SET_BIT(GPIOA->ODR, PIN_LNA_EN);

    /* Small delay for LNA stabilization */
    for (volatile uint32_t i = 0; i < 10000; i++)
        ;

    /* Enable mixer */
    SET_BIT(GPIOB->ODR, PIN_MIXER_EN);

    /* Enable PLL */
    SET_BIT(GPIOB->ODR, PIN_PLL_CE);

    /* Wait for power rails to stabilize */
    for (volatile uint32_t i = 0; i < 50000; i++)
        ;
}

/* ========================================================================== */
/* FPGA initialization (load bitstream from flash)                             */
/* ========================================================================== */
static int fpga_init(void)
{
    uint32_t timeout;

    /* Assert FPGA reset */
    CLR_BIT(GPIOC->ODR, PIN_FPGA_RST);

    /* Wait 1 ms reset pulse */
    for (volatile uint32_t i = 0; i < 10000; i++)
        ;

    /* Release FPGA reset */
    SET_BIT(GPIOC->ODR, PIN_FPGA_RST);

    /* Wait for CDONE with timeout */
    timeout = 1000000;
    while (!(GPIOA->IDR & (1UL << PIN_FPGA_CDONE)) && --timeout)
        ;

    if (timeout == 0) {
        /* FPGA configuration failed */
        SET_BIT(GPIOC->ODR, PIN_LED_ERROR);
        return -1;
    }

    /* Additional delay for FPGA initialization */
    for (volatile uint32_t i = 0; i < 100000; i++)
        ;

    /* Configure FPGA via SPI1 (write initial register settings) */
    /* FPGA register 0x01: FFT size = 1024, window = Hanning */
    uint16_t fpga_cfg[4] = {
        0x0100 | 1024,   /* Reg 0x01: FFT size = 1024 */
        0x0200 | 1,      /* Reg 0x02: Window = Hanning */
        0x0300 | 4,      /* Reg 0x03: Averaging = 4 */
        0x0400 | 0       /* Reg 0x04: Decimation = 1x */
    };

    for (int i = 0; i < 4; i++) {
        CLR_BIT(GPIOC->ODR, PIN_FPGA_CS);
        while (!(SPI1->SR & SPI_SR_TXE))
            ;
        SPI1->DR = fpga_cfg[i];
        while (!(SPI1->SR & SPI_SR_RXNE))
            ;
        (void)SPI1->DR;  /* Read to clear RXNE */
        SET_BIT(GPIOC->ODR, PIN_FPGA_CS);
        for (volatile uint32_t d = 0; d < 1000; d++)
            ;
    }

    return 0;
}

/* ========================================================================== */
/* Initialize default sweep state                                              */
/* ========================================================================== */
static void sweep_state_init(sweep_state_t *state)
{
    state->start_freq = SWEEP_DEFAULT_START;
    state->stop_freq = SWEEP_DEFAULT_STOP;
    state->rbw = 10000;                    /* 10 kHz default */
    state->fft_size = FFT_DEFAULT_SIZE;
    state->window = WINDOW_HANN;
    state->averaging = 4;
    state->attenuation = 0;
    state->agc_level = -20;               /* -20 dB default AGC */
    state->mode = SWEEP_MODE_CONTINUOUS;
    state->state = SYS_STATE_IDLE;
    state->ref_level = -200;              /* -20.0 dBm */
    state->scale = 100;                   /* 10 dB/div */
    state->sweep_count = 0;
    state->sweep_time_ms = 0;
    state->pll_locked = 0;
    state->adc_overrange = 0;
    state->battery_mv = 0;
    state->temperature = 0;
}

/* ========================================================================== */
/* Start a sweep                                                               */
/* ========================================================================== */
static void sweep_start(sweep_state_t *state)
{
    /* Calculate PLL frequency: f_LO = f_RF + f_IF */
    uint64_t lo_freq = state->start_freq + SWEEP_IF_FREQ_HZ;

    /* Configure ADF4351 for start frequency */
    adf4351_set_frequency(lo_freq);

    /* Wait for PLL lock */
    uint32_t timeout = 100000;
    while (!adf4351_lock_detect() && --timeout)
        ;

    if (timeout == 0) {
        state->pll_locked = 0;
        SET_BIT(GPIOC->ODR, PIN_LED_ERROR);
    } else {
        state->pll_locked = 1;
    }

    /* Configure ADC for sweep */
    ad9645_set_decimation(1);  /* No decimation for full bandwidth */

    /* Configure FPGA for sweep */
    /* (already configured in fpga_init) */

    /* Set sweep state */
    state->state = SYS_STATE_SWEEP_CONTINUOUS;
    state->sweep_count = 0;

    /* Enable FFT data ready interrupt */
    g_fft_data_ready = 0;
}

/* ========================================================================== */
/* Stop sweep                                                                   */
/* ========================================================================== */
static void sweep_stop(sweep_state_t *state)
{
    state->state = SYS_STATE_IDLE;

    /* Power down ADC */
    ad9645_power_down();

    /* Power down PLL output */
    adf4351_rf_output_disable();
}

/* ========================================================================== */
/* Process FFT bins from FPGA (called from DMA interrupt)                     */
/* ========================================================================== */
static void fft_process_bins(sweep_state_t *state)
{
    int16_t *bins = (int16_t *)fft_bins;
    uint16_t num_bins = state->fft_size;

    /* Find peaks in the FFT data */
    peak_data_t *peaks = (peak_data_t *)&peak_data;
    peaks->num_peaks = 0;

    int16_t prev_val = bins[0];
    int16_t curr_val = bins[1];
    int16_t next_val;

    for (uint16_t i = 1; i < num_bins - 1 && peaks->num_peaks < MAX_PEAKS; i++) {
        next_val = bins[i + 1];
        if (curr_val > prev_val && curr_val > next_val && curr_val > -700) {
            /* Local peak found */
            peaks->peaks[peaks->num_peaks].bin_index = i;
            peaks->peaks[peaks->num_peaks].power = curr_val;
            peaks->peaks[peaks->num_peaks].freq = state->start_freq +
                ((uint64_t)i * (state->stop_freq - state->start_freq)) / num_bins;
            peaks->num_peaks++;
        }
        prev_val = curr_val;
        curr_val = next_val;
    }

    /* Map FFT bins to display pixels (320 pixels wide) */
    uint16_t *spectrum_buf = (uint16_t *)&disp_buf.spectrum_buf;
    int16_t max_power = -1000;
    int16_t min_power = 0;

    /* Find min/max for auto-scaling */
    for (uint16_t i = 0; i < num_bins; i++) {
        if (bins[i] > max_power) max_power = bins[i];
        if (bins[i] < min_power) min_power = bins[i];
    }

    /* Map to screen coordinates */
    uint32_t power_range = (uint32_t)(max_power - min_power);
    if (power_range == 0) power_range = 1;

    for (uint16_t px = 0; px < ILI9341_WIDTH; px++) {
        uint16_t bin_start = (uint32_t)px * num_bins / ILI9341_WIDTH;
        uint16_t bin_end = (uint32_t)(px + 1) * num_bins / ILI9341_WIDTH;
        if (bin_end >= num_bins) bin_end = num_bins - 1;

        /* Find max power in this pixel range */
        int16_t pixel_max = -1000;
        for (uint16_t b = bin_start; b <= bin_end; b++) {
            if (bins[b] > pixel_max) pixel_max = bins[b];
        }

        /* Convert to screen Y coordinate */
        uint16_t y = ILI9341_HEIGHT - 1 -
            ((uint32_t)(pixel_max - min_power) * (ILI9341_HEIGHT - 1)) / power_range;
        spectrum_buf[px] = y;
    }

    /* Prepare waterfall data (color-map power to RGB565) */
    uint16_t *waterfall_buf = (uint16_t *)&disp_buf.waterfall_buf;
    for (uint16_t px = 0; px < ILI9341_WIDTH; px++) {
        uint16_t bin_idx = (uint32_t)px * num_bins / ILI9341_WIDTH;
        if (bin_idx >= num_bins) bin_idx = num_bins - 1;

        int16_t power = bins[bin_idx];
        /* Map power to color: -700 (blue) to 0 (red) */
        int32_t normalized = ((int32_t)power - min_power) * 255 / (int32_t)power_range;

        uint8_t r = 0, g = 0, b = 0;
        if (normalized < 64) {
            b = normalized * 4;
        } else if (normalized < 128) {
            g = (normalized - 64) * 4;
            b = 255;
        } else if (normalized < 192) {
            r = (normalized - 128) * 4;
            g = 255;
            b = 255 - (normalized - 128) * 4;
        } else {
            r = 255;
            g = 255 - (normalized - 192) * 4;
        }

        /* Convert to RGB565 */
        waterfall_buf[px] = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    }

    /* Set display update flags */
    disp_buf.update_flags = DISP_UPDATE_ALL;

    /* Update sweep counter */
    ((sweep_state_t *)state)->sweep_count++;
}

/* ========================================================================== */
/* Update display (called from 50ms timer)                                     */
/* ========================================================================== */
static void display_update(sweep_state_t *state)
{
    if (!(disp_buf.update_flags & DISP_UPDATE_ALL))
        return;

    /* Draw spectrum trace */
    if (disp_buf.update_flags & DISP_UPDATE_SPECTRUM) {
        uint16_t *spectrum_buf = (uint16_t *)&disp_buf.spectrum_buf;
        uint16_t prev_y = spectrum_buf[0];

        for (uint16_t x = 1; x < ILI9341_WIDTH; x++) {
            uint16_t y = spectrum_buf[x];
            ili9341_draw_line(x - 1, prev_y, x, y, COLOR_GREEN);
            prev_y = y;
        }
    }

    /* Draw waterfall (shift down, draw new line at top) */
    if (disp_buf.update_flags & DISP_UPDATE_WATERFALL) {
        uint16_t *waterfall_buf = (uint16_t *)&disp_buf.waterfall_buf;

        /* Scroll waterfall down by 1 pixel */
        /* (In production, use DMA2D for efficient scrolling) */
        for (uint16_t y = ILI9341_HEIGHT - 1; y > ILI9341_HEIGHT / 2; y--) {
            for (uint16_t x = 0; x < ILI9341_WIDTH; x++) {
                /* Read pixel at y-1, write at y */
                /* ili9341_draw_pixel(x, y, ili9341_read_pixel(x, y - 1)); */
            }
        }

        /* Draw new waterfall line at top of waterfall region */
        uint16_t wf_top = ILI9341_HEIGHT / 2;
        for (uint16_t x = 0; x < ILI9341_WIDTH; x++) {
            ili9341_draw_pixel(x, wf_top, waterfall_buf[x]);
        }
    }

    /* Draw status bar */
    if (disp_buf.update_flags & DISP_UPDATE_STATUS) {
        char buf[32];

        /* Frequency range */
        uint64_t start_mhz = state->start_freq / 1000000;
        uint64_t start_khz = (state->start_freq % 1000000) / 1000;
        uint64_t stop_mhz = state->stop_freq / 1000000;
        uint64_t stop_khz = (state->stop_freq % 1000000) / 1000;

        snprintf(buf, sizeof(buf), "%lu.%03lu - %lu.%03lu MHz",
                 (uint32_t)start_mhz, (uint32_t)start_khz,
                 (uint32_t)stop_mhz, (uint32_t)stop_khz);
        ili9341_draw_text(4, 2, buf, COLOR_WHITE, COLOR_DARK_GRAY);

        /* Sweep count */
        snprintf(buf, sizeof(buf), "Sweeps: %lu", state->sweep_count);
        ili9341_draw_text(4, 12, buf, COLOR_CYAN, COLOR_DARK_GRAY);

        /* PLL lock status */
        if (state->pll_locked) {
            ili9341_draw_text(280, 2, "PLL OK", COLOR_GREEN, COLOR_DARK_GRAY);
        } else {
            ili9341_draw_text(280, 2, "PLL !", COLOR_RED, COLOR_DARK_GRAY);
        }

        /* Battery */
        snprintf(buf, sizeof(buf), "BAT: %umV", state->battery_mv);
        ili9341_draw_text(280, 12, buf, COLOR_YELLOW, COLOR_DARK_GRAY);
    }

    /* Draw markers */
    if (disp_buf.update_flags & DISP_UPDATE_MARKERS) {
        peak_data_t *peaks = (peak_data_t *)&peak_data;
        for (uint8_t i = 0; i < peaks->num_peaks; i++) {
            uint16_t px = (uint32_t)peaks->peaks[i].bin_index * ILI9341_WIDTH / state->fft_size;
            uint16_t py = (uint16_t)((int32_t)peaks->peaks[i].power + 700) * ILI9341_HEIGHT / 700;
            ili9341_draw_line(px, py - 3, px, py + 3, COLOR_YELLOW);
            ili9341_draw_line(px - 3, py, px + 3, py, COLOR_YELLOW);
        }
    }

    /* Clear update flags */
    disp_buf.update_flags = 0;
}

/* ========================================================================== */
/* Read battery voltage                                                        */
/* ========================================================================== */
static uint16_t read_battery_mv(void)
{
    /* Read ADC channel 4 (PD3 = BAT_SENSE) */
    ADC1->CR |= ADC_CR_ADSTART;
    while (!(ADC1->ISR & ADC_ISR_EOC))
        ;
    uint16_t adc_val = ADC1->DR;

    /* Convert to millivolts: ADC 12-bit = 0-4095 = 0-3300mV
     * Voltage divider: VBAT = ADC_val * 3300 * 2 / 4095 */
    uint32_t mv = (uint32_t)adc_val * 3300UL * BAT_SENSE_DIVIDER / 4095UL;
    return (uint16_t)mv;
}

/* ========================================================================== */
/* Handle button input                                                         */
/* ========================================================================== */
static void ui_handle_event(sweep_state_t *state)
{
    uint32_t buttons = ~GPIOE->IDR & 0x3F;  /* Inverted (active-low) */

    if (buttons & BTN_CENTER) {
        /* Center button: toggle sweep on/off */
        if (state->state == SYS_STATE_IDLE) {
            sweep_start(state);
        } else if (state->state == SYS_STATE_SWEEP_CONTINUOUS) {
            sweep_stop(state);
        }
    }

    if (buttons & BTN_UP) {
        /* Up: increase reference level by 10 dB */
        if (state->ref_level < 200) {
            state->ref_level += 10;
        }
    }

    if (buttons & BTN_DOWN) {
        /* Down: decrease reference level by 10 dB */
        if (state->ref_level > -1200) {
            state->ref_level -= 10;
        }
    }

    if (buttons & BTN_LEFT) {
        /* Left: shift start frequency -10 MHz */
        if (state->start_freq > 100000UL) {
            state->start_freq -= 10000000UL;
            state->stop_freq -= 10000000UL;
        }
    }

    if (buttons & BTN_RIGHT) {
        /* Right: shift start frequency +10 MHz */
        if (state->stop_freq < SWEEP_MAX_FREQ_HZ) {
            state->start_freq += 10000000UL;
            state->stop_freq += 10000000UL;
        }
    }

    if (buttons & BTN_MENU) {
        /* Menu: cycle FFT window */
        state->window = (window_t)((state->window + 1) % 5);
    }

    g_ui_event = 0;
}

/* ========================================================================== */
/* Process BLE command                                                         */
/* ========================================================================== */
static void ble_process_command(sweep_state_t *state)
{
    uint8_t cmd = uart2_rx_buf[0];
    uint8_t len = uart2_rx_buf[1];
    uint16_t seq = (uart2_rx_buf[2] << 8) | uart2_rx_buf[3];

    switch (cmd) {
    case CMD_GET_STATUS:
        /* Send status response */
        break;

    case CMD_START_SWEEP:
        sweep_start(state);
        break;

    case CMD_STOP_SWEEP:
        sweep_stop(state);
        break;

    case CMD_SET_FREQ: {
        uint64_t start = ((uint64_t)uart2_rx_buf[4] << 24) |
                         ((uint64_t)uart2_rx_buf[5] << 16) |
                         ((uint64_t)uart2_rx_buf[6] << 8) |
                         uart2_rx_buf[7];
        uint64_t stop = ((uint64_t)uart2_rx_buf[8] << 24) |
                        ((uint64_t)uart2_rx_buf[9] << 16) |
                        ((uint64_t)uart2_rx_buf[10] << 8) |
                        uart2_rx_buf[11];
        if (start >= SWEEP_MIN_FREQ_HZ && stop <= SWEEP_MAX_FREQ_HZ && start < stop) {
            state->start_freq = start;
            state->stop_freq = stop;
        }
        break;
    }

    case CMD_SET_FFT_SIZE: {
        uint32_t fft_size = (uart2_rx_buf[4] << 24) |
                            (uart2_rx_buf[5] << 16) |
                            (uart2_rx_buf[6] << 8) |
                            uart2_rx_buf[7];
        if (fft_size >= FFT_MIN_SIZE && fft_size <= FFT_MAX_SIZE) {
            state->fft_size = fft_size;
        }
        break;
    }

    case CMD_SET_ATTEN: {
        uint8_t atten = uart2_rx_buf[4];
        if (atten == 0 || atten == 10 || atten == 20) {
            state->attenuation = atten;
            /* Update GPIO for attenuator */
            if (atten >= 10) SET_BIT(GPIOB->ODR, PIN_ATTEN_SEL);
            else CLR_BIT(GPIOB->ODR, PIN_ATTEN_SEL);
        }
        break;
    }

    case CMD_RESET:
        /* Software reset via NVIC */
        NVIC_SystemReset();
        break;

    default:
        break;
    }

    g_ble_rx_ready = 0;
}

/* ========================================================================== */
/* Debug printf via USART1                                                     */
/* ========================================================================== */
static void debug_puts(const char *str)
{
    while (*str) {
        while (!(USART1->ISR & USART_ISR_TXE))
            ;
        USART1->TDR = *str++;
    }
}

/* ========================================================================== */
/* Main entry point                                                            */
/* ========================================================================== */
int main(void)
{
    /* Step 1: Clock initialization */
    clock_init();

    /* Step 2: GPIO initialization */
    gpio_init();

    /* Step 3: Peripheral initialization */
    spi1_init();
    spi2_init();
    spi3_init();
    usart1_init();
    usart2_init();
    timer_init();
    dac_init();
    adc_init();

    /* Step 4: Power-on sequence */
    power_init();

    /* Step 5: FPGA initialization */
    if (fpga_init() != 0) {
        debug_puts("FPGA init failed!\r\n");
        /* Continue without FPGA - degraded mode */
    }

    /* Step 6: ADC initialization */
    ad9645_init();

    /* Step 7: PLL initialization */
    adf4351_init();

    /* Step 8: Display initialization */
    ili9341_init();
    ili9341_fill_rect(0, 0, ILI9341_WIDTH, ILI9341_HEIGHT, COLOR_BLACK);
    ili9341_draw_text(80, 100, "VORTEX-SDR", COLOR_WHITE, COLOR_BLACK);
    ili9341_draw_text(60, 120, "Initializing...", COLOR_CYAN, COLOR_BLACK);

    /* Step 9: BLE initialization */
    /* (nRF52832 reset via UART break + AT commands) */

    /* Step 10: Initialize default sweep state */
    sweep_state_t sweep;
    sweep_state_init(&sweep);

    /* Step 11: Display ready screen */
    ili9341_fill_rect(0, 0, ILI9341_WIDTH, ILI9341_HEIGHT, COLOR_BLACK);
    ili9341_draw_text(80, 100, "VORTEX-SDR", COLOR_WHITE, COLOR_BLACK);
    ili9341_draw_text(70, 120, "Ready to sweep", COLOR_GREEN, COLOR_BLACK);

    /* Debug output */
    debug_puts("Vortex-SDR v1.0 initialized\r\n");
    debug_puts("Clock: 480 MHz SYSCLK\r\n");
    debug_puts("FPGA: configured\r\n");
    debug_puts("PLL: initialized\r\n");

    /* Enable status LED */
    SET_BIT(GPIOC->ODR, PIN_LED_STATUS);

    /* Main super-loop */
    while (1) {
        /* Poll for new FFT data from FPGA */
        if (g_fft_data_ready) {
            fft_process_bins(&sweep);
            g_fft_data_ready = 0;
        }

        /* Update display (50 ms timer) */
        if (g_display_update) {
            display_update(&sweep);
            g_display_update = 0;
        }

        /* Handle BLE commands */
        if (g_ble_rx_ready) {
            ble_process_command(&sweep);
        }

        /* Handle button/touch input */
        if (g_ui_event) {
            ui_handle_event(&sweep);
        }

        /* Read battery voltage (every 1 second) */
        if ((g_tick_ms % 1000) == 0) {
            sweep.battery_mv = read_battery_mv();
        }

        /* Low-power wait for interrupt */
        __WFI();
    }
}

/* ========================================================================== */
/* Hard fault handler — blink error LED and log fault info                    */
/* ========================================================================== */
void HardFault_Handler(void)
{
    uint32_t *stack_ptr;
    __asm volatile (
        "TST LR, #4 \n"
        "ITE EQ \n"
        "MRSEQ %0, MSP \n"
        "MRSNE %0, PSP \n"
        : "=r" (stack_ptr)
    );

    g_fault_info.r0 = stack_ptr[0];
    g_fault_info.r1 = stack_ptr[1];
    g_fault_info.r2 = stack_ptr[2];
    g_fault_info.r3 = stack_ptr[3];
    g_fault_info.pc = stack_ptr[6];
    g_fault_info.lr = stack_ptr[5];
    g_fault_info.psr = stack_ptr[7];

    /* Blink red LED rapidly */
    while (1) {
        GPIOC->ODR ^= (1UL << PIN_LED_ERROR);
        for (volatile uint32_t i = 0; i < 500000; i++)
            ;
    }
}

/* ========================================================================== */
/* System tick handler (1 ms)                                                  */
/* ========================================================================== */
void TIM6_DAC_IRQHandler(void)
{
    if (TIM6->SR & TIM_SR_UIF) {
        TIM6->SR = ~TIM_SR_UIF;
        g_tick_ms++;
    }
}

/* ========================================================================== */
/* FFT poll timer handler (10 ms)                                              */
/* ========================================================================== */
void TIM7_IRQHandler(void)
{
    if (TIM7->SR & TIM_SR_UIF) {
        TIM7->SR = ~TIM_SR_UIF;
        g_fft_data_ready = 1;
    }
}

/* ========================================================================== */
/* Display update timer handler (50 ms)                                        */
/* ========================================================================== */
void TIM1_UP_IRQHandler(void)
{
    if (TIM1->SR & TIM_SR_UIF) {
        TIM1->SR = ~TIM_SR_UIF;
        g_display_update = 1;
    }
}

/* ========================================================================== */
/* EXTI handler (buttons)                                                      */
/* ========================================================================== */
void EXTI0_IRQHandler(void)
{
    if (EXTI->PR1 & (1UL << PIN_BTN_CENTER)) {
        EXTI->PR1 = (1UL << PIN_BTN_CENTER);
        g_button_state |= BTN_CENTER;
        g_ui_event = 1;
    }
}

void EXTI9_5_IRQHandler(void)
{
    uint32_t pr = EXTI->PR1;
    if (pr & (1UL << 7)) {  /* PB7: TOUCH_IRQ */
        EXTI->PR1 = (1UL << 7);
        g_ui_event = 1;
    }
}

/* ========================================================================== */
/* NVIC System Reset (for BLE reset command)                                   */
/* ========================================================================== */
static inline void NVIC_SystemReset(void)
{
    volatile uint32_t *aircr = (volatile uint32_t *)0xE000ED0CUL;
    *aircr = (0x5FAUL << 16) | (1UL << 2);
    while (1)
        ;
}