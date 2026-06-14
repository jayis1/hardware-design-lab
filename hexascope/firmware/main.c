/*
 * main.c — HexaScope SPL / Board Init Entry Point
 * Initializes clocks, GPIO, PMIC, FPGA, and enters main loop
 */

#include "board.h"
#include "registers.h"
#include <string.h>

/* ============================================================
 * External declarations
 * ============================================================ */
extern void pmic_init(void);
extern void pmic_enable_rails(void);
extern uint8_t pmic_read_reg(uint8_t reg);
extern void pmic_write_reg(uint8_t reg, uint8_t val);

extern void dac60508_init(void);
extern void dac_set_channel(uint8_t ch, uint16_t value);

extern void spi_flash_init(void);
extern void spi_flash_read(uint32_t addr, uint8_t *buf, uint32_t len);

extern void fpga_config_load(void);
extern void fpga_spi_write(uint32_t addr, uint32_t data);
extern uint32_t fpga_spi_read(uint32_t addr);

extern void usb_pd_init(void);
extern uint8_t usb_pd_negotiate(void);

extern void esp32_wifi_init(void);

/* ============================================================
 * System Clock Configuration
 * HSE 25 MHz → PLL → 170 MHz SYSCLK
 * PLLM=5, PLLN=68, PLLP=2, PLLR=2
 * VCO = 25/5 * 68 = 340 MHz
 * PLLP = 340/2 = 170 MHz (SYSCLK)
 * PLLQ = 340/8 = 42.5 MHz (not used for USB)
 * ============================================================ */
static void SystemClock_Config(void)
{
    /* Enable power clock and configure voltage */
    RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;
    PWR->CR1 |= PWR_CR1_VOS_0;  /* Voltage scale 1 for 170 MHz */

    /* Enable HSE */
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY))
        ;

    /* Configure PLL: HSE 25 MHz → VCO 340 MHz → SYSCLK 170 MHz */
    RCC->PLLCFGR = (5U << 0)     /* PLLM = 5 */
                  | (68U << 6)    /* PLLN = 68 */
                  | (0U << 16)    /* PLLP = 2 */
                  | (RCC_CFGR_SW_PLL << 16) /* PLLSRC = HSE */
                  | (2U << 21);   /* PLLQ = 8 / 2 = divide by 4 = 42.5 MHz */
    /* Note: On STM32G4, PLLCFGR encoding differs. Correct values:
     * PLLM[5:0] = 5
     * PLLN[14:6] = 68
     * PLLP[17:16] = 00 (division by 2)
     * PLLQ[21:20] = 00 (not used)
     * PLLR[25:24] = 00 (division by 2 → 170 MHz)
     * PLLSRC = HSE (bit 16 = 1)
     */
    RCC->PLLCFGR = (5U << 0)      /* PLLM = 5 */
                  | (68U << 6)     /* PLLN = 68 */
                  | (0U << 16)     /* PLLP = 2 */
                  | (1U << 16)     /* PLLSRC = HSE */
                  | (0U << 20)     /* PLLQ not used */
                  | (0U << 24);    /* PLLR = 2 */

    /* Enable PLL */
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY))
        ;

    /* Enable PLLR output (main PLL output) */
    RCC->CR |= (1U << 24);  /* PLLRDY is read-only; enable PLLP/R outputs */

    /* Flash latency for 170 MHz */
    FLASH->ACR = FLASH_ACR_LATENCY_5WS | FLASH_ACR_PRFTBE;
    while (!(FLASH->ACR & FLASH_ACR_PRFTBE))
        ;

    /* Switch SYSCLK to PLL */
    RCC->CFGR = RCC_CFGR_SW_PLL | RCC_CFGR_HPRE_DIV1 |
                 RCC_CFGR_PPRE1_DIV1 | RCC_CFGR_PPRE2_DIV1;
    while ((RCC->CFGR & (3U << 2)) != RCC_CFGR_SWS_PLL)
        ;
}

/* ============================================================
 * GPIO Initialization
 * ============================================================ */
static void GPIO_Init(void)
{
    /* Enable GPIO clocks */
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN |
                    RCC_AHBENR_GPIOCEN | RCC_AHBENR_GPIODEN;

    /* ---- Port A Configuration ---- */
    /* PA0:  Input  — BTN_RUN_STOP (with pull-up) */
    /* PA1:  Output — LED_TRIG (push-pull, low speed) */
    /* PA2:  Output — LED_USB (push-pull, low speed) */
    /* PA3:  Output — LED_PWR (push-pull, low speed) */
    /* PA4:  AF5    — SPI1_NSS (SPI Flash CS) */
    /* PA5:  AF5    — SPI1_SCK */
    /* PA6:  AF5    — SPI1_MISO */
    /* PA7:  AF5    — SPI1_MOSI */
    /* PA8:  AF6    — MCO1 (48 MHz output for USB3340) */
    /* PA11: AF10   — USB_DM */
    /* PA12: AF10   — USB_DP */

    GPIOA->MODER = (0x0U << (0*2))   |  /* PA0:  Input */
                   (0x1U << (1*2))   |  /* PA1:  Output (LED_TRIG) */
                   (0x1U << (2*2))   |  /* PA2:  Output (LED_USB) */
                   (0x1U << (3*2))   |  /* PA3:  Output (LED_PWR) */
                   (0x2U << (4*2))   |  /* PA4:  AF (SPI1_NSS) */
                   (0x2U << (5*2))   |  /* PA5:  AF (SPI1_SCK) */
                   (0x2U << (6*2))   |  /* PA6:  AF (SPI1_MISO) */
                   (0x2U << (7*2))   |  /* PA7:  AF (SPI1_MOSI) */
                   (0x2U << (8*2))   |  /* PA8:  AF (MCO1) */
                   (0x0U << (9*2))   |  /* PA9:  Input */
                   (0x0U << (10*2))  |  /* PA10: Input */
                   (0x2U << (11*2))  |  /* PA11: AF (USB_DM) */
                   (0x2U << (12*2))  |  /* PA12: AF (USB_DP) */
                   (0x2U << (13*2))  |  /* PA13: AF (SWDIO) */
                   (0x2U << (14*2))  |  /* PA14: AF (SWCLK) */
                   (0x2U << (15*2));    /* PA15: AF (MCU_GPIO0) */

    GPIOA->OSPEEDR = (0x0U << (0*2))   |  /* PA0:  Low */
                     (0x0U << (1*2))   |  /* PA1:  Low */
                     (0x3U << (4*2))   |  /* PA4:  Very High (SPI) */
                     (0x3U << (5*2))   |  /* PA5:  Very High (SPI) */
                     (0x3U << (6*2))   |  /* PA6:  Very High (SPI) */
                     (0x3U << (7*2))   |  /* PA7:  Very High (SPI) */
                     (0x3U << (8*2))   |  /* PA8:  Very High (MCO) */
                     (0x3U << (11*2))  |  /* PA11: Very High (USB) */
                     (0x3U << (12*2));    /* PA12: Very High (USB) */

    GPIOA->PUPDR = (0x1U << (0*2));  /* PA0: Pull-up (button) */

    GPIOA->AFRL = (5U << (4*4))   |  /* PA4:  AF5 (SPI1) */
                  (5U << (5*4))   |  /* PA5:  AF5 */
                  (5U << (6*4))   |  /* PA6:  AF5 */
                  (5U << (7*4));     /* PA7:  AF5 */

    GPIOA->AFRH = (0U << (8-8)*4)  |  /* PA8:  AF0 (MCO1) */
                  (10U << (11-8)*4) |  /* PA11: AF10 (USB) */
                  (10U << (12-8)*4);   /* PA12: AF10 (USB) */

    /* ---- Port B Configuration ---- */
    /* PB6:  AF4 — I2C1_SCL (PMIC) */
    /* PB7:  AF4 — I2C1_SDA (PMIC) */
    /* PB8:  AF4 — I2C2_SCL (FUSB302) */
    /* PB9:  AF4 — I2C2_SDA (FUSB302) */
    /* PB10: AF7 — USART3_TX (Wi-Fi) */
    /* PB11: AF7 — USART3_RX (Wi-Fi) */
    /* PB13: AF6 — SPI4_SCK (FPGA register SPI) */
    /* PB14: AF6 — SPI4_MISO */
    /* PB15: AF6 — SPI4_MOSI */

    GPIOB->MODER = (0x0U << (0*2))   |  /* PB0:  Input (MCU_GPIO1) */
                   (0x0U << (1*2))   |  /* PB1:  Input (MCU_GPIO2) */
                   (0x1U << (2*2))   |  /* PB2:  Output (DAC_CS_N, managed by SW) */
                   (0x3U << (3*2))   |  /* PB3:  AF (SPI2_SCK) */
                   (0x2U << (4*2))   |  /* PB4:  AF (SPI2_MISO) */
                   (0x2U << (5*2))   |  /* PB5:  AF (SPI2_MOSI) */
                   (0x2U << (6*2))   |  /* PB6:  AF4 (I2C1_SCL) */
                   (0x2U << (7*2))   |  /* PB7:  AF4 (I2C1_SDA) */
                   (0x2U << (8*2))   |  /* PB8:  AF4 (I2C2_SCL) */
                   (0x2U << (9*2))   |  /* PB9:  AF4 (I2C2_SDA) */
                   (0x2U << (10*2))  |  /* PB10: AF7 (USART3_TX) */
                   (0x2U << (11*2))  |  /* PB11: AF7 (USART3_RX) */
                   (0x0U << (12*2))  |  /* PB12: Input (BTN_FORCE) */
                   (0x2U << (13*2))  |  /* PB13: AF6 (SPI4_SCK) */
                   (0x2U << (14*2))  |  /* PB14: AF6 (SPI4_MISO) */
                   (0x2U << (15*2));   /* PB15: AF6 (SPI4_MOSI) */

    GPIOB->OSPEEDR = 0xFFFFFFFF;  /* All high speed */

    GPIOB->AFRL = (6U << (3*4))   |  /* PB3:  AF6 (SPI2) — actually AF5 for SPI2 */
                  (5U << (4*4))   |  /* PB4:  AF5 (SPI2) */
                  (5U << (5*4))   |  /* PB5:  AF5 (SPI2) */
                  (4U << (6*4))   |  /* PB6:  AF4 (I2C1) */
                  (4U << (7*4));     /* PB7:  AF4 (I2C1) */

    GPIOB->AFRH = (4U << (8-8)*4)   |  /* PB8:  AF4 (I2C2) */
                  (4U << (9-8)*4)   |  /* PB9:  AF4 (I2C2) */
                  (7U << (10-8)*4)  |  /* PB10: AF7 (USART3) */
                  (7U << (11-8)*4)  |  /* PB11: AF7 (USART3) */
                  (6U << (13-8)*4)  |  /* PB13: AF6 (SPI4) */
                  (6U << (14-8)*4)  |  /* PB14: AF6 (SPI4) */
                  (6U << (15-8)*4);    /* PB15: AF6 (SPI4) */

    /* ---- Port C Configuration ---- */
    GPIOC->MODER = (0x3U << (0*2))   |  /* PC0:  Analog (ANA_SENSE) */
                   (0x3U << (1*2))   |  /* PC1:  Analog (TEMP_SENSE) */
                   (0x3U << (2*2))   |  /* PC2:  Analog (VBUS_SENSE) */
                   (0x1U << (3*2))   |  /* PC3:  Output (LED_RUN) */
                   (0x0U << (6*2))   |  /* PC6:  Output (FPGA_SPI_CS_N, SW) */
                   (0x0U << (7*2))   |  /* PC7:  Input (MCU_IRQ) */
                   (0x0U << (8*2))   |  /* PC8:  Input (SD_SW) */
                   (0x1U << (9*2));     /* PC9:  Output (SD_CS_N) */

    /* ---- Port D Configuration ---- */
    GPIOD->MODER = (0x1U << (0*2))   |  /* PD0:  Output (JTAG_TCK) */
                   (0x1U << (1*2))   |  /* PD1:  Output (JTAG_TMS) */
                   (0x1U << (2*2))   |  /* PD2:  Output (JTAG_TDI) */
                   (0x0U << (3*2))   |  /* PD3:  Input (JTAG_TDO) */
                   (0x1U << (4*2))   |  /* PD4:  Output (FPGA_PROG_N) */
                   (0x0U << (5*2))   |  /* PD5:  Input (FPGA_INIT_N) */
                   (0x0U << (6*2))   |  /* PD6:  Input (FPGA_DONE) */
                   (0x0U << (7*2))   |  /* PD7:  Input (PD_INT_N) */
                   (0x0U << (8*2));     /* PD8:  Input (PMIC_INT_N) */

    GPIOD->OSPEEDR = 0x000000FF;  /* PD0-PD3: Very high speed (JTAG) */

    GPIOD->PUPDR = (0x1U << (4*2))   |  /* PD4:  Pull-up (FPGA_PROG_N) */
                    (0x1U << (5*2))   |  /* PD5:  Pull-up (FPGA_INIT_N) */
                    (0x1U << (7*2))   |  /* PD7:  Pull-up (PD_INT_N) */
                    (0x1U << (8*2));     /* PD8:  Pull-up (PMIC_INT_N) */

    /* Set initial LED states (all off, active low) */
    GPIOA->ODR |= (1U << 1) | (1U << 2) | (1U << 3);  /* LEDs off */
    GPIOC->ODR |= (1U << 3);                            /* LED_RUN off */

    /* Set initial FPGA control states */
    GPIOD->ODR &= ~(1U << 4);  /* FPGA_PROG_N = low (FPGA in reset) */
}

/* ============================================================
 * SPI Initialization
 * SPI1: Flash (50 MHz, mode 0)
 * SPI2: DAC (20 MHz, mode 1)
 * SPI4: FPGA registers (40 MHz, mode 0)
 * ============================================================ */
static void SPI1_Init(void)
{
    /* Enable SPI1 clock */
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

    /* Configure SPI1: Master, Mode 0, 8-bit, MSB first, no CRC */
    SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI |
                 SPI_CR1_BR_DIV4 |    /* 170/4 = 42.5 MHz ≈ 50 MHz target */
                 SPI_CR1_SPE;

    SPI1->CR2 = SPI_CR2_DS_8BIT | SPI_CR2_SSOE;
}

static void SPI2_Init(void)
{
    /* Enable SPI2 clock */
    RCC->APB1ENR1 |= RCC_APB1ENR1_SPI2EN;

    /* Configure SPI2: Master, Mode 1 (CPOL=0, CPHA=1), 8-bit, MSB first */
    SPI2->CR1 = SPI_CR1_MSTR | SPI_CR1_CPHA | SPI_CR1_SSM | SPI_CR1_SSI |
                 SPI_CR1_BR_DIV8 |    /* 170/8 = 21.25 MHz ≈ 20 MHz target */
                 SPI_CR1_SPE;

    SPI2->CR2 = SPI_CR2_DS_8BIT;
}

static void SPI4_Init(void)
{
    /* Enable SPI4 clock */
    RCC->APB2ENR |= RCC_APB2ENR_SPI4EN;

    /* Configure SPI4: Master, Mode 0, 8-bit, MSB first, high speed */
    SPI4->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI |
                 SPI_CR1_BR_DIV4 |    /* 170/4 = 42.5 MHz */
                 SPI_CR1_SPE;

    SPI4->CR2 = SPI_CR2_DS_8BIT;
}

/* ============================================================
 * I2C Initialization
 * I2C1: PMIC (400 kHz)
 * I2C2: FUSB302 (400 kHz)
 * ============================================================ */
static void I2C1_Init(void)
{
    RCC->APB1ENR1 |= RCC_APB1ENR1_I2C1EN;

    /* Timing for 400 kHz at 170 MHz:
     * PRESC = 3, SCLDEL = 3, SDADEL = 1, SCLH = 13, SCLL = 19
     * This gives ~400 kHz I2C clock
     */
    I2C1->TIMINGR = (3U << 28) | (3U << 20) | (1U << 16) |
                     (13U << 8) | (19U << 0);
    I2C1->CR1 = I2C_CR1_PE;
}

static void I2C2_Init(void)
{
    RCC->APB1ENR1 |= RCC_APB1ENR1_I2C2EN;

    I2C2->TIMINGR = (3U << 28) | (3U << 20) | (1U << 16) |
                     (13U << 8) | (19U << 0);
    I2C2->CR1 = I2C_CR1_PE;
}

/* ============================================================
 * USART3 Initialization (Wi-Fi UART)
 * 115200 baud, 8N1 with hardware flow control
 * ============================================================ */
static void USART3_Init(void)
{
    RCC->APB1ENR1 |= (1U << 18);  /* USART3EN */

    /* BRR = 170000000 / 115200 = 1476 */
    USART3->BRR = 1476;
    USART3->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
    USART3->CR2 = 0;
    USART3->CR3 = (1U << 8) | (1U << 9);  /* CTSE, RTSE (HW flow) */
}

/* ============================================================
 * Simple delay (approximate, for initialization)
 * ============================================================ */
static void delay_ms(uint32_t ms)
{
    /* At 170 MHz, each loop iteration is ~6 cycles → ~35 ns
     * For 1 ms, we need ~28500 iterations */
    volatile uint32_t count;
    while (ms--) {
        count = 28500;
        while (count--)
            ;
    }
}

/* ============================================================
 * LED Control
 * ============================================================ */
static void led_set(uint8_t led, uint8_t state)
{
    /* LEDs are active low */
    switch (led) {
    case 0: /* PWR (PA3) */
        if (state) GPIOA->ODR &= ~(1U << 3);
        else       GPIOA->ODR |=  (1U << 3);
        break;
    case 1: /* TRIG (PA1) */
        if (state) GPIOA->ODR &= ~(1U << 1);
        else       GPIOA->ODR |=  (1U << 1);
        break;
    case 2: /* USB (PA2) */
        if (state) GPIOA->ODR &= ~(1U << 2);
        else       GPIOA->ODR |=  (1U << 2);
        break;
    case 3: /* RUN (PC3) */
        if (state) GPIOC->ODR &= ~(1U << 3);
        else       GPIOC->ODR |=  (1U << 3);
        break;
    }
}

/* ============================================================
 * Button Read
 * ============================================================ */
static uint8_t button_pressed(uint8_t btn)
{
    switch (btn) {
    case 0: return !(GPIOA->IDR & (1U << 0));   /* PA0: RUN/STOP */
    case 1: return !(GPIOC->IDR & (1U << 14));   /* PC14: SINGLE */
    case 2: return !(GPIOC->IDR & (1U << 15));   /* PC15: AUTO */
    case 3: return !(GPIOB->IDR & (1U << 12));   /* PB12: FORCE */
    default: return 0;
    }
}

/* ============================================================
 * FPGA Configuration Sequence
 * ============================================================ */
static void fpga_configure(void)
{
    /* 1. Assert PROG_N low to reset FPGA */
    GPIOD->ODR &= ~(1U << 4);   /* FPGA_PROG_N = low */
    delay_ms(10);

    /* 2. Release PROG_N, wait for INIT_N to go high */
    GPIOD->ODR |= (1U << 4);    /* FPGA_PROG_N = high (pull-up) */
    delay_ms(50);

    /* 3. Load FPGA bitstream from SPI flash */
    fpga_config_load();

    /* 4. Wait for FPGA DONE to go high */
    uint32_t timeout = 1000;  /* 1 second timeout */
    while (!(GPIOD->IDR & (1U << 6)) && timeout--)
        delay_ms(1);

    if (timeout == 0) {
        /* FPGA configuration failed — blink PWR LED rapidly */
        while (1) {
            led_set(0, 1);
            delay_ms(100);
            led_set(0, 0);
            delay_ms(100);
        }
    }
}

/* ============================================================
 * FPGA Register Configuration
 * ============================================================ */
static void fpga_setup_defaults(void)
{
    /* Configure trigger engine: edge trigger, channel 1, mid-scale threshold */
    fpga_spi_write(0x0000, TRIG_TYPE_EDGE_RISING);
    fpga_spi_write(0x0004, 0x80);     /* CH1 threshold = 128 (mid-scale) */
    fpga_spi_write(0x0014, 0x04);     /* Hysteresis = 4 LSB */

    /* Configure decimator: 1:1 (no decimation) */
    fpga_spi_write(0x10000, 1);        /* Rate = 1 (no decimation) */
    fpga_spi_write(0x10004, 0);        /* Mode = raw (no averaging) */

    /* Enable all 4 analog channels, 2 digital channels */
    fpga_spi_write(0x30004, 0x0F);    /* ANA_EN = channels 0-3 */
    fpga_spi_write(0x30008, 0x03);    /* DIG_EN = channels 0-1 */

    /* Set digital thresholds (mid-scale = 1.25V for 2.5V range) */
    dac_set_channel(0, 2048);          /* CMP1_THRESH_P ≈ 1.25 V */
    dac_set_channel(1, 0);             /* CMP1_THRESH_N = 0 V (GND reference) */
    dac_set_channel(2, 2048);          /* CMP2_THRESH_P ≈ 1.25 V */
    dac_set_channel(3, 0);             /* CMP2_THRESH_N = 0 V */

    /* Configure Wi-Fi DMA UART: 4 MHz baud, streaming mode */
    fpga_spi_write(0x80004, 4000000);  /* UART baud = 4 MHz */
}

/* ============================================================
 * Main Application State Machine
 * ============================================================ */
typedef enum {
    STATE_IDLE,
    STATE_ARMED,
    STATE_TRIGGERED,
    STATE_ACQUIRING,
    STATE_DUMPING,
    STATE_ERROR
} app_state_t;

static volatile app_state_t g_state = STATE_IDLE;
static volatile uint32_t g_trigger_channel = 0;
static volatile uint32_t g_sample_count = 0;

int main(void)
{
    /* 1. System clock configuration */
    SystemClock_Config();

    /* 2. GPIO initialization */
    GPIO_Init();

    /* 3. Power-on indicator */
    led_set(0, 1);  /* PWR LED on */
    delay_ms(100);

    /* 4. Initialize I2C buses */
    I2C1_Init();    /* PMIC bus */
    I2C2_Init();    /* USB PD bus */

    /* 5. Initialize PMIC and enable power rails */
    pmic_init();
    delay_ms(50);   /* Wait for rails to stabilize */
    pmic_enable_rails();
    delay_ms(100);  /* Settle time for all rails */

    /* 6. Negotiate USB PD (request 15V) */
    usb_pd_init();
    uint8_t pd_voltage = usb_pd_negotiate();
    (void)pd_voltage;  /* Used for VBUS monitoring */

    /* 7. Initialize SPI buses */
    SPI1_Init();    /* Flash */
    SPI2_Init();    /* DAC */
    SPI4_Init();    /* FPGA registers */

    /* 8. Initialize DAC */
    dac60508_init();

    /* 9. Initialize USART for Wi-Fi */
    USART3_Init();

    /* 10. Configure and load FPGA */
    fpga_configure();
    led_set(3, 1);  /* RUN LED on */

    /* 11. Set FPGA defaults */
    fpga_setup_defaults();

    /* 12. Initialize Wi-Fi module */
    esp32_wifi_init();

    /* 13. Main application loop */
    while (1) {
        /* Read FPGA status */
        uint32_t status = fpga_spi_read(0x60000);

        /* State machine */
        switch (g_state) {
        case STATE_IDLE:
            /* Check for arm command from host or button press */
            if (button_pressed(0)) {  /* RUN/STOP button */
                /* Arm the trigger */
                fpga_spi_write(0x0000, TRIG_TYPE_EDGE_RISING);
                g_state = STATE_ARMED;
                led_set(3, 1);  /* RUN LED on */
            }
            break;

        case STATE_ARMED:
            /* Check if FPGA reports trigger event */
            if (status & FPGA_STATUS_TRIGGERED) {
                g_state = STATE_TRIGGERED;
                led_set(1, 1);  /* TRIG LED on */
            }
            /* Check for stop command */
            if (button_pressed(0)) {
                fpga_spi_write(0x0000, 0);  /* Disable trigger */
                g_state = STATE_IDLE;
                led_set(3, 0);  /* RUN LED off */
                led_set(1, 0);  /* TRIG LED off */
            }
            break;

        case STATE_TRIGGERED:
            /* Wait for acquisition to complete */
            if (status & FPGA_STATUS_DONE) {
                g_state = STATE_DUMPING;
                g_sample_count = fpga_spi_read(0x60008);
            }
            break;

        case STATE_DUMPING:
            /* Data is being streamed via USB or Wi-Fi */
            /* FPGA handles DMA transfer autonomously */
            if (status & FPGA_STATUS_USB_RDY || status & FPGA_STATUS_WIFI_RDY) {
                g_state = STATE_IDLE;
                led_set(1, 0);  /* TRIG LED off */
            }
            break;

        case STATE_ERROR:
            led_set(0, 0);  /* PWR LED off */
            delay_ms(500);
            led_set(0, 1);  /* PWR LED on */
            delay_ms(500);
            break;
        }

        /* Small delay to avoid busy-waiting */
        delay_ms(1);
    }

    return 0;
}