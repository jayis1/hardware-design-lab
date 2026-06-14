/*
 * NeuroLink — Main Entry Point
 * STM32H743ZIT6 SPL (Secondary Program Loader) and application init
 */

#include "board.h"
#include "registers.h"
#include "drivers/ads1299.h"
#include "drivers/bq25895.h"
#include "drivers/lsm6dsox.h"
#include "drivers/ice40_spi.h"

/* ============================================================
 * Static buffers in AXI SRAM (DMA-accessible)
 * ============================================================ */
#define AXI_SRAM_BUF1  ((volatile uint8_t *)0x24000000)
#define AXI_SRAM_BUF2  ((volatile uint8_t *)0x24000400)
#define AXI_SRAM_CMD   ((volatile uint8_t *)0x24000800)

static volatile uint32_t sample_count = 0;
static volatile uint8_t  ads_data_ready = 0;
static volatile uint8_t  fpga_irq_flag = 0;
static volatile uint8_t  ble_data_ready = 0;

/* ============================================================
 * Clock Initialization
 * ============================================================ */
static void clock_init(void)
{
    /* Enable power interface clock */
    RCC_APB1ENR |= (1 << 28);  // PWREN

    /* Configure voltage scaling: VOS1 (high performance) */
    *((volatile uint32_t *)0x50001000) = 3 << 14;  // PWR_D3CR VOS=11

    /* Wait for VOS ready */
    while (!(*((volatile uint32_t *)0x50001000) & (1 << 13)));

    /* Enable HSE (27 MHz) */
    RCC_CR |= (1 << 16);  // HSEON
    while (!(RCC_CR & (1 << 17)));  // Wait HSERDY

    /* Configure PLL1: 27 MHz → 480 MHz
     * PLL1M = 27, PLL1N = 480, PLL1P = 1, PLL1Q = 20, PLL1R = 2
     */
    RCC_PLLCKSELR = (27 << 0) |    // PLL1M = 27
                     (1 << 4) |      // PLL1SRC = HSE
                     (27 << 8) |     // PLL2M = 27
                     (1 << 12);      // PLL2SRC = HSE

    RCC_PLLCFGR = (1 << 24) |      // PLL1VCOSEL = 128-560MHz
                   (1 << 26);       // PLL2VCOSEL = 128-560MHz

    /* PLL1 dividers: N=480, P=1, Q=20, R=2 */
    RCC_PLL1DIVR = (479 << 0) |     // PLL1N = 480 (reg = N-1)
                    (0 << 9) |       // PLL1P = 1 (reg = P-1)
                    (19 << 16) |     // PLL1Q = 20 (reg = Q-1)
                    (1 << 24);       // PLL1R = 2 (reg = R-1)

    /* PLL2 dividers: N=400, P=8, Q=2 */
    RCC_PLL2DIVR = (399 << 0) |     // PLL2N = 400
                    (7 << 9) |       // PLL2P = 8
                    (1 << 16);       // PLL2Q = 2

    /* Enable PLL1 and PLL2 */
    RCC_CR |= (1 << 24) | (1 << 26);  // PLL1ON, PLL2ON
    while (!(RCC_CR & (1 << 25)));     // Wait PLL1RDY
    while (!(RCC_CR & (1 << 27)));     // Wait PLL2RDY

    /* Configure bus prescalers before switching clock */
    /* D1 domain: HPRE = /2 (240 MHz) */
    RCC_D1CFGR = (1 << 0);   // HPRE = /2

    /* D1/D2 APB: D1PPRE = /2, D2PPRE1 = /2, D2PPRE2 = /2 */
    RCC_D1CFGR |= (3 << 4);  // D1PPRE = /2
    RCC_D2CFGR = (3 << 0) | (3 << 4) | (3 << 8);
    RCC_D3CFGR = (3 << 0);   // D3PPRE = /2

    /* Switch SYSCLK to PLL1 */
    RCC_CFGR = (3 << 0);     // SW = PLL1
    while (((RCC_CFGR >> 3) & 7) != 3);  // Wait for SWS = PLL1

    /* Enable peripheral clocks */
    RCC_AHB1ENR |= (1 << 0);   // GPIOA
    RCC_AHB1ENR |= (1 << 1);   // GPIOB
    RCC_AHB1ENR |= (1 << 2);   // GPIOC
    RCC_AHB1ENR |= (1 << 3);   // GPIOD
    RCC_AHB1ENR |= (1 << 4);   // GPIOE
    RCC_AHB1ENR |= (1 << 5);   // GPIOF
    RCC_AHB1ENR |= (1 << 6);   // GPIOG
    RCC_AHB1ENR |= (1 << 7);   // GPIOH

    RCC_APB2ENR |= (1 << 12);  // SPI1
    RCC_APB1ENR |= (1 << 14);  // SPI2
    RCC_APB2ENR |= (1 << 4);   // USART1
    RCC_AHB1ENR |= (1 << 16);  // DMA1
    RCC_AHB1ENR |= (1 << 17);  // DMA2
    RCC_AHB1ENR |= (1 << 21);  // MDMA
    RCC_APB1ENR |= (1 << 21);  // I2C1
    RCC_APB1ENR |= (1 << 22);  // I2C2
}

/* ============================================================
 * GPIO Initialization
 * ============================================================ */
static void gpio_init(void)
{
    /* --- SPI1 Pins: ADS1299 (PA4-PA8) --- */
    /* PA4 = ADS_CS_N (GPIO output, push-pull, high speed) */
    GPIO_MODER(GPIOA) = (GPIO_MODER(GPIOA) & ~(3 << (4*2))) | (1 << (4*2));
    GPIO_OTYPER(GPIOA) &= ~(1 << 4);
    GPIO_OSPEEDR(GPIOA) |= (3 << (4*2));  // Very high speed
    GPIO_PUPDR(GPIOA) &= ~(3 << (4*2));    // No pull
    GPIO_BSRR(GPIOA) |= (1 << 4);          // CS high (inactive)

    /* PA5 = ADS_SCLK (AF5 = SPI1_SCK) */
    GPIO_MODER(GPIOA) = (GPIO_MODER(GPIOA) & ~(3 << (5*2))) | (2 << (5*2));
    GPIO_OSPEEDR(GPIOA) |= (3 << (5*2));
    GPIO_AFRL(GPIOA) = (GPIO_AFRL(GPIOA) & ~(0xF << (5*4))) | (AF_SPI1 << (5*4));

    /* PA6 = ADS_MISO (AF5 = SPI1_MISO) */
    GPIO_MODER(GPIOA) = (GPIO_MODER(GPIOA) & ~(3 << (6*2))) | (2 << (6*2));
    GPIO_OSPEEDR(GPIOA) |= (3 << (6*2));
    GPIO_AFRL(GPIOA) = (GPIO_AFRL(GPIOA) & ~(0xF << (6*4))) | (AF_SPI1 << (6*4));

    /* PA7 = ADS_MOSI (AF5 = SPI1_MOSI) */
    GPIO_MODER(GPIOA) = (GPIO_MODER(GPIOA) & ~(3 << (7*2))) | (2 << (7*2));
    GPIO_OSPEEDR(GPIOA) |= (3 << (7*2));
    GPIO_AFRL(GPIOA) = (GPIO_AFRL(GPIOA) & ~(0xF << (7*4))) | (AF_SPI1 << (7*4));

    /* PA8 = ADS_START (GPIO output, push-pull) */
    GPIO_MODER(GPIOA) = (GPIO_MODER(GPIOA) & ~(3 << (8*2))) | (1 << (8*2));
    GPIO_OTYPER(GPIOA) &= ~(1 << 8);
    GPIO_OSPEEDR(GPIOA) |= (2 << (8*2));  // High speed

    /* --- PD7 = ADS_DRDY (GPIO input, pull-up, EXTI) --- */
    GPIO_MODER(GPIOD) &= ~(3 << (7*2));    // Input
    GPIO_PUPDR(GPIOD) |= (1 << (7*2));      // Pull-up
    /* EXTI7 configured in interrupt handler */

    /* PE15 = ADS_RESET_N (GPIO output, push-pull, initially high) */
    GPIO_MODER(GPIOE) = (GPIO_MODER(GPIOE) & ~(3 << (15*2))) | (1 << (15*2));
    GPIO_OTYPER(GPIOE) &= ~(1 << 15);
    GPIO_BSRR(GPIOE) |= (1 << 15);  // Reset inactive (high)

    /* --- SPI2 Pins: iCE40 FPGA (PB10, PB12, PB14, PB15) --- */
    /* PB12 = FPGA_CS_N (GPIO output) */
    GPIO_MODER(GPIOB) = (GPIO_MODER(GPIOB) & ~(3 << (12*2))) | (1 << (12*2));
    GPIO_OTYPER(GPIOB) &= ~(1 << 12);
    GPIO_OSPEEDR(GPIOB) |= (3 << (12*2));
    GPIO_BSRR(GPIOB) |= (1 << 12);  // CS high (inactive)

    /* PB10 = FPGA_SCK (AF5 = SPI2_SCK) */
    GPIO_MODER(GPIOB) = (GPIO_MODER(GPIOB) & ~(3 << (10*2))) | (2 << (10*2));
    GPIO_OSPEEDR(GPIOB) |= (3 << (10*2));
    GPIO_AFRL(GPIOB) = (GPIO_AFRL(GPIOB) & ~(0xF << (10*4))) | (AF_SPI2 << (10*4));

    /* PB14 = FPGA_MISO (AF5 = SPI2_MISO) */
    GPIO_MODER(GPIOB) = (GPIO_MODER(GPIOB) & ~(3 << (14*2))) | (2 << (14*2));
    GPIO_OSPEEDR(GPIOB) |= (3 << (14*2));
    GPIO_AFRL(GPIOB) = (GPIO_AFRL(GPIOB) & ~(0xF << (14*4))) | (AF_SPI2 << (14*4));

    /* PB15 = FPGA_MOSI (AF5 = SPI2_MOSI) */
    GPIO_MODER(GPIOB) = (GPIO_MODER(GPIOB) & ~(3 << (15*2))) | (2 << (15*2));
    GPIO_OSPEEDR(GPIOB) |= (3 << (15*2));
    GPIO_AFRL(GPIOB) = (GPIO_AFRL(GPIOB) & ~(0xF << (15*4))) | (AF_SPI2 << (15*4));

    /* PB0 = FPGA_RESET_N (GPIO output, initially low = in reset) */
    GPIO_MODER(GPIOB) = (GPIO_MODER(GPIOB) & ~(3 << (0*2))) | (1 << (0*2));
    GPIO_OTYPER(GPIOB) &= ~(1 << 0);
    GPIO_BSRR(GPIOB) |= (1 << (0 + 16));  // Reset low (active)

    /* PB1 = FPGA_CDONE (GPIO input, pull-up) */
    GPIO_MODER(GPIOB) &= ~(3 << (1*2));
    GPIO_PUPDR(GPIOB) |= (1 << (1*2));

    /* PE8 = FPGA_IRQ (GPIO input, pull-up, EXTI) */
    GPIO_MODER(GPIOE) &= ~(3 << (8*2));
    GPIO_PUPDR(GPIOE) |= (1 << (8*2));

    /* --- I2C1: IMU (PC0, PC1) --- */
    GPIO_MODER(GPIOC) = (GPIO_MODER(GPIOC) & ~(3 << (0*2))) | (2 << (0*2));
    GPIO_MODER(GPIOC) = (GPIO_MODER(GPIOC) & ~(3 << (1*2))) | (2 << (1*2));
    GPIO_OTYPER(GPIOC) |= (1 << 0) | (1 << 1);  // Open drain
    GPIO_OSPEEDR(GPIOC) |= (2 << (0*2)) | (2 << (1*2));
    GPIO_AFRL(GPIOC) = (GPIO_AFRL(GPIOC) & ~(0xFF)) | (AF_I2C1 << (0*4)) | (AF_I2C1 << (1*4));
    GPIO_PUPDR(GPIOC) &= ~(3 << (0*2));
    GPIO_PUPDR(GPIOC) &= ~(3 << (1*2));

    /* --- I2C2: PMIC + Temp (PC4, PC5) --- */
    GPIO_MODER(GPIOC) = (GPIO_MODER(GPIOC) & ~(3 << (4*2))) | (2 << (4*2));
    GPIO_MODER(GPIOC) = (GPIO_MODER(GPIOC) & ~(3 << (5*2))) | (2 << (5*2));
    GPIO_OTYPER(GPIOC) |= (1 << 4) | (1 << 5);  // Open drain
    GPIO_OSPEEDR(GPIOC) |= (2 << (4*2)) | (2 << (5*2));
    GPIO_AFRL(GPIOC) = (GPIO_AFRL(GPIOC) & ~(0xFF << (4*4))) |
                       (AF_I2C2 << (4*4)) | (AF_I2C2 << (5*4));

    /* --- USART1: BLE (PA9=TX, PA10=RX) --- */
    GPIO_MODER(GPIOA) = (GPIO_MODER(GPIOA) & ~(3 << (9*2))) | (2 << (9*2));
    GPIO_MODER(GPIOA) = (GPIO_MODER(GPIOA) & ~(3 << (10*2))) | (2 << (10*2));
    GPIO_OSPEEDR(GPIOA) |= (2 << (9*2)) | (2 << (10*2));
    GPIO_AFRL(GPIOA) = (GPIO_AFRL(GPIOA) & ~(0xF << (9*4))) | (AF_USART1 << (9*4));
    GPIO_AFRH(GPIOA) = (GPIO_AFRH(GPIOA) & ~(0xF << ((10-8)*4))) | (AF_USART1 << ((10-8)*4));

    /* --- LED pins (PE5=Red, PE6=Green, PE7=Blue) --- */
    for (int pin = 5; pin <= 7; pin++) {
        GPIO_MODER(GPIOE) = (GPIO_MODER(GPIOE) & ~(3 << (pin*2))) | (1 << (pin*2));
        GPIO_OTYPER(GPIOE) &= ~(1 << pin);
        GPIO_OSPEEDR(GPIOE) |= (1 << (pin*2));
    }
    /* All LEDs off initially (active low with resistors) */
    GPIO_BSRR(GPIOE) |= (1 << 5) | (1 << 6) | (1 << 7);

    /* --- PH4 = USB VBUS enable (GPIO output, initially low) --- */
    GPIO_MODER(GPIOH) = (GPIO_MODER(GPIOH) & ~(3 << (4*2))) | (1 << (4*2));
    GPIO_OTYPER(GPIOH) &= ~(1 << 4);
}

/* ============================================================
 * SPI1 Initialization (ADS1299 bus)
 * ============================================================ */
static void spi1_init(void)
{
    /* SPI1: Master, CPOL=0, CPHA=1 (Mode 1), 16-bit data, MSB first */
    SPI1_CR1 = 0;
    SPI1_CR1 |= (0 << 0) |    // CPHA = 1 (second edge)
                 (0 << 1) |    // CPOL = 0 (clock idle low)
                 (1 << 2) |    // MSTR = 1 (master)
                 (3 << 3) |    // BR[2:0] = /32 (7.5 MHz at 240 MHz APB2)
                 (0 << 7) |    // LSBFIRST = 0 (MSB first)
                 (1 << 8) |    // SSM = 1 (software slave management)
                 (1 << 9) |    // SSI = 1 (internal slave select)
                 (0 << 10) |   // RXONLY = 0 (full duplex)
                 (0 << 11);    // DFF = 0 (8-bit frame, we'll use FIFO)

    /* SPI1 CR2: DMA enabled, FIFO threshold */
    SPI1_CR2 = (1 << 0) |     // RXDMAEN = 1
               (1 << 1) |      // TXDMAEN = 1
               (1 << 12) |     // FRXTH = 1 (RXNE at 8 bits)
               (0xF << 4);     // DS = 16-bit data size (we'll toggle)

    SPI1_CR1 |= (1 << 6);     // SPE = 1 (enable SPI)
}

/* ============================================================
 * SPI2 Initialization (iCE40 FPGA)
 * ============================================================ */
static void spi2_init(void)
{
    SPI2_CR1 = 0;
    SPI2_CR1 |= (0 << 0) |    // CPHA = 0 (first edge)
                 (0 << 1) |    // CPOL = 0 (clock idle low)
                 (1 << 2) |    // MSTR = 1
                 (3 << 3) |    // BR = /32
                 (1 << 8) |    // SSM = 1
                 (1 << 9);     // SSI = 1

    SPI2_CR2 = (1 << 0) |     // RXDMAEN
               (1 << 1) |     // TXDMAEN
               (1 << 12);     // FRXTH

    SPI2_CR1 |= (1 << 6);     // SPE
}

/* ============================================================
 * USART1 Initialization (BLE)
 * ============================================================ */
static void usart1_init(void)
{
    /* USART1: 3 Mbps, 8N1, hardware flow control */
    USART1_CR1 = 0;
    USART1_CR1 |= (1 << 0) |   // UE = enable
                   (1 << 2) |   // RE = receiver enable
                   (1 << 3) |   // TE = transmitter enable
                   (1 << 5) |   // RXNEIE = RX interrupt
                   (0 << 12) |  // M0 = 8 bits
                   (0 << 28);   // M1 = 0 (8-bit word)

    USART1_CR2 = 0;  // 1 stop bit
    USART1_CR3 = (1 << 8) | (1 << 9);  // CTSE, RTSE (flow control)

    /* BRR = APB2_CLK / BAUD = 120000000 / 3000000 = 40 */
    USART1_BRR = 40;

    USART1_CR1 |= (1 << 0);  // Enable
}

/* ============================================================
 * LED Control
 * ============================================================ */
static void led_set(int led, int r, int g, int b)
{
    /* LEDs are active-low through NPN drivers:
       GPIO high = LED off, GPIO low = LED on */
    uint32_t port = GPIOE;
    if (r) GPIO_BSRR(port) |= (1 << (5 + 16));  // Set = on (active low)
    else   GPIO_BSRR(port) |= (1 << 5);          // Reset = off
    if (g) GPIO_BSRR(port) |= (1 << (6 + 16));
    else   GPIO_BSRR(port) |= (1 << 6);
    if (b) GPIO_BSRR(port) |= (1 << (7 + 16));
    else   GPIO_BSRR(port) |= (1 << 7);
}

static void led_all_off(void)
{
    GPIO_BSRR(GPIOE) |= (1 << 5) | (1 << 6) | (1 << 7);
}

/* ============================================================
 * ADS1299 Data Ready Interrupt Handler (EXTI7 = PD7)
 * ============================================================ */
void EXTI7_IRQHandler(void)
{
    /* Clear pending interrupt */
    EXTI_PR1 |= (1 << 7);

    /* Read data from ADS1299 daisy chain via SPI DMA */
    ads_data_ready = 1;
    sample_count++;
}

/* ============================================================
 * FPGA Interrupt Handler (EXTI8 = PE8)
 * ============================================================ */
void EXTI8_IRQHandler(void)
{
    EXTI_PR1 |= (1 << 8);
    fpga_irq_flag = 1;
}

/* ============================================================
 * Main Program
 * ============================================================ */
int main(void)
{
    /* Disable interrupts during init */
    __asm volatile("cpsid i");

    /* Initialize hardware */
    clock_init();
    gpio_init();
    spi1_init();
    spi2_init();
    usart1_init();

    /* Enable interrupts */
    __asm volatile("cpsie i");

    /* Power-on self-test: blink RGB LED */
    led_set(0, 1, 0, 0);  // Red
    for (volatile int i = 0; i < 1000000; i++);
    led_set(0, 0, 1, 0);  // Green
    for (volatile int i = 0; i < 1000000; i++);
    led_set(0, 0, 0, 1);  // Blue
    for (volatile int i = 0; i < 1000000; i++);
    led_all_off();

    /* Initialize I2C peripherals */
    bq25895_init();
    lsm6dsox_init();

    /* Initialize ADS1299 chain */
    ads1299_init();
    ads1299_config_all();

    /* Load FPGA bitstream */
    ice40_load_bitstream();
    ice40_reset_release();

    /* Wait for FPGA configuration */
    volatile int timeout = 100000;
    while (!ice40_is_config_done() && timeout--) {
        for (volatile int i = 0; i < 100; i++);
    }

    if (!ice40_is_config_done()) {
        /* FPGA config failed — blink red rapidly */
        while (1) {
            led_set(0, 1, 0, 0);
            for (volatile int i = 0; i < 200000; i++);
            led_all_off();
            for (volatile int i = 0; i < 200000; i++);
        }
    }

    /* Start ADS1299 conversions */
    ads1299_start();

    /* Green LED = running */
    led_set(0, 0, 1, 0);

    /* Main loop */
    while (1) {
        if (ads_data_ready) {
            ads_data_ready = 0;

            /* Read 216 bytes from ADS1299 daisy chain via SPI1 DMA */
            ads1299_read_frame((uint8_t *)AXI_SRAM_BUF1);

            /* Send to FPGA DSP pipeline via SPI2 */
            ice40_send_samples((uint8_t *)AXI_SRAM_BUF1, ADS_FRAME_SIZE);

            /* If FPGA has processed data, read it back */
            if (fpga_irq_flag) {
                fpga_irq_flag = 0;
                ice40_read_processed((uint8_t *)AXI_SRAM_BUF2, ADS_FRAME_SIZE);

                /* Forward to USB and/or BLE */
                /* (USB and BLE drivers would handle this) */
            }
        }

        /* Read IMU data periodically */
        if ((sample_count & 0xFF) == 0) {
            /* Every 256 samples, read IMU for motion tagging */
            lsm6dsox_read_accel((int16_t *)AXI_SRAM_CMD);
            lsm6dsox_read_gyro((int16_t *)(AXI_SRAM_CMD + 6));
        }

        /* Monitor battery status */
        if ((sample_count & 0x3FF) == 0) {
            uint8_t chg_status = bq25895_get_charging_status();
            uint8_t fault = bq25895_get_fault_status();
            (void)chg_status;
            (void)fault;
        }
    }

    return 0;
}