/*
 * main.c — Chronos-RTK SPL/board init entry point
 * STM32G474RET6 @ 170 MHz, dual-frequency RTK GNSS + LoRa
 */

#include <string.h>
#include <stdint.h>
#include "board.h"
#include "registers.h"
#include "drivers/sx1262.h"
#include "drivers/w25q128.h"
#include "drivers/ssd1306.h"

/* ========================================================================== */
/* DMA buffers                                                                */
/* ========================================================================== */
#define UART1_RX_BUF_SIZE  4096
static volatile uint8_t uart1_rx_buf[UART1_RX_BUF_SIZE];
static volatile uint16_t uart1_rx_head = 0;
static volatile uint16_t uart1_rx_tail = 0;

/* GNSS sentence buffer */
#define GNSS_BUF_SIZE  512
static uint8_t gnss_buf[GNSS_BUF_SIZE];
static uint16_t gnss_idx = 0;

/* System state */
typedef enum {
    STATE_INIT = 0,
    STATE_ACQUIRING,
    STATE_2D_FIX,
    STATE_3D_FIX,
    STATE_RTK_FLOAT,
    STATE_RTK_FIXED
} gnss_state_t;

static volatile gnss_state_t g_gnss_state = STATE_INIT;
static volatile uint32_t g_pps_count = 0;
static volatile uint32_t g_lora_irq_flag = 0;

/* ========================================================================== */
/* Clock initialization (HSE 32 MHz → PLL → 170 MHz)                          */
/* ========================================================================== */
static void clock_init(void)
{
    /* Enable HSE (32 MHz crystal) */
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY))
        ;

    /* Configure PLL: HSE/4 * 85 / 4 = 170 MHz */
    RCC->PLLCFGR = (4U << RCC_PLLCFGR_PLLM_Pos)    /* PLLM = 4 */
                 | (85U << RCC_PLLCFGR_PLLN_Pos)     /* PLLN = 85 */
                 | (0U << RCC_PLLCFGR_PLLP_Pos)       /* PLLP = 2 (000=2) */
                 | RCC_PLLCFGR_PLLSRC_HSE
                 | (7U << 28);                        /* PLLQ = 8 (for USB, not used from PLL) */

    /* Enable PLL */
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY))
        ;

    /* Configure flash latency: 5 WS for 170 MHz @ 3.3V */
    FLASH->ACR = FLASH_ACR_DCEN | FLASH_ACR_ICEN | FLASH_ACR_PRFTEN
               | (5U << FLASH_ACR_LATENCY_Pos);

    /* Switch SYSCLK to PLL */
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL)
        ;

    /* Enable HSI48 for USB (48 MHz) */
    RCC->CRRC |= (1U << 0);  /* HSI48ON */
    while (!(RCC->CRRC & (1U << 1)))  /* HSI48RDY */
        ;

    /* Enable LSE for RTC (32.768 kHz) */
    RCC->BDCR |= (1U << 0);  /* LSEON */
    while (!(RCC->BDCR & (1U << 1)))  /* LSERDY */
        ;

    /* Enable peripheral clocks */
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN
                  | RCC_AHB2ENR_GPIOCEN | RCC_AHB2ENR_GPIODEN;
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN | RCC_AHB1ENR_CRCEN;
    RCC->APB1ENR1 |= RCC_APB1ENR1_USART1EN | RCC_APB1ENR1_SPI2EN
                   | RCC_APB1ENR1_I2C1EN | RCC_APB1ENR1_TIM2EN
                   | RCC_APB1ENR1_TIM6EN;
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN | RCC_APB2ENR_SYSCFGEN;
    RCC->APB1ENR2 |= RCC_APB1ENR2_USBEN;
}

/* ========================================================================== */
/* GPIO initialization                                                         */
/* ========================================================================== */
static void gpio_init(void)
{
    /* --- Power control --- */
    /* PB3: POWER_EN (enable 3.3V rail) */
    GPIOB->MODER = (GPIOB->MODER & ~(3U << (2*3))) | (1U << (2*3));
    GPIOB->OSPEEDR |= (3U << (2*3));
    GPIOB->ODR |= (1U << 3);  /* Enable 3.3V */

    /* --- UART1 (PA2=TX, PA3=RX) AF7 --- */
    GPIOA->MODER = (GPIOA->MODER & ~(3U << (2*2))) | (2U << (2*2));
    GPIOA->AFR[0] = (GPIOA->AFR[0] & ~(0xFU << (4*2))) | (7U << (4*2));
    GPIOA->OSPEEDR |= (3U << (2*2));
    GPIOA->MODER = (GPIOA->MODER & ~(3U << (2*3))) | (2U << (2*3));
    GPIOA->AFR[0] = (GPIOA->AFR[0] & ~(0xFU << (4*3))) | (7U << (4*3));
    GPIOA->PUPDR = (GPIOA->PUPDR & ~(3U << (2*3))) | (1U << (2*3));

    /* --- SPI1 (PA4=NSS SW, PA5=SCK, PA6=MISO, PA7=MOSI) --- */
    /* PA4: GPIO output (software CS) */
    GPIOA->MODER = (GPIOA->MODER & ~(3U << (2*4))) | (1U << (2*4));
    GPIOA->ODR |= (1U << 4);  /* CS# high = deselected */
    GPIOA->OSPEEDR |= (3U << (2*4));
    /* PA5: AF5 SPI1_SCK */
    GPIOA->MODER = (GPIOA->MODER & ~(3U << (2*5))) | (2U << (2*5));
    GPIOA->AFR[0] = (GPIOA->AFR[0] & ~(0xFU << (4*5))) | (5U << (4*5));
    GPIOA->OSPEEDR |= (3U << (2*5));
    /* PA6: AF5 SPI1_MISO */
    GPIOA->MODER = (GPIOA->MODER & ~(3U << (2*6))) | (2U << (2*6));
    GPIOA->AFR[0] = (GPIOA->AFR[0] & ~(0xFU << (4*6))) | (5U << (4*6));
    /* PA7: AF5 SPI1_MOSI */
    GPIOA->MODER = (GPIOA->MODER & ~(3U << (2*7))) | (2U << (2*7));
    GPIOA->AFR[0] = (GPIOA->AFR[0] & ~(0xFU << (4*7))) | (5U << (4*7));
    GPIOA->OSPEEDR |= (3U << (2*7));

    /* --- SPI2 (PA0=SCK, PA1=NSS SW, PC3=MOSI) --- */
    /* PA0: AF5 SPI2_SCK */
    GPIOA->MODER = (GPIOA->MODER & ~(3U << (2*0))) | (2U << (2*0));
    GPIOA->AFR[0] = (GPIOA->AFR[0] & ~(0xFU << (4*0))) | (5U << (4*0));
    GPIOA->OSPEEDR |= (3U << (2*0));
    /* PA1: GPIO output (software CS for SX1262) */
    GPIOA->MODER = (GPIOA->MODER & ~(3U << (2*1))) | (1U << (2*1));
    GPIOA->ODR |= (1U << 1);  /* NSS high = deselected */
    GPIOA->OSPEEDR |= (3U << (2*1));
    /* PC3: AF5 SPI2_MOSI */
    GPIOC->MODER = (GPIOC->MODER & ~(3U << (2*3))) | (2U << (2*3));
    GPIOC->AFR[0] = (GPIOC->AFR[0] & ~(0xFU << (4*3))) | (5U << (4*3));

    /* --- I2C1 (PB10=SCL, PB11=SDA) AF4 open-drain --- */
    GPIOB->MODER = (GPIOB->MODER & ~(3U << (2*10))) | (2U << (2*10));
    GPIOB->MODER = (GPIOB->MODER & ~(3U << (2*11))) | (2U << (2*11));
    GPIOB->AFR[1] = (GPIOB->AFR[1] & ~(0xFU << (4*2))) | (4U << (4*2));
    GPIOB->AFR[1] = (GPIOB->AFR[1] & ~(0xFU << (4*3))) | (4U << (4*3));
    GPIOB->OSPEEDR |= (3U << (2*10)) | (3U << (2*11));
    GPIOB->OTYPER |= (1U << 10) | (1U << 11);
    GPIOB->PUPDR = (GPIOB->PUPDR & ~(3U << (2*10))) | (1U << (2*10));
    GPIOB->PUPDR = (GPIOB->PUPDR & ~(3U << (2*11))) | (1U << (2*11));

    /* --- USB (PA11=DM, PA12=DP) AF10 --- */
    GPIOA->MODER = (GPIOA->MODER & ~(3U << (2*11))) | (2U << (2*11));
    GPIOA->MODER = (GPIOA->MODER & ~(3U << (2*12))) | (2U << (2*12));
    GPIOA->AFR[1] = (GPIOA->AFR[1] & ~(0xFU << (4*3))) | (0xAU << (4*3));
    GPIOA->AFR[1] = (GPIOA->AFR[1] & ~(0xFU << (4*4))) | (0xAU << (4*4));

    /* --- LEDs (PB12=R, PB13=G, PB14=B) push-pull output --- */
    GPIOB->MODER = (GPIOB->MODER & ~(3U << (2*12))) | (1U << (2*12));
    GPIOB->MODER = (GPIOB->MODER & ~(3U << (2*13))) | (1U << (2*13));
    GPIOB->MODER = (GPIOB->MODER & ~(3U << (2*14))) | (1U << (2*14));
    LED_R_OFF(); LED_G_OFF(); LED_B_OFF();

    /* --- LoRa control pins --- */
    /* PC4: LORA_BUSY (input) */
    GPIOC->MODER = (GPIOC->MODER & ~(3U << (2*4))) | (0U << (2*4));
    /* PC5: LORA_RST (output, high = not in reset) */
    GPIOC->MODER = (GPIOC->MODER & ~(3U << (2*5))) | (1U << (2*5));
    GPIOC->ODR |= (1U << 5);
    /* PB0: LORA_DIO1 (input, EXTI0) */
    GPIOB->MODER = (GPIOB->MODER & ~(3U << (2*0))) | (0U << (2*0));
    EXTI->IMR1 |= (1U << 0);
    EXTI->RTSR1 |= (1U << 0);
    EXTI->FTSR1 |= (1U << 0);
    SYSCFG->EXTICR[0] = (SYSCFG->EXTICR[0] & ~(0xFU << 0)) | (1U << 0);  /* PB0 */

    /* --- GNSS control --- */
    /* PB2: GNSS_RST (output, high = running) */
    GPIOB->MODER = (GPIOB->MODER & ~(3U << (2*2))) | (1U << (2*2));
    GPIOB->ODR |= (1U << 2);
    /* PB15: GNSS_PPS (input) */
    GPIOB->MODER = (GPIOB->MODER & ~(3U << (2*15))) | (0U << (2*15));
    /* PC7: GNSS_SAFEBOOT (output, high = normal) */
    GPIOC->MODER = (GPIOC->MODER & ~(3U << (2*7))) | (1U << (2*7));
    GPIOC->ODR |= (1U << 7);
    /* PB7: OLED_RST (output, high = running) */
    GPIOB->MODER = (GPIOB->MODER & ~(3U << (2*7))) | (1U << (2*7));
    GPIOB->ODR |= (1U << 7);

    /* --- Buttons (input, active low) --- */
    GPIOC->MODER &= ~(3U << (2*14)) & ~(3U << (2*15)) & ~(3U << (2*2));

    /* --- Charge status (PB5, PB6 input) --- */
    GPIOB->MODER = (GPIOB->MODER & ~(3U << (2*5))) & ~(3U << (2*6));
    /* PB4: CHARGE_EN (output, active low) */
    GPIOB->MODER = (GPIOB->MODER & ~(3U << (2*4))) | (1U << (2*4));
    GPIOB->ODR &= ~(1U << 4);  /* Enable charger (active low) */

    /* --- Debug (PA13=SWDIO, PA14=SWCLK) - default AF after reset --- */
}

/* ========================================================================== */
/* USART1 initialization (460800 baud, 8N1, DMA RX)                           */
/* ========================================================================== */
static void usart1_init(void)
{
    USART_TypeDef *usart = (USART_TypeDef *)USART1_BASE;

    /* Disable USART */
    usart->CR1 = 0;

    /* Baud rate: 170000000 / 460800 ≈ 369 (oversampling 16) */
    usart->BRR = 170000000UL / 460800UL;  /* ≈ 369 */

    /* 8 data bits, no parity, 1 stop bit */
    usart->CR1 = USART_CR1_TE | USART_CR1_RE;

    /* Enable DMA for RX */
    usart->CR3 |= (1U << 6);  /* DMAR bit */

    /* Configure DMA1 Channel 5 for USART1_RX */
    DMA1_CH5->CPAR = (uint32_t)&usart->RDR;
    DMA1_CH5->CMAR = (uint32_t)uart1_rx_buf;
    DMA1_CH5->CNDTR = UART1_RX_BUF_SIZE;
    DMA1_CH5->CCR = (1U << 7)   /* MINC: memory increment */
                   | (1U << 5)   /* CIRC: circular mode */
                   | (1U << 3)   /* TEIE: transfer error interrupt enable */
                   | (1U << 1);  /* EN: channel enable */

    /* Enable USART */
    usart->CR1 |= USART_CR1_UE;
}

/* ========================================================================== */
/* SPI1 initialization (50 MHz, mode 0, master)                               */
/* ========================================================================== */
static void spi1_init(void)
{
    SPI_TypeDef *spi = (SPI_TypeDef *)SPI1_BASE;

    /* Disable SPI */
    spi->CR1 = 0;

    /* Master mode, clock = APB2/2 = 85 MHz (still within W25Q128 spec) */
    /* BR[2:0] = 000 → fPCLK/2 */
    spi->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI
              | (0U << SPI_CR1_BR_Pos);  /* fPCLK/2 */

    /* 8-bit data, MSB first, full duplex */
    spi->CR2 = (7U << SPI_CR2_DS_Pos)  /* 8-bit data size */
             | SPI_CR2_FRXTH;            /* FIFO threshold = 8 bits */

    /* Enable SPI */
    spi->CR1 |= SPI_CR1_SPE;
}

/* ========================================================================== */
/* SPI2 initialization (16 MHz, mode 0, master)                               */
/* ========================================================================== */
static void spi2_init(void)
{
    SPI_TypeDef *spi = (SPI_TypeDef *)SPI2_BASE;

    /* Disable SPI */
    spi->CR1 = 0;

    /* Master mode, clock = APB1/8 ≈ 21 MHz (within SX1262 spec of 16 MHz) */
    /* BR[2:0] = 010 → fPCLK/8 ≈ 21.25 MHz */
    spi->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI
              | (2U << SPI_CR1_BR_Pos);  /* fPCLK/8 */

    /* 8-bit data, MSB first, full duplex */
    spi->CR2 = (7U << SPI_CR2_DS_Pos) | SPI_CR2_FRXTH;

    /* Enable SPI */
    spi->CR1 |= SPI_CR1_SPE;
}

/* ========================================================================== */
/* I2C1 initialization (400 kHz fast mode)                                    */
/* ========================================================================== */
static void i2c1_init(void)
{
    I2C_TypeDef *i2c = (I2C_TypeDef *)I2C1_BASE;

    /* Disable I2C */
    i2c->CR1 = 0;

    /* Timing register for 400 kHz @ 170 MHz (from STM32Cube calculations) */
    /* PRESC=0, SCLDEL=3, SDADEL=0, SCLH=16, SCLL=28 */
    i2c->TIMINGR = (0U << 28)  /* PRESC */
                 | (3U << 20)  /* SCLDEL */
                 | (0U << 16)  /* SDADEL */
                 | (16U << 8)  /* SCLH */
                 | (28U << 0); /* SCLL */

    /* Enable I2C */
    i2c->CR1 = I2C_CR1_PE;
}

/* ========================================================================== */
/* CRC initialization (CRC-32, used for flash integrity)                       */
/* ========================================================================== */
static void crc_init(void)
{
    CRC->CR = CRC_CR_RESET;
    CRC->POL = 0x04C11DB7UL;  /* CRC-32 polynomial */
    CRC->INIT = 0xFFFFFFFFUL;
}

/* ========================================================================== */
/* EXTI0 IRQ handler (LoRa DIO1)                                              */
/* ========================================================================== */
void EXTI0_IRQHandler(void)
{
    if (EXTI->PR1 & (1U << 0)) {
        EXTI->PR1 = (1U << 0);  /* Clear pending */
        g_lora_irq_flag = 1;
    }
}

/* ========================================================================== */
/* TIM2 IRQ handler (PPS input from GNSS)                                     */
/* ========================================================================== */
void TIM2_IRQHandler(void)
{
    /* Clear update interrupt */
    ((TIM_TypeDef *)TIM2_BASE)->SR = ~(1U << 0);
    g_pps_count++;
}

/* ========================================================================== */
/* Main loop                                                                  */
/* ========================================================================== */
int main(void)
{
    /* Initialize clocks */
    clock_init();

    /* Initialize GPIO */
    gpio_init();

    /* Enable 3.3V rail (already done in gpio_init) */
    /* Small delay for power stabilization */
    for (volatile int i = 0; i < 100000; i++)
        ;

    /* Initialize peripherals */
    spi1_init();   /* Flash */
    spi2_init();   /* LoRa */
    i2c1_init();   /* OLED + BME280 */
    usart1_init(); /* GNSS */
    crc_init();

    /* Initialize drivers */
    w25q128_init();
    sx1262_init();
    ssd1306_init();

    /* Display splash screen */
    ssd1306_clear();
    ssd1306_draw_string(0, 0, "Chronos-RTK v1.0", 1);
    ssd1306_draw_string(0, 2, "Initializing...", 1);
    ssd1306_display();

    /* Configure GNSS (send UBX messages to enable RTCM, set rate, etc.) */
    /* In a production firmware, these would be proper UBX binary frames */
    /* For SPL, we send UBX-CFG-VALSET commands via UART1 */

    /* Configure SX1262 for LoRa */
    sx1262_set_frequency(LORA_FREQ_EU);
    sx1262_set_modulation_params(SF7, BW_125KHZ, CR_4_5);
    sx1262_set_packet_params(8, HEADER_EXPLICIT, LORA_MAX_PAYLOAD, CRC_ON);
    sx1262_set_tx_power(22);  /* +22 dBm */
    sx1262_set_sync_word(LORA_SYNC_WORD);

    /* Start LoRa RX */
    sx1262_set_rx(0);  /* Continuous RX */

    /* Enable NVIC interrupts */
    NVIC->ISER[0] = (1U << 6);   /* EXTI0 (LoRa DIO1) */

    /* Set initial state */
    g_gnss_state = STATE_ACQUIRING;
    LED_R_ON();  /* Red = acquiring */

    /* ======== Main loop ======== */
    while (1) {
        /* Process UART1 DMA circular buffer for GNSS data */
        uint16_t head = UART1_RX_BUF_SIZE - DMA1_CH5->CNDTR;
        while (uart1_rx_tail != head) {
            uint8_t byte = uart1_rx_buf[uart1_rx_tail];
            uart1_rx_tail = (uart1_rx_tail + 1) % UART1_RX_BUF_SIZE;

            /* Process byte through NMEA parser */
            gnss_buf[gnss_idx++] = byte;
            if (gnss_idx >= GNSS_BUF_SIZE) gnss_idx = 0;

            /* Check for complete NMEA sentence */
            if (byte == '\n') {
                gnss_buf[gnss_idx] = '\0';
                /* Forward NMEA to USB CDC-ACM (if connected) */
                /* Update GNSS state based on GGA/GSA sentences */
                gnss_idx = 0;
            }
        }

        /* Process LoRa interrupt */
        if (g_lora_irq_flag) {
            g_lora_irq_flag = 0;
            sx1262_handle_irq();
        }

        /* Update OLED display (every 100 ms) */
        static uint32_t display_timer = 0;
        if (++display_timer > 1700000) {  /* ~100ms at 170 MHz loop */
            display_timer = 0;
            ssd1306_clear();
            switch (g_gnss_state) {
                case STATE_ACQUIRING:
                    ssd1306_draw_string(0, 0, "Acquiring...", 1);
                    break;
                case STATE_2D_FIX:
                    ssd1306_draw_string(0, 0, "2D Fix", 1);
                    break;
                case STATE_3D_FIX:
                    ssd1306_draw_string(0, 0, "3D Fix", 1);
                    break;
                case STATE_RTK_FLOAT:
                    ssd1306_draw_string(0, 0, "RTK Float", 1);
                    LED_G_ON(); LED_R_OFF();
                    break;
                case STATE_RTK_FIXED:
                    ssd1306_draw_string(0, 0, "RTK Fixed!", 1);
                    LED_G_ON(); LED_R_OFF(); LED_B_ON();
                    break;
                default:
                    break;
            }
            ssd1306_display();
        }

        /* Log raw observations to SPI flash (periodic) */
        /* In production: write to W25Q128 circular buffer */

        /* Handle button presses */
        if (!(GPIOC->IDR & (1U << PIN_BTN_MODE))) {
            /* Mode button pressed - toggle display mode */
        }
        if (!(GPIOC->IDR & (1U << PIN_BTN_ACT))) {
            /* Action button pressed - log waypoint */
        }

        /* Monitor battery voltage via ADC */
        /* In production: read PC0 (VBAT_SENSE) */
    }

    return 0;
}