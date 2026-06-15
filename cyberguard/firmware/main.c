/*
 * main.c - CyberGuard SPL/Board Init Entry Point
 * STM32L562QE Multi-Protocol Hardware Security Token
 *
 * Boot sequence:
 *   1. Reset handler (startup.s) → SystemInit()
 *   2. Clock configuration (HSI16 → PLL → 110 MHz)
 *   3. Power management (STPMIC1 enable rails)
 *   4. GPIO initialization
 *   5. Peripheral initialization (I2C, SPI, UART)
 *   6. Secure element probe (A71CH)
 *   7. NFC controller init (PN7150)
 *   8. Fingerprint sensor init (FPC1025)
 *   9. BLE coprocessor init (nRF52833)
 *  10. USB CTAP2 device init
 *  11. Main loop: event dispatch
 */

#include "board.h"
#include "registers.h"
#include <string.h>

/* ============================================================
 * External symbols from linker script
 * ============================================================ */
extern uint32_t _estack;
extern uint32_t _sidata;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;

/* ============================================================
 * Driver headers
 * ============================================================ */
#include "drivers/i2c_se.h"
#include "drivers/spi_nfc.h"
#include "drivers/uart_fp.h"
#include "drivers/spi_flash.h"
#include "drivers/pmic.h"

/* ============================================================
 * Global state
 * ============================================================ */
static volatile uint32_t systick_ms = 0;
static volatile uint8_t  usb_connected = 0;
static volatile uint8_t  nfc_field_active = 0;
static volatile uint8_t  fp_touch_detected = 0;
static volatile uint8_t  button_pressed = 0;

typedef enum {
    STATE_IDLE,
    STATE_USB_AUTH,
    STATE_NFC_AUTH,
    STATE_BLE_AUTH,
    STATE_ENROLLING,
    STATE_UPDATING,
    STATE_ERROR
} system_state_t;

static volatile system_state_t system_state = STATE_IDLE;

/* ============================================================
 * Delay using SysTick
 * ============================================================ */
static void delay_ms(uint32_t ms)
{
    uint32_t start = systick_ms;
    while ((systick_ms - start) < ms) {
        __asm__("wfi");
    }
}

/* ============================================================
 * SysTick Handler (1ms interval)
 * ============================================================ */
void SysTick_Handler(void)
{
    systick_ms++;
}

/* ============================================================
 * EXTI Handler (Button, NFC IRQ, SE IRQ, FP IRQ)
 * ============================================================ */
void EXTI5_9_IRQHandler(void)
{
    /* Check NFC IRQ (PC2 = EXTI2) */
    if (EXTI->RPR1 & (1U << 2)) {
        EXTI->RPR1 = (1U << 2);
        nfc_field_active = 1;
    }
}

void EXTI10_15_IRQHandler(void)
{
    /* Check SE IRQ (PC4 = EXTI4 → falls in 10-15 as PC4 would map differently) */
    /* Check button press (if mapped to EXTI line) */
    if (EXTI->RPR1 & (1U << 13)) {
        EXTI->RPR1 = (1U << 13);
        /* PC13 is not EXTI13 for STM32L5 - this would be PA13 */
    }
}

/* ============================================================
 * Clock Configuration
 * HSI16 → PLL → 110 MHz SYSCLK
 * ============================================================ */
static void clock_init(void)
{
    /* Enable power interface clock */
    RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;

    /* Set voltage range to Range 0 for 110 MHz */
    PWR->CR1 &= ~(3U << 9);  /* VOS = 00 = Range 0 */

    /* Wait for voltage regulator to be ready */
    while (!(PWR->SR2 & (1U << 13))); /* VOSF bit */

    /* Enable HSI16 */
    RCC->CR |= RCC_CR_HSION;
    while (!(RCC->CR & RCC_CR_HSIRDY));

    /* Configure PLL: HSI16 → 110 MHz
     * PLLM = 1, PLLN = 55, PLLR = 2
     * VCO = 16 MHz / 1 * 55 = 880 MHz
     * PLLR output = 880 / 2 / 4 = 110 MHz (with PLLR=2, main PLL output divider /4 for 110)
     */
    RCC->PLLCFGR = (1U << 4)    /* PLLM = 1 */
                  | (55U << 8)   /* PLLN = 55 */
                  | (2U << 16)   /* PLLR = 2 */
                  | (5U << 21)   /* PLLQ = 5 */
                  | (7U << 27)   /* PLLP = 7 */
                  | (2U << 0);   /* PLLSRC = HSI16 */

    /* Enable PLL */
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY));

    /* Enable PLL output (PLLRCLK) */
    RCC->PLLCFGR |= (1U << 24); /* PLLREN */

    /* Set flash wait states for 110 MHz (5 WS) */
    FLASH->ACR &= ~(7U << 0);
    FLASH->ACR |= (5U << 0);    /* 5 wait states */
    FLASH->ACR |= (1U << 8);    /* Instruction cache enable */
    FLASH->ACR |= (1U << 9);    /* Data cache enable */
    FLASH->ACR |= (1U << 10);   /* Prefetch enable */

    /* Switch system clock to PLL */
    RCC->CFGR &= ~(3U << 0);
    RCC->CFGR |= RCC_CFGR_SW_PLL;

    /* Wait for PLL as system clock */
    while ((RCC->CFGR & RCC_CFGR_SWS_MASK) != RCC_CFGR_SWS_PLL);

    /* Update SystemCoreClock variable */
    /* SystemCoreClock = 110000000UL; */
}

/* ============================================================
 * GPIO Initialization
 * ============================================================ */
static void gpio_init(void)
{
    /* Enable GPIO clocks */
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN
                  | RCC_AHB2ENR_GPIOBEN
                  | RCC_AHB2ENR_GPIOCEN
                  | RCC_AHB2ENR_GPIOHEN;

    /* Small delay for GPIO clock stabilization */
    for (volatile int i = 0; i < 10; i++);

    /* --- GPIOA Configuration --- */
    /* PA0  = LED_RED (output, push-pull, low speed) */
    GPIOA->MODER &= ~(3U << (PIN_LED_RED * 2));
    GPIOA->MODER |= (GPIO_MODE_OUTPUT << (PIN_LED_RED * 2));
    GPIOA->OTYPER &= ~(1U << PIN_LED_RED);
    GPIOA->OSPEEDR &= ~(3U << (PIN_LED_RED * 2));

    /* PA1  = LED_BLUE (output, push-pull, low speed) */
    GPIOA->MODER &= ~(3U << (PIN_LED_BLUE * 2));
    GPIOA->MODER |= (GPIO_MODE_OUTPUT << (PIN_LED_BLUE * 2));
    GPIOA->OTYPER &= ~(1U << PIN_LED_BLUE);
    GPIOA->OSPEEDR &= ~(3U << (PIN_LED_BLUE * 2));

    /* PA2  = USART2_TX (AF7) - BLE */
    GPIOA->MODER &= ~(3U << (PIN_BLE_TX * 2));
    GPIOA->MODER |= (GPIO_MODE_AF << (PIN_BLE_TX * 2));
    GPIOA->AFR[0] &= ~(0xFU << (PIN_BLE_TX * 4));
    GPIOA->AFR[0] |= (GPIO_AF7_USART2 << (PIN_BLE_TX * 4));

    /* PA3  = USART2_RX (AF7) - BLE */
    GPIOA->MODER &= ~(3U << (PIN_BLE_RX * 2));
    GPIOA->MODER |= (GPIO_MODE_AF << (PIN_BLE_RX * 2));
    GPIOA->AFR[0] &= ~(0xFU << (PIN_BLE_RX * 4));
    GPIOA->AFR[0] |= (GPIO_AF7_USART2 << (PIN_BLE_RX * 4));

    /* PA4  = SPI1_NSS (output, push-pull, high speed) - NFC CS */
    GPIOA->MODER &= ~(3U << (PIN_NFC_NSS * 2));
    GPIOA->MODER |= (GPIO_MODE_OUTPUT << (PIN_NFC_NSS * 2));
    GPIOA->OTYPER &= ~(1U << PIN_NFC_NSS);
    GPIOA->OSPEEDR |= (GPIO_SPEED_VERY_HIGH << (PIN_NFC_NSS * 2));
    GPIOA->ODR |= (1U << PIN_NFC_NSS); /* CS high (deselected) */

    /* PA5  = SPI1_SCK (AF5) - NFC SPI clock */
    GPIOA->MODER &= ~(3U << (PIN_NFC_SCK * 2));
    GPIOA->MODER |= (GPIO_MODE_AF << (PIN_NFC_SCK * 2));
    GPIOA->AFR[0] &= ~(0xFU << (PIN_NFC_SCK * 4));
    GPIOA->AFR[0] |= (GPIO_AF5_SPI1 << (PIN_NFC_SCK * 4));
    GPIOA->OSPEEDR |= (GPIO_SPEED_VERY_HIGH << (PIN_NFC_SCK * 2));

    /* PA6  = SPI1_MISO (AF5) - NFC SPI MISO */
    GPIOA->MODER &= ~(3U << (PIN_NFC_MISO * 2));
    GPIOA->MODER |= (GPIO_MODE_AF << (PIN_NFC_MISO * 2));
    GPIOA->AFR[0] &= ~(0xFU << (PIN_NFC_MISO * 4));
    GPIOA->AFR[0] |= (GPIO_AF5_SPI1 << (PIN_NFC_MISO * 4));

    /* PA7  = SPI1_MOSI (AF5) - NFC SPI MOSI */
    GPIOA->MODER &= ~(3U << (PIN_NFC_MOSI * 2));
    GPIOA->MODER |= (GPIO_MODE_AF << (PIN_NFC_MOSI * 2));
    GPIOA->AFR[0] &= ~(0xFU << (PIN_NFC_MOSI * 4));
    GPIOA->AFR[0] |= (GPIO_AF5_SPI1 << (PIN_NFC_MOSI * 4));

    /* PA11 = USB_DM (AF10) */
    GPIOA->MODER &= ~(3U << (PIN_USB_DM * 2));
    GPIOA->MODER |= (GPIO_MODE_AF << (PIN_USB_DM * 2));
    GPIOA->AFR[1] &= ~(0xFU << ((PIN_USB_DM - 8) * 4));
    GPIOA->AFR[1] |= (GPIO_AF10_USB << ((PIN_USB_DM - 8) * 4));

    /* PA12 = USB_DP (AF10) */
    GPIOA->MODER &= ~(3U << (PIN_USB_DP * 2));
    GPIOA->MODER |= (GPIO_MODE_AF << (PIN_USB_DP * 2));
    GPIOA->AFR[1] &= ~(0xFU << ((PIN_USB_DP - 8) * 4));
    GPIOA->AFR[1] |= (GPIO_AF10_USB << ((PIN_USB_DP - 8) * 4));

    /* PA15 = FLASH_WP (output, push-pull) */
    GPIOA->MODER &= ~(3U << (PIN_FLASH_WP * 2));
    GPIOA->MODER |= (GPIO_MODE_OUTPUT << (PIN_FLASH_WP * 2));
    GPIOA->ODR |= (1U << PIN_FLASH_WP); /* Write protect off initially */

    /* --- GPIOB Configuration --- */
    /* PB0  = I2C1_SCL (AF4) - SE */
    GPIOB->MODER &= ~(3U << (PIN_SE_SCL * 2));
    GPIOB->MODER |= (GPIO_MODE_AF << (PIN_SE_SCL * 2));
    GPIOB->AFR[0] &= ~(0xFU << (PIN_SE_SCL * 4));
    GPIOB->AFR[0] |= (GPIO_AF4_I2C1 << (PIN_SE_SCL * 4));
    GPIOB->OTYPER |= (1U << PIN_SE_SCL); /* Open-drain */
    GPIOB->OSPEEDR |= (GPIO_SPEED_HIGH << (PIN_SE_SCL * 2));
    GPIOB->PUPDR &= ~(3U << (PIN_SE_SCL * 2)); /* External pull-ups */

    /* PB1  = USART1_TX (AF7) - Fingerprint */
    GPIOB->MODER &= ~(3U << (PIN_FP_TX * 2));
    GPIOB->MODER |= (GPIO_MODE_AF << (PIN_FP_TX * 2));
    GPIOB->AFR[0] &= ~(0xFU << (PIN_FP_TX * 4));
    GPIOB->AFR[0] |= (GPIO_AF7_USART1 << (PIN_FP_TX * 4));

    /* PB2  = USART1_RX (AF7) - Fingerprint */
    GPIOB->MODER &= ~(3U << (PIN_FP_RX * 2));
    GPIOB->MODER |= (GPIO_MODE_AF << (PIN_FP_RX * 2));
    GPIOB->AFR[0] &= ~(0xFU << (PIN_FP_RX * 4));
    GPIOB->AFR[0] |= (GPIO_AF7_USART1 << (PIN_FP_RX * 4));

    /* PB10 = I2C2_SCL (AF4) - PMIC */
    GPIOB->MODER &= ~(3U << (PIN_PMIC_SCL * 2));
    GPIOB->MODER |= (GPIO_MODE_AF << (PIN_PMIC_SCL * 2));
    GPIOB->AFR[1] &= ~(0xFU << ((PIN_PMIC_SCL - 8) * 4));
    GPIOB->AFR[1] |= (GPIO_AF4_I2C2 << ((PIN_PMIC_SCL - 8) * 4));
    GPIOB->OTYPER |= (1U << PIN_PMIC_SCL);
    GPIOB->OSPEEDR |= (GPIO_SPEED_HIGH << (PIN_PMIC_SCL * 2));

    /* PB11 = I2C2_SDA (AF4) - PMIC */
    GPIOB->MODER &= ~(3U << (PIN_PMIC_SDA * 2));
    GPIOB->MODER |= (GPIO_MODE_AF << (PIN_PMIC_SDA * 2));
    GPIOB->AFR[1] &= ~(0xFU << ((PIN_PMIC_SDA - 8) * 4));
    GPIOB->AFR[1] |= (GPIO_AF4_I2C2 << ((PIN_PMIC_SDA - 8) * 4));
    GPIOB->OTYPER |= (1U << PIN_PMIC_SDA);

    /* PB12-PB15 = SPI2 (QSPI Flash) - will be used in SPI mode initially */
    GPIOB->MODER &= ~(3U << (PIN_FLASH_NSS * 2));
    GPIOB->MODER |= (GPIO_MODE_OUTPUT << (PIN_FLASH_NSS * 2));
    GPIOB->ODR |= (1U << PIN_FLASH_NSS); /* CS high (deselected) */

    GPIOB->MODER &= ~(3U << (PIN_FLASH_SCK * 2));
    GPIOB->MODER |= (GPIO_MODE_AF << (PIN_FLASH_SCK * 2));
    /* AF5 for SPI2 on PB13 */

    GPIOB->MODER &= ~(3U << (PIN_FLASH_MISO * 2));
    GPIOB->MODER |= (GPIO_MODE_AF << (PIN_FLASH_MISO * 2));

    GPIOB->MODER &= ~(3U << (PIN_FLASH_MOSI * 2));
    GPIOB->MODER |= (GPIO_MODE_AF << (PIN_FLASH_MOSI * 2));

    /* --- GPIOC Configuration --- */
    /* PC0  = nBLE_RST (output, push-pull) */
    GPIOC->MODER &= ~(3U << (PIN_nBLE_RST * 2));
    GPIOC->MODER |= (GPIO_MODE_OUTPUT << (PIN_nBLE_RST * 2));
    GPIOC->ODR |= (1U << PIN_nBLE_RST); /* Reset inactive (high) */

    /* PC1  = BLE_MODE (output, push-pull) */
    GPIOC->MODER &= ~(3U << (PIN_BLE_MODE * 2));
    GPIOC->MODER |= (GPIO_MODE_OUTPUT << (PIN_BLE_MODE * 2));

    /* PC2  = NFC_IRQ (input, pull-up) */
    GPIOC->MODER &= ~(3U << (PIN_NFC_IRQ * 2));
    /* Input mode (default) */
    GPIOC->PUPDR &= ~(3U << (PIN_NFC_IRQ * 2));
    GPIOC->PUPDR |= (GPIO_PULL_UP << (PIN_NFC_IRQ * 2));

    /* PC3  = nNFC_RST (output, push-pull) */
    GPIOC->MODER &= ~(3U << (PIN_nNFC_RST * 2));
    GPIOC->MODER |= (GPIO_MODE_OUTPUT << (PIN_nNFC_RST * 2));
    GPIOC->ODR |= (1U << PIN_nNFC_RST);

    /* PC4  = SE_IRQ (input, pull-up) */
    GPIOC->MODER &= ~(3U << (PIN_SE_IRQ * 2));
    GPIOC->PUPDR &= ~(3U << (PIN_SE_IRQ * 2));
    GPIOC->PUPDR |= (GPIO_PULL_UP << (PIN_SE_IRQ * 2));

    /* PC5  = I2C1_SDA (AF4) - SE */
    GPIOC->MODER &= ~(3U << (PIN_SE_SDA * 2));
    GPIOC->MODER |= (GPIO_MODE_AF << (PIN_SE_SDA * 2));
    GPIOC->AFR[0] &= ~(0xFU << (PIN_SE_SDA * 4));
    GPIOC->AFR[0] |= (GPIO_AF4_I2C1 << (PIN_SE_SDA * 4));
    GPIOC->OTYPER |= (1U << PIN_SE_SDA);

    /* PC6-7 = QSPI Flash data lines (output initially, switch to AF for quad) */
    GPIOC->MODER &= ~(3U << (PIN_FLASH_D2 * 2));
    GPIOC->MODER |= (GPIO_MODE_AF << (PIN_FLASH_D2 * 2));
    GPIOC->MODER &= ~(3U << (PIN_FLASH_D3 * 2));
    GPIOC->MODER |= (GPIO_MODE_AF << (PIN_FLASH_D3 * 2));

    /* PC13 = nFP_RST (output, push-pull) */
    GPIOC->MODER &= ~(3U << (PIN_nFP_RST * 2));
    GPIOC->MODER |= (GPIO_MODE_OUTPUT << (PIN_nFP_RST * 2));
    GPIOC->ODR |= (1U << PIN_nFP_RST);

    /* PC15 = LED_GREEN (output, push-pull) */
    GPIOC->MODER &= ~(3U << (PIN_LED_GREEN * 2));
    GPIOC->MODER |= (GPIO_MODE_OUTPUT << (PIN_LED_GREEN * 2));
}

/* ============================================================
 * USART Initialization
 * ============================================================ */
static void usart1_init(void)
{
    /* Enable USART1 clock */
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;

    /* Configure USART1 for 115200 baud (FP sensor)
     * BRR = APB2_CLOCK / BAUD = 110000000 / 115200 ≈ 955
     */
    USART1->BRR = 955;
    USART1->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
}

static void usart2_init(void)
{
    /* Enable USART2 clock */
    RCC->APB1ENR1 |= RCC_APB1ENR1_USART2EN;

    /* Configure USART2 for 921600 baud (BLE)
     * BRR = APB1_CLOCK / BAUD = 110000000 / 921600 ≈ 119
     */
    USART2->BRR = 119;
    USART2->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
}

/* ============================================================
 * SPI Initialization
 * ============================================================ */
static void spi1_init(void)
{
    /* Enable SPI1 clock */
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

    /* Configure SPI1 for PN7150 (Mode 0, 10 MHz, Master) */
    SPI1->CR1 = SPI_CR1_MSTR
               | SPI_CR1_SSM | SPI_CR1_SSI  /* Software NSS management */
               | (2U << 3);  /* BR = fPCLK/8 ≈ 13.75 MHz, close enough */
    SPI1->CR2 = SPI_CR2_DS_8BIT | SPI_CR2_SSOE;

    /* Enable SPI1 */
    SPI1->CR1 |= SPI_CR1_SPE;
}

static void spi2_init(void)
{
    /* Enable SPI2 clock */
    RCC->APB1ENR1 |= RCC_APB1ENR1_SPI2EN;

    /* Configure SPI2 for MX25R1635F (Mode 0, up to 80 MHz) */
    SPI2->CR1 = SPI_CR1_MSTR
               | SPI_CR1_SSM | SPI_CR1_SSI
               | (1U << 3);  /* BR = fPCLK/4 ≈ 27.5 MHz for initial */
    SPI2->CR2 = SPI_CR2_DS_8BIT | SPI_CR2_SSOE;

    /* Enable SPI2 */
    SPI2->CR1 |= SPI_CR1_SPE;
}

/* ============================================================
 * I2C Initialization
 * ============================================================ */
static void i2c1_init(void)
{
    /* Enable I2C1 clock */
    RCC->APB1ENR1 |= RCC_APB1ENR1_I2C1EN;

    /* Configure I2C1 timing for 400 kHz Fast Mode at 110 MHz */
    I2C1->TIMINGR = I2C_TIMINGR_400K;
    I2C1->CR1 = I2C_CR1_PE;
}

static void i2c2_init(void)
{
    /* Enable I2C2 clock */
    RCC->APB1ENR1 |= RCC_APB1ENR1_I2C2EN;

    /* Configure I2C2 timing for 400 kHz */
    I2C2->TIMINGR = I2C_TIMINGR_400K;
    I2C2->CR1 = I2C_CR1_PE;
}

/* ============================================================
 * NVIC Configuration
 * ============================================================ */
static void nvic_init(void)
{
    /* Enable interrupts for peripherals */
    NVIC->ICPR[0] = 0xFFFFFFFF; /* Clear all pending */
    
    /* Enable USART1 (FP) interrupt */
    NVIC->ISER[USART1_IRQn >> 5] = (1U << (USART1_IRQn & 0x1F));
    
    /* Enable USART2 (BLE) interrupt */
    NVIC->ISER[USART2_IRQn >> 5] = (1U << (USART2_IRQn & 0x1F));
    
    /* Enable SPI1 (NFC) interrupt */
    NVIC->ISER[SPI1_IRQn >> 5] = (1U << (SPI1_IRQn & 0x1F));
    
    /* Enable I2C1 (SE) interrupt */
    NVIC->ISER[I2C1_IRQn >> 5] = (1U << (I2C1_IRQn & 0x1F));
    
    /* Enable EXTI interrupts */
    NVIC->ISER[EXTI5_9_IRQn >> 5] = (1U << (EXTI5_9_IRQn & 0x1F));
}

/* ============================================================
 * SysTick Configuration (1ms interval)
 * ============================================================ */
static void systick_init(void)
{
    SysTick->LOAD = (SYS_CLOCK / 1000) - 1; /* 1ms at 110 MHz */
    SysTick->VAL = 0;
    SysTick->CTRL = SysTick_CTRL_ENABLE | SysTick_CTRL_TICKINT | SysTick_CTRL_CLKSOURCE;
}

/* ============================================================
 * System Initialization Sequence
 * ============================================================ */
static void system_init(void)
{
    /* Step 1: Configure clocks */
    clock_init();

    /* Step 2: Configure SysTick */
    systick_init();

    /* Step 3: Initialize GPIO */
    gpio_init();

    /* Step 4: Initialize NVIC */
    nvic_init();

    /* Step 5: Initialize PMIC (STPMIC1) - enable power rails */
    pmic_init();

    /* Small delay for power rail stabilization */
    delay_ms(10);

    /* Step 6: Initialize I2C buses */
    i2c1_init();  /* Secure Element */
    i2c2_init();  /* PMIC */

    /* Step 7: Initialize SPI buses */
    spi1_init();  /* NFC Controller */
    spi2_init();  /* QSPI Flash */

    /* Step 8: Initialize UARTs */
    usart1_init(); /* Fingerprint Sensor */
    usart2_init(); /* BLE Coprocessor */

    /* Step 9: Initialize Secure Element (A71CH) */
    se_init();

    /* Step 10: Initialize QSPI Flash */
    spi_flash_init();

    /* Step 11: Initialize NFC Controller (PN7150) */
    nfc_init();

    /* Step 12: Initialize Fingerprint Sensor (FPC1025) */
    fp_init();

    /* Step 13: Initialize BLE Coprocessor (nRF52833) */
    /* BLE init is via UART, handled in main loop */

    /* Step 14: Set initial LED state */
    LED_GREEN_ON();
    delay_ms(100);
    LED_GREEN_OFF();
}

/* ============================================================
 * LED Status Indication
 * ============================================================ */
static void led_set_color(led_color_t color)
{
    /* All LEDs are active-low (cathode to GND via resistor) */
    if (color & 0x01) LED_RED_ON();   else LED_RED_OFF();
    if (color & 0x02) LED_GREEN_ON(); else LED_GREEN_OFF();
    if (color & 0x04) LED_BLUE_ON();  else LED_BLUE_OFF();
}

/* ============================================================
 * Power Management - Enter STOP2 Mode
 * ============================================================ */
static void enter_stop2_mode(void)
{
    /* Disable LEDs */
    led_set_color(LED_COLOR_OFF);

    /* Configure EXTI for wake-up sources:
     * - NFC field detect (PC2 / EXTI2)
     * - Touch button (external interrupt)
     * - USB VBUS detect (PA9)
     */

    /* Clear SLEEPDEEP bit, set SLEEPONEXIT */
    SCB->SCR |= (1U << 2); /* SLEEPDEEP */

    /* Set PWR low-power mode to STOP2 */
    PWR->CR1 |= (2U << 0); /* LPMS = 10 = STOP2 */

    /* Enter STOP mode */
    __asm__("wfi");

    /* Woken up - reconfigure clocks */
    clock_init();
    systick_init();
}

/* ============================================================
 * Main Loop
 * ============================================================ */
int main(void)
{
    /* Initialize all hardware */
    system_init();

    /* Indicate successful boot */
    led_set_color(LED_COLOR_GREEN);
    delay_ms(500);
    led_set_color(LED_COLOR_OFF);

    /* Main event loop */
    while (1) {
        /* Check for USB connection */
        if (usb_connected) {
            system_state = STATE_USB_AUTH;
            led_set_color(LED_COLOR_BLUE);
        }

        /* Check for NFC field */
        if (nfc_field_active) {
            nfc_field_active = 0;
            system_state = STATE_NFC_AUTH;
            led_set_color(LED_COLOR_PURPLE);
        }

        /* Main state machine */
        switch (system_state) {
        case STATE_IDLE:
            /* Enter low-power mode after 5 seconds of inactivity */
            led_set_color(LED_COLOR_OFF);
            enter_stop2_mode();
            system_state = STATE_IDLE;
            break;

        case STATE_USB_AUTH:
            /* Handle FIDO2 authentication via USB */
            /* USB CTAP2 handler would be called here */
            /* ... see drivers/usb_ctap2.c ... */
            led_set_color(LED_COLOR_BLUE);
            break;

        case STATE_NFC_AUTH:
            /* Handle FIDO2 authentication via NFC */
            /* NFC APDU handler would be called here */
            led_set_color(LED_COLOR_PURPLE);
            break;

        case STATE_BLE_AUTH:
            /* Handle BLE proximity authentication */
            led_set_color(LED_COLOR_CYAN);
            break;

        case STATE_ENROLLING:
            /* Fingerprint enrollment mode */
            led_set_color(LED_COLOR_YELLOW);
            break;

        case STATE_UPDATING:
            /* Firmware update mode */
            led_set_color(LED_COLOR_RED);
            /* Flash LED during update */
            for (int i = 0; i < 10; i++) {
                LED_RED_TOGGLE();
                delay_ms(200);
            }
            break;

        case STATE_ERROR:
            /* Error indication - fast blink red */
            LED_RED_TOGGLE();
            delay_ms(100);
            break;

        default:
            system_state = STATE_IDLE;
            break;
        }

        /* Small delay to prevent busy-looping */
        delay_ms(10);
    }

    /* Should never reach here */
    return 0;
}