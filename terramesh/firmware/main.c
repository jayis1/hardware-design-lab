/**
 * @file    main.c
 * @brief   Terramesh geotechnical node — main firmware
 * @author  jayis1
 * @copyright Copyright © 2026 jayis1
 * @license GPL-2.0
 *
 * Main super-loop with interrupt-driven state machine for ultra-low-power
 * subterranean geotechnical monitoring. Spends >99.9% of time in STOP2
 * sleep mode (2.5 µA), waking on RTC alarm, tilt interrupt, or LoRa RX.
 */

#include <stdint.h>
#include <string.h>
#include "board.h"
#include "registers.h"

/* ======================================================================== *
 *  Forward declarations                                                      *
 * ======================================================================== */
static void system_clock_init(void);
static void gpio_init(void);
static void spi1_init(void);
static void spi2_init(void);
static void i2c1_init(void);
static void i2c2_init(void);
static void usart2_init(void);
static void lpuart1_init(void);
static void adc_init(void);
static void rtc_init(void);
static void iwdg_init(void);
static void systick_init(void);
static void exti_init(void);
static void nvic_init(void);
static void enter_stop2_sleep(void);
static void wakeup_clock_restore(void);
static void read_all_sensors(sensor_record_t *rec);
static classification_t classify_anomaly(const sensor_record_t *rec);
static void log_to_flash(const sensor_record_t *rec);
static void enqueue_mesh_tx(const sensor_record_t *rec);
static void process_mesh_rx(void);
static void process_lte_uplink(void);
static void update_routing_table(void);
static void led_set_classification(classification_t cls);
static void debug_printf(const char *fmt, ...);
static uint32_t get_timestamp_s(void);
static uint32_t crc32_calc(const uint8_t *data, uint32_t len);
static void flash_erase_sector(uint32_t sector);
static void flash_write_page(uint32_t addr, const uint8_t *data, uint32_t len);
static void flash_read(uint32_t addr, uint8_t *data, uint32_t len);

/* ======================================================================== *
 *  Global state                                                               *
 * ======================================================================== */
static volatile system_state_t g_state = STATE_SLEEP;
static volatile uint32_t g_sys_tick_ms = 0;
static volatile bool g_rtc_alarm_flag = false;
static volatile bool g_tilt_int_flag = false;
static volatile bool g_lora_rx_flag = false;
static volatile bool g_lte_timer_flag = false;

static sensor_record_t g_last_record;
static classification_t g_last_classification = CLASS_NORMAL;
static uint32_t g_last_sample_time_s = 0;
static uint32_t g_last_lora_tx_time_s = 0;
static uint32_t g_last_lte_uplink_time_s = 0;
static uint32_t g_sample_interval_s = NORMAL_SAMPLE_INTERVAL_S;
static uint32_t g_lora_tx_interval_s = LORA_TX_INTERVAL_S;

static route_entry_t g_routing_table[ROUTE_TABLE_SIZE];
static uint16_t g_node_id = 0x0001;  /* Default, set during commissioning */
static uint16_t g_seq_num = 0;
static uint8_t g_mesh_tx_buffer[MESH_PACKET_MAX_SIZE];
static uint8_t g_mesh_rx_buffer[MESH_PACKET_MAX_SIZE];
static uint32_t g_log_write_index = 0;
static uint16_t g_battery_mv = 7200;

/* ======================================================================== *
 *  SysTick handler (1 ms interrupt)                                           *
 * ======================================================================== */
void SysTick_Handler(void) {
    g_sys_tick_ms++;
}

/* ======================================================================== *
 *  RTC alarm interrupt                                                        *
 * ======================================================================== */
void RTC_Alarm_IRQHandler(void) {
    /* Clear RTC alarm flag */
    REG_WRITE(RTC_BASE + 0x0C, 0x00000100);  /* RTC_SCR: clear ALRAF */
    g_rtc_alarm_flag = true;
}

/* ======================================================================== *
 *  EXTI handlers — tilt interrupt (ADXL372 INT1 on PB5)                       *
 * ======================================================================== */
void EXTI9_5_IRQHandler(void) {
    if (REG_READ(EXTI_PR1) & PIN_ADXL372_INT1) {
        REG_WRITE(EXTI_PR1, PIN_ADXL372_INT1);
        g_tilt_int_flag = true;
    }
}

/* ======================================================================== *
 *  EXTI handler — SX1262 DIO1 (LoRa RX/TX done)                               *
 * ======================================================================== */
void EXTI0_IRQHandler(void) {
    if (REG_READ(EXTI_PR1) & PIN_SX1262_DIO1) {
        REG_WRITE(EXTI_PR1, PIN_SX1262_DIO1);
        g_lora_rx_flag = true;
    }
}

/* ======================================================================== *
 *  System clock initialization                                                 *
 * ======================================================================== */
static void system_clock_init(void) {
    /* Enable HSI16 oscillator */
    REG_SET_BITS(RCC_CR, RCC_CR_HSI16ON);
    while (!(REG_READ(RCC_CR) & RCC_CR_HSI16RDYF)) { __asm__("nop"); }

    /* Configure PLL: HSI16 / 2 * 80 / 4 = 160 MHz */
    REG_WRITE(RCC_PLLCFGR,
        RCC_PLLCFGR_PLLSRC_HSI16 |
        (2UL << RCC_PLLCFGR_PLLM_POS) |    /* PLLM = 2 */
        (80UL << RCC_PLLCFGR_PLLN_POS) |   /* PLLN = 80 */
        RCC_PLLCFGR_PLLP_DIV4 |            /* PLLP = /4 → 160 MHz */
        (4UL << RCC_PLLCFGR_PLLQ_POS));    /* PLLQ = /8 → 80 MHz */

    /* Enable PLL */
    REG_SET_BITS(RCC_CR, RCC_CR_PLLON);
    while (!(REG_READ(RCC_CR) & RCC_CR_PLLRDY)) { __asm__("nop"); }

    /* Configure flash wait states for 160 MHz (4 WS) */
    REG_WRITE(FLASH_ACR, FLASH_ACR_LATENCY_4WS | FLASH_ACR_PRFTEN |
              FLASH_ACR_ICEN | FLASH_ACR_DCEN);

    /* Switch system clock to PLL */
    REG_WRITE_BF(RCC_CFGR, 1, 0, RCC_CFGR_SW_PLL);
    while (REG_READ_BF(RCC_CFGR, 3, 2) != RCC_CFGR_SWS_PLL) { __asm__("nop"); }

    /* Enable peripheral clocks */
    REG_SET_BITS(RCC_AHB1ENR, RCC_AHB1ENR_DMA1EN | RCC_AHB1ENR_CRCEN);
    REG_SET_BITS(RCC_AHB2ENR, RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN |
                 RCC_AHB2ENR_GPIOCEN | RCC_AHB2ENR_GPIODEN | RCC_AHB2ENR_ADCEN);
    REG_SET_BITS(RCC_APB1ENR1, RCC_APB1ENR1_SPI2EN | RCC_APB1ENR1_USART2EN |
                 RCC_APB1ENR1_I2C1EN | RCC_APB1ENR1_I2C2EN | RCC_APB1ENR1_LPUART1EN);
    REG_SET_BITS(RCC_APB2ENR, RCC_APB2ENR_SPI1EN);
}

/* ======================================================================== *
 *  GPIO initialization                                                         *
 * ======================================================================== */
static void gpio_init(void) {
    /* === SPI1 pins (PA0–PA7): AF5 === */
    GPIO_SET_MODE(GPIOA_BASE, PIN_SPI1_SCK, GPIO_MODE_AF);
    GPIO_SET_MODE(GPIOA_BASE, PIN_SPI1_MISO, GPIO_MODE_AF);
    GPIO_SET_MODE(GPIOA_BASE, PIN_SPI1_MOSI, GPIO_MODE_AF);
    GPIO_SET_AF(GPIOA_BASE, PIN_SPI1_SCK, 5);
    GPIO_SET_AF(GPIOA_BASE, PIN_SPI1_MISO, 5);
    GPIO_SET_AF(GPIOA_BASE, PIN_SPI1_MOSI, 5);

    /* SX1262 control pins: output, push-pull */
    GPIO_SET_MODE(GPIOA_BASE, PIN_SX1262_NSS, GPIO_MODE_OUTPUT);
    GPIO_SET_MODE(GPIOA_BASE, PIN_FLASH_CS, GPIO_MODE_OUTPUT);
    GPIO_SET_MODE(GPIOA_BASE, PIN_SX1262_RESET, GPIO_MODE_OUTPUT);
    GPIO_SET_MODE(GPIOA_BASE, PIN_SX1262_BUSY, GPIO_MODE_INPUT);
    GPIO_SET_MODE(GPIOA_BASE, PIN_SX1262_DIO1, GPIO_MODE_INPUT);
    GPIO_SET_PUPDR(GPIOA_BASE, PIN_SX1262_NSS, GPIO_PUPD_PULLUP);
    GPIO_SET_PUPDR(GPIOA_BASE, PIN_FLASH_CS, GPIO_PUPD_PULLUP);
    GPIO_SET_PUPDR(GPIOA_BASE, PIN_SX1262_BUSY, GPIO_PUPD_PULLDOWN);
    GPIO_SET_PUPDR(GPIOA_BASE, PIN_SX1262_DIO1, GPIO_PUPD_PULLDOWN);

    /* De-assert chip selects */
    GPIO_SET_PIN(GPIOA_BASE, PIN_SX1262_NSS);
    GPIO_SET_PIN(GPIOA_BASE, PIN_FLASH_CS);

    /* === SPI2 pins (PB0–PB4): AF5 === */
    GPIO_SET_MODE(GPIOB_BASE, PIN_SPI2_SCK, GPIO_MODE_AF);
    GPIO_SET_MODE(GPIOB_BASE, PIN_SPI2_MISO, GPIO_MODE_AF);
    GPIO_SET_MODE(GPIOB_BASE, PIN_SPI2_MOSI, GPIO_MODE_AF);
    GPIO_SET_AF(GPIOB_BASE, PIN_SPI2_SCK, 5);
    GPIO_SET_AF(GPIOB_BASE, PIN_SPI2_MISO, 5);
    GPIO_SET_AF(GPIOB_BASE, PIN_SPI2_MOSI, 5);

    GPIO_SET_MODE(GPIOB_BASE, PIN_SCL3300_CS, GPIO_MODE_OUTPUT);
    GPIO_SET_MODE(GPIOB_BASE, PIN_ADXL372_CS, GPIO_MODE_OUTPUT);
    GPIO_SET_PIN(GPIOB_BASE, PIN_SCL3300_CS);
    GPIO_SET_PIN(GPIOB_BASE, PIN_ADXL372_CS);

    /* ADXL372 INT1: input with pull-down */
    GPIO_SET_MODE(GPIOB_BASE, PIN_ADXL372_INT1, GPIO_MODE_INPUT);
    GPIO_SET_PUPDR(GPIOB_BASE, PIN_ADXL372_INT1, GPIO_PUPD_PULLDOWN);

    /* === I2C1 pins (PB6–PB7): AF4, open-drain === */
    GPIO_SET_MODE(GPIOB_BASE, PIN_I2C1_SCL, GPIO_MODE_AF);
    GPIO_SET_MODE(GPIOB_BASE, PIN_I2C1_SDA, GPIO_MODE_AF);
    GPIO_SET_OTYPER(GPIOB_BASE, PIN_I2C1_SCL, GPIO_OTYPE_OD);
    GPIO_SET_OTYPER(GPIOB_BASE, PIN_I2C1_SDA, GPIO_OTYPE_OD);
    GPIO_SET_AF(GPIOB_BASE, PIN_I2C1_SCL, 4);
    GPIO_SET_AF(GPIOB_BASE, PIN_I2C1_SDA, 4);

    /* === I2C2 pins (PB8–PB9): AF4, open-drain === */
    GPIO_SET_MODE(GPIOB_BASE, PIN_I2C2_SCL, GPIO_MODE_AF);
    GPIO_SET_MODE(GPIOB_BASE, PIN_I2C2_SDA, GPIO_MODE_AF);
    GPIO_SET_OTYPER(GPIOB_BASE, PIN_I2C2_SCL, GPIO_OTYPE_OD);
    GPIO_SET_OTYPER(GPIOB_BASE, PIN_I2C2_SDA, GPIO_OTYPE_OD);
    GPIO_SET_AF(GPIOB_BASE, PIN_I2C2_SCL, 4);
    GPIO_SET_AF(GPIOB_BASE, PIN_I2C2_SDA, 4);

    /* === USART2 pins (PC4–PC5): AF7 === */
    GPIO_SET_MODE(GPIOC_BASE, PIN_USART2_TX, GPIO_MODE_AF);
    GPIO_SET_MODE(GPIOC_BASE, PIN_USART2_RX, GPIO_MODE_AF);
    GPIO_SET_AF(GPIOC_BASE, PIN_USART2_TX, 7);
    GPIO_SET_AF(GPIOC_BASE, PIN_USART2_RX, 7);

    /* === LPUART1 pins (PC0–PC1): AF8 === */
    GPIO_SET_MODE(GPIOC_BASE, PIN_LPUART1_TX, GPIO_MODE_AF);
    GPIO_SET_MODE(GPIOC_BASE, PIN_LPUART1_RX, GPIO_MODE_AF);
    GPIO_SET_AF(GPIOC_BASE, PIN_LPUART1_TX, 8);
    GPIO_SET_AF(GPIOC_BASE, PIN_LPUART1_RX, 8);

    /* BG95 control pins */
    GPIO_SET_MODE(GPIOC_BASE, PIN_BG95_PWRKEY, GPIO_MODE_OUTPUT);
    GPIO_SET_MODE(GPIOC_BASE, PIN_BG95_STATUS, GPIO_MODE_INPUT);
    GPIO_CLR_PIN(GPIOC_BASE, PIN_BG95_PWRKEY);

    /* === Interrupt inputs (PC6–PC9): input, pull-up === */
    GPIO_SET_MODE(GPIOC_BASE, PIN_RTC_INT, GPIO_MODE_INPUT);
    GPIO_SET_MODE(GPIOC_BASE, PIN_SCL3300_DRDY, GPIO_MODE_INPUT);
    GPIO_SET_MODE(GPIOC_BASE, PIN_ADS1120_1_DRDY, GPIO_MODE_INPUT);
    GPIO_SET_MODE(GPIOC_BASE, PIN_ADS1120_2_DRDY, GPIO_MODE_INPUT);
    GPIO_SET_PUPDR(GPIOC_BASE, PIN_RTC_INT, GPIO_PUPD_PULLUP);
    GPIO_SET_PUPDR(GPIOC_BASE, PIN_SCL3300_DRDY, GPIO_PUPD_PULLUP);
    GPIO_SET_PUPDR(GPIOC_BASE, PIN_ADS1120_1_DRDY, GPIO_PUPD_PULLUP);
    GPIO_SET_PUPDR(GPIOC_BASE, PIN_ADS1120_2_DRDY, GPIO_PUPD_PULLUP);

    /* === Misc pins (PD0–PD6) === */
    GPIO_SET_MODE(GPIOD_BASE, PIN_MOISTURE_EN, GPIO_MODE_OUTPUT);
    GPIO_SET_MODE(GPIOD_BASE, PIN_LED_RED, GPIO_MODE_OUTPUT);
    GPIO_SET_MODE(GPIOD_BASE, PIN_LED_GREEN, GPIO_MODE_OUTPUT);
    GPIO_SET_MODE(GPIOD_BASE, PIN_PMIC_EN, GPIO_MODE_OUTPUT);
    GPIO_SET_MODE(GPIOD_BASE, PIN_LTE_EN, GPIO_MODE_OUTPUT);
    GPIO_SET_MODE(GPIOD_BASE, PIN_BATTERY_ADC, GPIO_MODE_ANALOG);
    GPIO_SET_MODE(GPIOD_BASE, PIN_MOISTURE_ADC, GPIO_MODE_ANALOG);

    /* Initial state: all off */
    GPIO_CLR_PIN(GPIOD_BASE, PIN_MOISTURE_EN);
    GPIO_CLR_PIN(GPIOD_BASE, PIN_LED_RED);
    GPIO_CLR_PIN(GPIOD_BASE, PIN_LED_GREEN);
    GPIO_CLR_PIN(GPIOD_BASE, PIN_PMIC_EN);
    GPIO_CLR_PIN(GPIOD_BASE, PIN_LTE_EN);

    /* Set unused pins to analog mode for lowest power */
    /* (GPIOA pins 8–15, GPIOB 10–15, GPIOC 10–15, GPIOD 7–15) */
    for (int i = 8; i < 16; i++) {
        GPIO_SET_MODE(GPIOA_BASE, 1UL << i, GPIO_MODE_ANALOG);
        GPIO_SET_MODE(GPIOB_BASE, 1UL << i, GPIO_MODE_ANALOG);
        GPIO_SET_MODE(GPIOC_BASE, 1UL << i, GPIO_MODE_ANALOG);
        GPIO_SET_MODE(GPIOD_BASE, 1UL << i, GPIO_MODE_ANALOG);
    }
}

/* ======================================================================== *
 *  SPI1 initialization (LoRa + Flash, 10 MHz, mode 0)                        *
 * ======================================================================== */
static void spi1_init(void) {
    /* Disable SPI1 */
    REG_CLR_BITS(SPI_CR1(SPI1_BASE), SPI_CR1_SPE);

    /* Configure CR1: master, CPOL=0, CPHA=0, BR=div2 (40 MHz, SX1262 max 10 MHz) */
    REG_WRITE(SPI_CR1(SPI1_BASE),
        SPI_CR1_MSTR |
        (SPI_BR_DIV8 << SPI_CR1_BR_POS) |  /* 80 MHz / 8 = 10 MHz */
        SPI_CR1_SSI | SPI_CR1_SSM);         /* Software slave management */

    /* Configure CR2: 8-bit data, SSOE disabled */
    REG_WRITE(SPI_CR2(SPI1_BASE), SPI_CR2_DS_8BIT);

    /* Enable SPI1 */
    REG_SET_BITS(SPI_CR1(SPI1_BASE), SPI_CR1_SPE);
}

/* ======================================================================== *
 *  SPI2 initialization (Inclinometer + Accel, 8 MHz, mode 3)                 *
 * ======================================================================== */
static void spi2_init(void) {
    REG_CLR_BITS(SPI_CR1(SPI2_BASE), SPI_CR1_SPE);

    REG_WRITE(SPI_CR1(SPI2_BASE),
        SPI_CR1_MSTR |
        SPI_CR1_CPOL | SPI_CR1_CPHA |       /* Mode 3 */
        (SPI_BR_DIV10 << SPI_CR1_BR_POS) |  /* 80 MHz / 10 = 8 MHz */
        SPI_CR1_SSI | SPI_CR1_SSM);

    REG_WRITE(SPI_CR2(SPI2_BASE), SPI_CR2_DS_8BIT);
    REG_SET_BITS(SPI_CR1(SPI2_BASE), SPI_CR1_SPE);
}

/* ======================================================================== *
 *  I²C1 initialization (400 kHz)                                               *
 * ======================================================================== */
static void i2c1_init(void) {
    /* Disable I2C1 */
    REG_CLR_BITS(I2C_CR1(I2C1_BASE), I2C_CR1_PE);

    /* Timing for 400 kHz with 80 MHz APB1 clock */
    /* STM32U5 I2C timing: PRESC=1, SCLDEL=4, SDADEL=2, SCLH=6, SCLL=8 */
    REG_WRITE(I2C_TIMINGR(I2C1_BASE), 0x00420608);

    /* Enable I2C1 */
    REG_SET_BITS(I2C_CR1(I2C1_BASE), I2C_CR1_PE);
}

/* ======================================================================== *
 *  I²C2 initialization (100 kHz for ADS1120)                                   *
 * ======================================================================== */
static void i2c2_init(void) {
    REG_CLR_BITS(I2C_CR1(I2C2_BASE), I2C_CR1_PE);

    /* Timing for 100 kHz with 80 MHz APB1 clock */
    REG_WRITE(I2C_TIMINGR(I2C2_BASE), 0x00B01C3C);

    REG_SET_BITS(I2C_CR1(I2C2_BASE), I2C_CR1_PE);
}

/* ======================================================================== *
 *  USART2 initialization (115200 baud, 8N1, for USB CDC-ACM debug)            *
 * ======================================================================== */
static void usart2_init(void) {
    REG_CLR_BITS(USART_CR1(USART2_BASE), USART_CR1_UE);

    /* 80 MHz / 115200 = 694.44 → BRR = 694 */
    REG_WRITE(USART_BRR(USART2_BASE), 694);

    /* Enable TX, RX, and USART */
    REG_WRITE(USART_CR1(USART2_BASE), USART_CR1_UE | USART_CR1_TE | USART_CR1_RE);
}

/* ======================================================================== *
 *  LPUART1 initialization (9600 baud for BG95-M3 LTE module)                   *
 * ======================================================================== */
static void lpuart1_init(void) {
    REG_CLR_BITS(USART_CR1(LPUART1_BASE), USART_CR1_UE);

    /* LPUART1 clock = LSE 32.768 kHz, 9600 baud */
    /* BRR = 32768 / 9600 = 3.41 → use 3 with fractional */
    REG_WRITE(USART_BRR(LPUART1_BASE), 3);

    REG_WRITE(USART_CR1(LPUART1_BASE), USART_CR1_UE | USART_CR1_TE | USART_CR1_RE);
}

/* ======================================================================== *
 *  ADC initialization (battery + moisture sense)                               *
 * ======================================================================== */
static void adc_init(void) {
    /* Enable ADC voltage regulator */
    REG_SET_BITS(ADC1_BASE + 0x04, (1UL << 28));  /* ADC_CR: ADVREGEN */

    /* Wait for regulator startup */
    for (volatile uint32_t i = 0; i < 10000; i++) { __asm__("nop"); }

    /* Calibrate ADC */
    REG_SET_BITS(ADC1_BASE + 0x04, (1UL << 31));  /* ADCAL */
    while (REG_READ(ADC1_BASE + 0x04) & (1UL << 31)) { __asm__("nop"); }

    /* Configure: 12-bit, single conversion, software trigger */
    REG_WRITE(ADC1_BASE + 0x08, 0x00000000);  /* ADC_CFGR */
    REG_WRITE(ADC1_BASE + 0x0C, 0x00000000);  /* ADC_CFGR2 */

    /* Enable ADC */
    REG_SET_BITS(ADC1_BASE + 0x00, (1UL << 0));  /* ADC_CR: ADEN */
    while (!(REG_READ(ADC1_BASE + 0x00) & (1UL << 0))) { __asm__("nop"); }
}

/* ======================================================================== *
 *  RTC initialization (LSE, alarm every NORMAL_SAMPLE_INTERVAL_S)             *
 * ======================================================================== */
static void rtc_init(void) {
    /* Enable PWR clock and allow access to backup domain */
    REG_SET_BITS(RCC_APB1ENR1, RCC_APB1ENR1_LPUART1EN);  /* PWREN is on APB1 */
    /* Note: PWREN bit is at position 28 in APB1ENR1 */
    REG_SET_BITS(RCC_APB1ENR1, (1UL << 28));

    /* Enable backup domain access */
    REG_SET_BITS(PWR_CR1, (1UL << 0));  /* PWR_CR1: DBP */

    /* Enable LSE oscillator */
    REG_SET_BITS(RCC_BDCR, (1UL << 0));  /* LSEON */
    while (!(REG_READ(RCC_BDCR) & (1UL << 1))) { __asm__("nop"); }  /* LSERDY */

    /* Select LSE as RTC clock */
    REG_WRITE_BF(RCC_BDCR, 9, 8, 1);  /* RTCSEL = 01 (LSE) */

    /* Enable RTC */
    REG_SET_BITS(RCC_BDCR, (1UL << 15));  /* RTCEN */

    /* Wait for RTC register sync */
    REG_WRITE(RTC_BASE + 0x08, 0x00000001);  /* RTC_WPR: disable write protection */
    REG_WRITE(RTC_BASE + 0x08, 0x00000002);
    REG_WRITE(RTC_BASE + 0x08, 0x00000001);

    /* Set RTC prescaler: 32768 / (127 + 1) * (255 + 1) = 1 Hz */
    REG_WRITE(RTC_BASE + 0x1C, 0x0000007F);  /* RTC_PRER: sync = 127 */
    REG_WRITE(RTC_BASE + 0x20, 0x000000FF);  /* RTC_PRER: async = 255 */

    /* Set alarm to fire every NORMAL_SAMPLE_INTERVAL_S */
    /* For simplicity, set alarm to current time + interval */
    uint32_t current_time = REG_READ(RTC_BASE + 0x24);  /* RTC_TR (time reg) */
    uint32_t alarm_time = current_time + NORMAL_SAMPLE_INTERVAL_S;
    REG_WRITE(RTC_BASE + 0x2C, alarm_time);  /* RTC_ALRMAR */

    /* Enable alarm interrupt */
    REG_SET_BITS(RTC_BASE + 0x04, (1UL << 8));  /* RTC_CR: ALRAIE */
}

/* ======================================================================== *
 *  IWDG initialization (10 second timeout)                                     *
 * ======================================================================== */
static void iwdg_init(void) {
    /* Enable access to IWDG registers */
    REG_WRITE(IWDG_KR, IWDG_KR_KEY_ACCESS);

    /* Set prescaler to 256: 32.768 kHz / 256 = 128 Hz */
    REG_WRITE(IWDG_PR, 0x00000006);  /* PR[2:0] = 110 → /256 */

    /* Set reload value: 128 Hz × 10 s = 1280 */
    REG_WRITE(IWDG_RLR, 1280);

    /* Start IWDG */
    REG_WRITE(IWDG_KR, IWDG_KR_KEY_ENABLE);
}

/* ======================================================================== *
 *  SysTick initialization (1 ms)                                                *
 * ======================================================================== */
static void systick_init(void) {
    /* SysTick at 160 MHz / 160000 = 1 kHz */
    REG_WRITE(SYSTICK_RVR, 160000 - 1);
    REG_WRITE(SYSTICK_CVR, 0);
    REG_WRITE(SYSTICK_CSR, SYSTICK_CSR_ENABLE | SYSTICK_CSR_TICKINT | SYSTICK_CSR_CLKSOURCE);
}

/* ======================================================================== *
 *  EXTI initialization                                                         *
 * ======================================================================== */
static void exti_init(void) {
    /* EXTI line 5 (PB5 = ADXL372 INT1): rising edge trigger */
    REG_WRITE(EXTI_EXTICR2, (REG_READ(EXTI_EXTICR2) & ~(0xFUL << 8)) | (1UL << 8));
    REG_SET_BITS(EXTI_RTSR1, PIN_ADXL372_INT1);
    REG_SET_BITS(EXTI_IMR1, PIN_ADXL372_INT1);

    /* EXTI line 5 (PA5 = SX1262 DIO1): rising edge trigger */
    REG_WRITE(EXTI_EXTICR1, (REG_READ(EXTI_EXTICR1) & ~(0xFUL << 20)) | (0UL << 20));
    REG_SET_BITS(EXTI_RTSR1, PIN_SX1262_DIO1);
    REG_SET_BITS(EXTI_IMR1, PIN_SX1262_DIO1);
}

/* ======================================================================== *
 *  NVIC initialization                                                         *
 * ======================================================================== */
static void nvic_init(void) {
    NVIC_SET_PRIORITY(IRQ_RTC_ALARM, 1);
    NVIC_SET_PRIORITY(IRQ_EXTI9_5, 2);
    NVIC_SET_PRIORITY(IRQ_EXTI0, 2);
    NVIC_SET_PRIORITY(SysTick_IRQn, 3);

    NVIC_ENABLE_IRQ(IRQ_RTC_ALARM);
    NVIC_ENABLE_IRQ(IRQ_EXTI9_5);
    NVIC_ENABLE_IRQ(IRQ_EXTI0);
}

/* ======================================================================== *
 *  Enter STOP2 sleep mode (2.5 µA typical)                                      *
 * ======================================================================== */
static void enter_stop2_sleep(void) {
    /* Set STOP2 mode */
    REG_SET_BITS(PWR_CR1, PWR_CR1_STOP2);

    /* Clear SLEEPDEEP and set SLEEPONEXIT */
    REG_CLR_BITS(SCB_SCR, SCB_SLEEPDEEP);
    REG_CLR_BITS(SCB_SCR, SCB_SLEEPONEXIT);

    /* Ensure all pending operations complete */
    __asm__("dsb");
    __asm__("isb");

    /* Wait for interrupt (WFI) */
    __asm__("wfi");
}

/* ======================================================================== *
 *  Wakeup clock restore (after STOP2)                                          *
 * ======================================================================== */
static void wakeup_clock_restore(void) {
    /* After STOP2, HSI16 is enabled automatically. Re-lock PLL. */
    REG_SET_BITS(RCC_CR, RCC_CR_PLLON);
    while (!(REG_READ(RCC_CR) & RCC_CR_PLLRDY)) { __asm__("nop"); }

    /* Switch back to PLL */
    REG_WRITE_BF(RCC_CFGR, 1, 0, RCC_CFGR_SW_PLL);
    while (REG_READ_BF(RCC_CFGR, 3, 2) != RCC_CFGR_SWS_PLL) { __asm__("nop"); }
}

/* ======================================================================== *
 *  SPI1 transfer (blocking, 8-bit)                                             *
 * ======================================================================== */
static uint8_t spi1_transfer(uint8_t tx_byte) {
    /* Wait for TX buffer empty */
    while (!(REG_READ(SPI_SR(SPI1_BASE)) & SPI_SR_TXE)) { __asm__("nop"); }
    REG_WRITE(SPI_DR(SPI1_BASE), tx_byte);
    /* Wait for RX buffer not empty */
    while (!(REG_READ(SPI_SR(SPI1_BASE)) & SPI_SR_RXNE)) { __asm__("nop"); }
    return (uint8_t)REG_READ(SPI_DR(SPI1_BASE));
}

/* ======================================================================== *
 *  SPI1 multi-byte transfer (blocking)                                          *
 * ======================================================================== */
static void spi1_transfer_buf(const uint8_t *tx, uint8_t *rx, uint32_t len) {
    for (uint32_t i = 0; i < len; i++) {
        uint8_t t = (tx) ? tx[i] : 0xFF;
        uint8_t r = spi1_transfer(t);
        if (rx) rx[i] = r;
    }
}

/* ======================================================================== *
 *  SPI2 transfer (blocking, 8-bit)                                             *
 * ======================================================================== */
static uint8_t spi2_transfer(uint8_t tx_byte) {
    while (!(REG_READ(SPI_SR(SPI2_BASE)) & SPI_SR_TXE)) { __asm__("nop"); }
    REG_WRITE(SPI_DR(SPI2_BASE), tx_byte);
    while (!(REG_READ(SPI_SR(SPI2_BASE)) & SPI_SR_RXNE)) { __asm__("nop"); }
    return (uint8_t)REG_READ(SPI_DR(SPI2_BASE));
}

/* ======================================================================== *
 *  I²C1 write (blocking)                                                        *
 * ======================================================================== */
static bool i2c1_write(uint8_t dev_addr, const uint8_t *data, uint32_t len) {
    /* Wait for bus not busy */
    while (REG_READ(I2C_ISR(I2C1_BASE)) & I2C_ISR_BUSY) { __asm__("nop"); }

    /* Configure transfer: dev_addr << 1, write, len bytes, auto-end */
    REG_WRITE(I2C_CR2(I2C1_BASE),
        ((uint32_t)dev_addr << I2C_CR2_SADD_POS) |
        (len << I2C_CR2_NBYTES_POS) |
        I2C_CR2_AUTOEND |
        I2C_CR2_START);

    /* Send data */
    for (uint32_t i = 0; i < len; i++) {
        while (!(REG_READ(I2C_ISR(I2C1_BASE)) & I2C_ISR_TXE)) { __asm__("nop"); }
        REG_WRITE(I2C_DR(I2C1_BASE), data[i]);
    }

    /* Wait for stop condition */
    while (!(REG_READ(I2C_ISR(I2C1_BASE)) & I2C_ISR_TC)) {
        if (REG_READ(I2C_ISR(I2C1_BASE)) & I2C_ISR_NACKF) {
            REG_WRITE(I2C_ICR(I2C1_BASE), I2C_ISR_NACKF);
            return false;
        }
        __asm__("nop");
    }

    return true;
}

/* ======================================================================== *
 *  I²C1 read (blocking)                                                         *
 * ======================================================================== */
static bool i2c1_read(uint8_t dev_addr, uint8_t *data, uint32_t len) {
    while (REG_READ(I2C_ISR(I2C1_BASE)) & I2C_ISR_BUSY) { __asm__("nop"); }

    /* Configure transfer: dev_addr << 1 | 1 (read), len bytes, auto-end */
    REG_WRITE(I2C_CR2(I2C1_BASE),
        ((uint32_t)dev_addr << I2C_CR2_SADD_POS) |
        I2C_CR2_RD_WRN |
        (len << I2C_CR2_NBYTES_POS) |
        I2C_CR2_AUTOEND |
        I2C_CR2_START);

    /* Receive data */
    for (uint32_t i = 0; i < len; i++) {
        while (!(REG_READ(I2C_ISR(I2C1_BASE)) & I2C_ISR_RXNE)) {
            if (REG_READ(I2C_ISR(I2C1_BASE)) & I2C_ISR_NACKF) {
                REG_WRITE(I2C_ICR(I2C1_BASE), I2C_ISR_NACKF);
                return false;
            }
            __asm__("nop");
        }
        data[i] = (uint8_t)REG_READ(I2C_DR(I2C1_BASE));
    }

    return true;
}

/* ======================================================================== *
 *  I²C2 write (blocking, for ADS1120 ADCs)                                      *
 * ======================================================================== */
static bool i2c2_write(uint8_t dev_addr, const uint8_t *data, uint32_t len) {
    while (REG_READ(I2C_ISR(I2C2_BASE)) & I2C_ISR_BUSY) { __asm__("nop"); }

    REG_WRITE(I2C_CR2(I2C2_BASE),
        ((uint32_t)dev_addr << I2C_CR2_SADD_POS) |
        (len << I2C_CR2_NBYTES_POS) |
        I2C_CR2_AUTOEND |
        I2C_CR2_START);

    for (uint32_t i = 0; i < len; i++) {
        while (!(REG_READ(I2C_ISR(I2C2_BASE)) & I2C_ISR_TXE)) { __asm__("nop"); }
        REG_WRITE(I2C_DR(I2C2_BASE), data[i]);
    }

    while (!(REG_READ(I2C_ISR(I2C2_BASE)) & I2C_ISR_TC)) {
        if (REG_READ(I2C_ISR(I2C2_BASE)) & I2C_ISR_NACKF) {
            REG_WRITE(I2C_ICR(I2C2_BASE), I2C_ISR_NACKF);
            return false;
        }
        __asm__("nop");
    }
    return true;
}

/* ======================================================================== *
 *  I²C2 read (blocking)                                                         *
 * ======================================================================== */
static bool i2c2_read(uint8_t dev_addr, uint8_t *data, uint32_t len) {
    while (REG_READ(I2C_ISR(I2C2_BASE)) & I2C_ISR_BUSY) { __asm__("nop"); }

    REG_WRITE(I2C_CR2(I2C2_BASE),
        ((uint32_t)dev_addr << I2C_CR2_SADD_POS) |
        I2C_CR2_RD_WRN |
        (len << I2C_CR2_NBYTES_POS) |
        I2C_CR2_AUTOEND |
        I2C_CR2_START);

    for (uint32_t i = 0; i < len; i++) {
        while (!(REG_READ(I2C_ISR(I2C2_BASE)) & I2C_ISR_RXNE)) {
            if (REG_READ(I2C_ISR(I2C2_BASE)) & I2C_ISR_NACKF) {
                REG_WRITE(I2C_ICR(I2C2_BASE), I2C_ISR_NACKF);
                return false;
            }
            __asm__("nop");
        }
        data[i] = (uint8_t)REG_READ(I2C_DR(I2C2_BASE));
    }
    return true;
}

/* ======================================================================== *
 *  USART2 debug print (blocking, simple)                                       *
 * ======================================================================== */
static void debug_putchar(char c) {
    while (!(REG_READ(USART_ISR(USART2_BASE)) & USART_ISR_TXE)) { __asm__("nop"); }
    REG_WRITE(USART_TDR(USART2_BASE), (uint8_t)c);
}

static void debug_printf(const char *fmt, ...) {
    char buf[128];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    for (char *p = buf; *p; p++) {
        if (*p == '\n') debug_putchar('\r');
        debug_putchar(*p);
    }
}

/* ======================================================================== *
 *  Timestamp (simplified — uses RTC counter)                                    *
 * ======================================================================== */
static uint32_t get_timestamp_s(void) {
    return REG_READ(RTC_BASE + 0x24);  /* RTC_TR (simplified) */
}

/* ======================================================================== *
 *  CRC32 calculation (using hardware CRC peripheral)                           *
 * ======================================================================== */
static uint32_t crc32_calc(const uint8_t *data, uint32_t len) {
    /* Reset CRC peripheral */
    REG_WRITE(CRC_CR, CRC_CR_RESET);

    /* Feed data */
    for (uint32_t i = 0; i < len; i++) {
        REG_WRITE(CRC_DR, data[i]);
    }

    return REG_READ(CRC_DR);
}

/* ======================================================================== *
 *  Flash erase sector                                                           *
 * ======================================================================== */
static void flash_erase_sector(uint32_t sector) {
    /* Wait for previous operation */
    while (REG_READ(FLASH_SR) & FLASH_SR_BSY) { __asm__("nop"); }

    /* Unlock flash */
    REG_WRITE(FLASH_KEYR, 0x45670123);
    REG_WRITE(FLASH_KEYR, 0xCDEF89AB);

    /* Configure sector erase */
    REG_WRITE(FLASH_CR, FLASH_CR_SER | ((sector << FLASH_CR_SNB_POS) & FLASH_CR_SNB_MASK));
    REG_SET_BITS(FLASH_CR, FLASH_CR_START);

    /* Wait for completion */
    while (REG_READ(FLASH_SR) & FLASH_SR_BSY) { __asm__("nop"); }

    /* Lock flash */
    REG_SET_BITS(FLASH_CR, FLASH_CR_LOCK);
}

/* ======================================================================== *
 *  Flash write page (256 bytes)                                                 *
 * ======================================================================== */
static void flash_write_page(uint32_t addr, const uint8_t *data, uint32_t len) {
    while (REG_READ(FLASH_SR) & FLASH_SR_BSY) { __asm__("nop"); }

    REG_WRITE(FLASH_KEYR, 0x45670123);
    REG_WRITE(FLASH_KEYR, 0xCDEF89AB);

    REG_SET_BITS(FLASH_CR, FLASH_CR_PG);

    /* Write 64-bit words */
    for (uint32_t i = 0; i < len; i += 8) {
        uint64_t word = 0;
        for (uint32_t j = 0; j < 8 && (i + j) < len; j++) {
            word |= ((uint64_t)data[i + j]) << (j * 8);
        }
        *(volatile uint64_t *)(addr + i) = word;
        while (REG_READ(FLASH_SR) & FLASH_SR_BSY) { __asm__("nop"); }
    }

    REG_CLR_BITS(FLASH_CR, FLASH_CR_PG);
    REG_SET_BITS(FLASH_CR, FLASH_CR_LOCK);
}

/* ======================================================================== *
 *  Flash read                                                                   *
 * ======================================================================== */
static void flash_read(uint32_t addr, uint8_t *data, uint32_t len) {
    for (uint32_t i = 0; i < len; i++) {
        data[i] = *(volatile uint8_t *)(addr + i);
    }
}

/* ======================================================================== *
 *  SX1262 register write                                                        *
 * ======================================================================== */
static void sx1262_write_reg(uint16_t reg, uint8_t val) {
    uint8_t cmd[] = { 0x0D, (reg >> 8) & 0xFF, reg & 0xFF, val };
    GPIO_CLR_PIN(GPIOA_BASE, PIN_SX1262_NSS);
    spi1_transfer_buf(cmd, NULL, 4);
    GPIO_SET_PIN(GPIOA_BASE, PIN_SX1262_NSS);
}

/* ======================================================================== *
 *  SX1262 register read                                                         *
 * ======================================================================== */
static uint8_t sx1262_read_reg(uint16_t reg) {
    uint8_t cmd[] = { 0x1D, (reg >> 8) & 0xFF, reg & 0xFF, 0x00 };
    uint8_t rx[4];
    GPIO_CLR_PIN(GPIOA_BASE, PIN_SX1262_NSS);
    spi1_transfer_buf(cmd, rx, 4);
    GPIO_SET_PIN(GPIOA_BASE, PIN_SX1262_NSS);
    return rx[3];
}

/* ======================================================================== *
 *  SX1262 set operating mode                                                    *
 * ======================================================================== */
static void sx1262_set_opmode(uint8_t mode) {
    uint8_t cmd[] = { 0x80 | mode };
    GPIO_CLR_PIN(GPIOA_BASE, PIN_SX1262_NSS);
    spi1_transfer_buf(cmd, NULL, 1);
    GPIO_SET_PIN(GPIOA_BASE, PIN_SX1262_NSS);
}

/* ======================================================================== *
 *  SX1262 initialize                                                            *
 * ======================================================================== */
static void sx1262_init(void) {
    /* Reset SX1262 */
    GPIO_CLR_PIN(GPIOA_BASE, PIN_SX1262_RESET);
    for (volatile uint32_t i = 0; i < 1000; i++) { __asm__("nop"); }
    GPIO_SET_PIN(GPIOA_BASE, PIN_SX1262_RESET);
    for (volatile uint32_t i = 0; i < 10000; i++) { __asm__("nop"); }

    /* Wait for BUSY to go low */
    while (GPIO_READ_PIN(GPIOA_BASE, PIN_SX1262_BUSY)) { __asm__("nop"); }

    /* Set RF frequency: 868.5 MHz = (868.5e6 / 61.035) ≈ 14230340 */
    uint32_t freq = (uint32_t)((LORA_FREQ_HZ / 61.035f) + 0.5f);
    uint8_t freq_cmd[] = { 0x86, (freq >> 16) & 0xFF, (freq >> 8) & 0xFF, freq & 0xFF };
    GPIO_CLR_PIN(GPIOA_BASE, PIN_SX1262_NSS);
    spi1_transfer_buf(freq_cmd, NULL, 4);
    GPIO_SET_PIN(GPIOA_BASE, PIN_SX1262_NSS);

    /* Set TX params: +14 dBm */
    uint8_t tx_cmd[] = { 0x8E, 0x0E, 0x04 };  /* +14 dBm, ramp 40 µs */
    GPIO_CLR_PIN(GPIOA_BASE, PIN_SX1262_NSS);
    spi1_transfer_buf(tx_cmd, NULL, 3);
    GPIO_SET_PIN(GPIOA_BASE, PIN_SX1262_NSS);

    /* Set modulation params: SF10, BW 125 kHz, CR 4/5 */
    uint8_t mod_cmd[] = { 0x8B, LORA_SF, LORA_BW_KHZ, LORA_CR, 0x00, 0x00, 0x00, 0x00 };
    GPIO_CLR_PIN(GPIOA_BASE, PIN_SX1262_NSS);
    spi1_transfer_buf(mod_cmd, NULL, 8);
    GPIO_SET_PIN(GPIOA_BASE, PIN_SX1262_NSS);

    /* Set packet params: preamble 8, fixed length, CRC on */
    uint8_t pkt_cmd[] = { 0x8C, LORA_PREAMBLE_LEN, 0x00, MESH_PACKET_MAX_SIZE, 0x00, 0x00, 0x00, 0x01 };
    GPIO_CLR_PIN(GPIOA_BASE, PIN_SX1262_NSS);
    spi1_transfer_buf(pkt_cmd, NULL, 8);
    GPIO_SET_PIN(GPIOA_BASE, PIN_SX1262_NSS);

    /* Set sync word */
    sx1262_write_reg(0x0740, LORA_SYNC_WORD);

    /* Set DIO1 as TX_DONE / RX_DONE */
    sx1262_write_reg(0x0580, 0x00);  /* DIO1 mapping */

    /* Enter standby */
    sx1262_set_opmode(0x01);  /* STDBY_RC */
}

/* ======================================================================== *
 *  SX1262 send packet                                                           *
 * ======================================================================== */
static bool sx1262_send(const uint8_t *data, uint32_t len) {
    if (len > MESH_PACKET_MAX_SIZE) return false;

    /* Wait for BUSY */
    while (GPIO_READ_PIN(GPIOA_BASE, PIN_SX1262_BUSY)) { __asm__("nop"); }

    /* Write buffer */
    uint8_t buf_cmd[] = { 0x0E, 0x00, 0x00, (uint8_t)len };
    GPIO_CLR_PIN(GPIOA_BASE, PIN_SX1262_NSS);
    spi1_transfer_buf(buf_cmd, NULL, 4);
    spi1_transfer_buf(data, NULL, len);
    GPIO_SET_PIN(GPIOA_BASE, PIN_SX1262_NSS);

    /* Set TX mode */
    sx1262_set_opmode(0x03);  /* TX */

    /* Wait for TX done (DIO1 interrupt) */
    uint32_t timeout = 5000;  /* 5 s timeout */
    while (!g_lora_rx_flag && timeout--) {
        __asm__("nop");
    }

    /* Enter standby */
    sx1262_set_opmode(0x01);

    return g_lora_rx_flag;
}

/* ======================================================================== *
 *  SX1262 enter receive mode                                                    *
 * ======================================================================== */
static void sx1262_rx(void) {
    while (GPIO_READ_PIN(GPIOA_BASE, PIN_SX1262_BUSY)) { __asm__("nop"); }

    /* Set RX mode with 5 s timeout */
    uint8_t rx_cmd[] = { 0x82, 0x00, 0x00, 0x00 };
    GPIO_CLR_PIN(GPIOA_BASE, PIN_SX1262_NSS);
    spi1_transfer_buf(rx_cmd, NULL, 4);
    GPIO_SET_PIN(GPIOA_BASE, PIN_SX1262_NSS);
}

/* ======================================================================== *
 *  SX1262 read received packet                                                  *
 * ======================================================================== */
static uint32_t sx1262_read_rx_packet(uint8_t *buf, uint32_t max_len) {
    while (GPIO_READ_PIN(GPIOA_BASE, PIN_SX1262_BUSY)) { __asm__("nop"); }

    /* Get RX buffer status */
    uint8_t cmd[] = { 0x12, 0x00, 0x00 };
    uint8_t rx[3];
    GPIO_CLR_PIN(GPIOA_BASE, PIN_SX1262_NSS);
    spi1_transfer_buf(cmd, rx, 3);
    GPIO_SET_PIN(GPIOA_BASE, PIN_SX1262_NSS);

    uint8_t payload_len = rx[1];
    uint8_t rx_start = rx[0];

    if (payload_len > max_len) payload_len = max_len;

    /* Read payload */
    uint8_t read_cmd[] = { 0x1E, rx_start, 0x00 };
    GPIO_CLR_PIN(GPIOA_BASE, PIN_SX1262_NSS);
    spi1_transfer_buf(read_cmd, NULL, 3);
    spi1_transfer_buf(NULL, buf, payload_len);
    GPIO_SET_PIN(GPIOA_BASE, PIN_SX1262_NSS);

    return payload_len;
}

/* ======================================================================== *
 *  SCL3300 read angle (SPI, mode 3)                                            *
 * ======================================================================== */
static bool scl3300_read_angles(float *x_deg, float *y_deg) {
    uint8_t rx[6];

    /* Select SCL3300 */
    GPIO_CLR_PIN(GPIOB_BASE, PIN_SCL3300_CS);

    /* Read angle data: 0x0400 | 0x02 = 0x0402 (auto-increment from 0x04) */
    uint8_t cmd[] = { 0x04, 0x02 };
    spi2_transfer(cmd[0]);
    spi2_transfer(cmd[1]);

    /* Read 6 bytes (3 × 16-bit) */
    for (int i = 0; i < 6; i++) {
        rx[i] = spi2_transfer(0x00);
    }

    GPIO_SET_PIN(GPIOB_BASE, PIN_SCL3300_CS);

    /* Parse: each axis is 16-bit signed, 0.001°/LSB */
    int16_t raw_x = (int16_t)((rx[0] << 8) | rx[1]);
    int16_t raw_y = (int16_t)((rx[2] << 8) | rx[3]);

    /* Check status bits (bits 14:12 should be 001 for normal) */
    if ((rx[0] & 0x70) != 0x10) return false;

    *x_deg = (float)raw_x * 0.001f;
    *y_deg = (float)raw_y * 0.001f;

    return true;
}

/* ======================================================================== *
 *  ADXL372 read acceleration (SPI, mode 0)                                     *
 * ======================================================================== */
static bool adxl372_read_accel(float *x_g, float *y_g, float *z_g) {
    uint8_t rx[6];

    GPIO_CLR_PIN(GPIOB_BASE, PIN_ADXL372_CS);

    /* Read XDATA, YDATA, ZDATA (0x08, auto-increment) */
    uint8_t cmd = 0x08 | 0x80;  /* Read with MSB=1 for multi-byte */
    spi2_transfer(cmd);
    for (int i = 0; i < 6; i++) {
        rx[i] = spi2_transfer(0x00);
    }

    GPIO_SET_PIN(GPIOB_BASE, PIN_ADXL372_CS);

    /* Parse: 12-bit signed, 50 mg/LSB */
    int16_t raw_x = (int16_t)((rx[0] << 4) | (rx[1] >> 4));
    int16_t raw_y = (int16_t)((rx[2] << 4) | (rx[3] >> 4));
    int16_t raw_z = (int16_t)((rx[4] << 4) | (rx[5] >> 4));

    /* Sign-extend from 12-bit */
    if (raw_x & 0x0800) raw_x |= 0xF000;
    if (raw_y & 0x0800) raw_y |= 0xF000;
    if (raw_z & 0x0800) raw_z |= 0xF000;

    *x_g = (float)raw_x * 0.050f;
    *y_g = (float)raw_y * 0.050f;
    *z_g = (float)raw_z * 0.050f;

    return true;
}

/* ======================================================================== *
 *  BME688 read environment (I²C)                                               *
 * ======================================================================== */
static bool bme688_read(float *temp_c, float *press_pa, float *humid_pct) {
    uint8_t reg = 0x22;  /* Register for temperature MSB */
    uint8_t rx[8];

    /* Write register address */
    if (!i2c1_write(I2C_ADDR_BME688, &reg, 1)) return false;

    /* Read 8 bytes: temp (3), press (3), humid (2) */
    if (!i2c1_read(I2C_ADDR_BME688, rx, 8)) return false;

    /* Parse raw values */
    uint32_t raw_temp = ((uint32_t)rx[0] << 12) | ((uint32_t)rx[1] << 4) | (rx[2] >> 4);
    uint32_t raw_press = ((uint32_t)rx[3] << 12) | ((uint32_t)rx[4] << 4) | (rx[5] >> 4);
    uint16_t raw_humid = ((uint16_t)rx[6] << 8) | rx[7];

    /* Simplified conversion (calibration data would be read from BME688 at init) */
    *temp_c = (float)raw_temp * 0.01f - 40.0f;
    *press_pa = (float)raw_press * 0.01f;
    *humid_pct = (float)raw_humid * 0.001f;

    return true;
}

/* ======================================================================== *
 *  ADS1120 read pore pressure (I²C)                                            *
 * ======================================================================== */
static bool ads1120_read_pressure(uint8_t dev_addr, float *pressure_kpa) {
    uint8_t config = 0x02;  /* PGA=1, continuous conversion, 20 SPS */
    uint8_t rx[2];

    /* Write config register */
    uint8_t write_cmd[] = { 0x40, config };
    if (!i2c2_write(dev_addr, write_cmd, 2)) return false;

    /* Wait for conversion */
    for (volatile uint32_t i = 0; i < 5000; i++) { __asm__("nop"); }

    /* Read conversion result */
    uint8_t reg = 0x00;
    if (!i2c2_write(dev_addr, &reg, 1)) return false;
    if (!i2c2_read(dev_addr, rx, 2)) return false;

    int16_t raw = (int16_t)((rx[0] << 8) | rx[1]);

    /* Convert: 16-bit, ±500 kPa range, PGA=1 */
    *pressure_kpa = (float)raw * (500.0f / 32768.0f);

    return true;
}

/* ======================================================================== *
 *  Read moisture probe (capacitance, ADC)                                      *
 * ======================================================================== */
static float read_moisture(void) {
    /* Enable moisture probe */
    GPIO_SET_PIN(GPIOD_BASE, PIN_MOISTURE_EN);

    /* Settling time */
    for (volatile uint32_t i = 0; i < 1000; i++) { __asm__("nop"); }

    /* Start ADC conversion on channel 2 (PD2) */
    REG_WRITE(ADC1_BASE + 0x10, 0x00000002);  /* ADC_SQR1: SQ1 = ch 2 */
    REG_SET_BITS(ADC1_BASE + 0x00, (1UL << 2));  /* ADSTART */

    /* Wait for conversion complete */
    while (!(REG_READ(ADC1_BASE + 0x00) & (1UL << 2))) { __asm__("nop"); }

    uint32_t adc_val = REG_READ(ADC1_BASE + 0x40);  /* ADC_DR */

    /* Disable moisture probe */
    GPIO_CLR_PIN(GPIOD_BASE, PIN_MOISTURE_EN);

    /* Convert to % VWC (calibration-dependent) */
    return (float)adc_val * (100.0f / 4095.0f);
}

/* ======================================================================== *
 *  Read battery voltage                                                         *
 * ======================================================================== */
static uint16_t read_battery_mv(void) {
    /* Start ADC conversion on channel 1 (PD1) */
    REG_WRITE(ADC1_BASE + 0x10, 0x00000001);  /* ADC_SQR1: SQ1 = ch 1 */
    REG_SET_BITS(ADC1_BASE + 0x00, (1UL << 2));

    while (!(REG_READ(ADC1_BASE + 0x00) & (1UL << 2))) { __asm__("nop"); }

    uint32_t adc_val = REG_READ(ADC1_BASE + 0x40);

    /* Voltage divider: 1/3 ratio, 12-bit ADC, VREF=3.3V */
    float v_bat = (float)adc_val * (3.3f / 4095.0f) * 3.0f;
    return (uint16_t)(v_bat * 1000.0f);
}

/* ======================================================================== *
 *  Read all sensors and populate record                                        *
 * ======================================================================== */
static void read_all_sensors(sensor_record_t *rec) {
    float temp_c, press_pa, humid_pct;
    float tilt_x, tilt_y;
    float accel_x, accel_y, accel_z;
    float press_shallow_kpa, press_deep_kpa;

    memset(rec, 0, sizeof(sensor_record_t));

    /* Read timestamp */
    rec->timestamp = get_timestamp_s();

    /* Read BME688 environment */
    if (bme688_read(&temp_c, &press_pa, &humid_pct)) {
        rec->temperature = (int16_t)(temp_c * 100.0f);
        rec->pressure = (uint16_t)(press_pa);
    }

    /* Read SCL3300 tilt */
    if (scl3300_read_angles(&tilt_x, &tilt_y)) {
        rec->tilt_x = (int16_t)(tilt_x * 1000.0f);
        rec->tilt_y = (int16_t)(tilt_y * 1000.0f);
    }

    /* Read ADXL372 acceleration */
    if (adxl372_read_accel(&accel_x, &accel_y, &accel_z)) {
        rec->accel_x = (int16_t)(accel_x * 1000.0f);
        rec->accel_y = (int16_t)(accel_y * 1000.0f);
        rec->accel_z = (int16_t)(accel_z * 1000.0f);
    }

    /* Read ADS1120 pore pressure (shallow) */
    if (ads1120_read_pressure(I2C_ADDR_ADS1120_1, &press_shallow_kpa)) {
        rec->pore_press_shallow = (uint16_t)(press_shallow_kpa * 100.0f);
    }

    /* Read ADS1120 pore pressure (deep) */
    if (ads1120_read_pressure(I2C_ADDR_ADS1120_2, &press_deep_kpa)) {
        rec->pore_press_deep = (uint16_t)(press_deep_kpa * 100.0f);
    }

    /* Read moisture probe */
    rec->moisture = (uint16_t)(read_moisture() * 100.0f);

    /* Read battery voltage */
    g_battery_mv = read_battery_mv();
    rec->battery_mv = g_battery_mv / 20;
}

/* ======================================================================== *
 *  On-node classification decision tree                                        *
 * ======================================================================== */
static classification_t classify_anomaly(const sensor_record_t *rec) {
    /* Compute deltas from baseline (stored in flash at commissioning) */
    /* For this implementation, use hardcoded baseline values */
    static const float baseline_p_shallow = 50.0f;   /* 50 kPa */
    static const float baseline_p_deep = 30.0f;      /* 30 kPa */
    static const float baseline_tilt = 0.0f;          /* 0° */
    static const float baseline_moisture = 20.0f;     /* 20% VWC */

    float p_shallow = (float)rec->pore_press_shallow * 0.01f;
    float p_deep = (float)rec->pore_press_deep * 0.01f;
    float tilt = (float)rec->tilt_x * 0.001f;  /* Use X-axis */
    float moisture = (float)rec->moisture * 0.01f;

    float dp_shallow = p_shallow - baseline_p_shallow;
    float dp_deep = p_deep - baseline_p_deep;
    float dtilt = (tilt > 0) ? tilt : -tilt;
    float dmoisture = moisture - baseline_moisture;

    /* Track tilt rate (over last 3 samples) */
    static float prev_tilt = 0.0f;
    static uint32_t prev_time = 0;
    float tilt_rate = 0.0f;
    if (prev_time != 0 && rec->timestamp > prev_time) {
        float dt = (float)(rec->timestamp - prev_time) / 3600.0f;  /* hours */
        if (dt > 0.0f) {
            tilt_rate = (tilt - prev_tilt) / dt;
        }
    }
    prev_tilt = tilt;
    prev_time = rec->timestamp;

    /* Decision tree */
    if (dp_deep > THRESH_PORE_DEEP_KPA && dmoisture > THRESH_MOISTURE_PCT) {
        /* Perched water table forming */
        if (tilt_rate > THRESH_TILT_RATE_CRITICAL) {
            return CLASS_CRITICAL;  /* Shear surface lubrication likely */
        }
        return CLASS_WARNING;
    }

    if (dp_shallow > THRESH_PORE_SHALLOW_KPA && dtilt > THRESH_TILT_DEG) {
        return CLASS_WARNING;  /* Rainfall infiltration + minor movement */
    }

    if (tilt_rate > THRESH_TILT_RATE_CRITICAL) {
        return CLASS_CRITICAL;  /* Rapid movement, possible failure */
    }

    return CLASS_NORMAL;
}

/* ======================================================================== *
 *  Log record to flash                                                         *
 * ======================================================================== */
static void log_to_flash(const sensor_record_t *rec) {
    uint32_t log_addr = LOG_START + (g_log_write_index * sizeof(sensor_record_t));

    /* Check if we need to erase next sector */
    uint32_t sector = (g_log_write_index * sizeof(sensor_record_t)) / (LOG_SECTOR_SIZE_KB * 1024);
    static uint32_t last_erased_sector = 0xFFFFFFFF;

    if (sector != last_erased_sector) {
        flash_erase_sector(sector);
        last_erased_sector = sector;
    }

    /* Write record */
    flash_write_page(log_addr, (const uint8_t *)rec, sizeof(sensor_record_t));

    g_log_write_index++;
}

/* ======================================================================== *
 *  Enqueue mesh TX packet                                                      *
 * ======================================================================== */
static void enqueue_mesh_tx(const sensor_record_t *rec) {
    mesh_packet_header_t *hdr = (mesh_packet_header_t *)g_mesh_tx_buffer;

    /* Build header */
    memset(hdr, 0, MESH_HEADER_SIZE);
    hdr->dest_addr = MESH_BROADCAST_ADDR;
    hdr->src_addr = g_node_id;
    hdr->seq_num = g_seq_num++;
    hdr->hop_count = MESH_MAX_HOPS;
    hdr->msg_type = MSG_TYPE_DATA;

    /* Copy payload (sensor record) */
    uint32_t payload_len = sizeof(sensor_record_t);
    memcpy(hdr->payload, rec, payload_len);

    /* Append CRC32 */
    uint32_t crc = crc32_calc((const uint8_t *)hdr, MESH_HEADER_SIZE - 4 + payload_len);
    uint8_t *crc_ptr = (uint8_t *)hdr + MESH_HEADER_SIZE - 4 + payload_len;
    crc_ptr[0] = (crc >> 24) & 0xFF;
    crc_ptr[1] = (crc >> 16) & 0xFF;
    crc_ptr[2] = (crc >> 8) & 0xFF;
    crc_ptr[3] = crc & 0xFF;

    uint32_t total_len = MESH_HEADER_SIZE - 4 + payload_len + 4;

    /* Send over LoRa */
    if (sx1262_send(g_mesh_tx_buffer, total_len)) {
        debug_printf("[TM] TX OK: seq=%u, type=%d, len=%lu\r\n",
                     hdr->seq_num, hdr->msg_type, (unsigned long)total_len);
    } else {
        debug_printf("[TM] TX FAIL: seq=%u\r\n", hdr->seq_num);
    }
}

/* ======================================================================== *
 *  Process received mesh packet                                                *
 * ======================================================================== */
static void process_mesh_rx(void) {
    uint32_t len = sx1262_read_rx_packet(g_mesh_rx_buffer, MESH_PACKET_MAX_SIZE);
    if (len < MESH_HEADER_SIZE) return;

    mesh_packet_header_t *hdr = (mesh_packet_header_t *)g_mesh_rx_buffer;

    /* Verify CRC */
    uint32_t payload_len = len - MESH_HEADER_SIZE;
    uint32_t crc = crc32_calc((const uint8_t *)hdr, MESH_HEADER_SIZE - 4 + payload_len);
    uint8_t *crc_ptr = (uint8_t *)hdr + MESH_HEADER_SIZE - 4 + payload_len;
    uint32_t rx_crc = ((uint32_t)crc_ptr[0] << 24) |
                      ((uint32_t)crc_ptr[1] << 16) |
                      ((uint32_t)crc_ptr[2] << 8) |
                      crc_ptr[3];
    if (crc != rx_crc) {
        debug_printf("[TM] CRC mismatch: calc=0x%08lX, rx=0x%08lX\r\n",
                     (unsigned long)crc, (unsigned long)rx_crc);
        return;
    }

    /* Deduplicate */
    static uint16_t last_seq[8];
    static uint16_t last_src[8];
    for (int i = 0; i < 8; i++) {
        if (last_src[i] == hdr->src_addr && last_seq[i] == hdr->seq_num) {
            return;  /* Already processed */
        }
    }
    /* Store in circular buffer */
    static int seq_idx = 0;
    last_src[seq_idx] = hdr->src_addr;
    last_seq[seq_idx] = hdr->seq_num;
    seq_idx = (seq_idx + 1) % 8;

    debug_printf("[TM] RX: src=0x%04X, seq=%u, type=%d, hop=%u\r\n",
                 hdr->src_addr, hdr->seq_num, hdr->msg_type, hdr->hop_count);

    /* Handle message type */
    switch (hdr->msg_type) {
        case MSG_TYPE_DATA:
            /* Forward if not for us and TTL > 0 */
            if (hdr->dest_addr != g_node_id && hdr->hop_count > 0) {
                hdr->hop_count--;
                sx1262_send(g_mesh_rx_buffer, len);
            }
            break;

        case MSG_TYPE_BEACON:
            /* Update routing table */
            update_routing_table();
            break;

        case MSG_TYPE_CMD:
            /* Process command (e.g., change sample interval) */
            if (hdr->dest_addr == g_node_id || hdr->dest_addr == MESH_BROADCAST_ADDR) {
                if (payload_len >= 4) {
                    uint32_t cmd = *(uint32_t *)hdr->payload;
                    if (cmd == 0x01) {  /* Set sample interval */
                        if (payload_len >= 8) {
                            g_sample_interval_s = *(uint32_t *)(hdr->payload + 4);
                        }
                    }
                }
            }
            break;

        default:
            break;
    }
}

/* ======================================================================== *
 *  Update routing table (from HELLO beacons)                                   *
 * ======================================================================== */
static void update_routing_table(void) {
    /* Simplified: just mark the source as reachable */
    mesh_packet_header_t *hdr = (mesh_packet_header_t *)g_mesh_rx_buffer;

    for (int i = 0; i < ROUTE_TABLE_SIZE; i++) {
        if (g_routing_table[i].dest_addr == hdr->src_addr) {
            g_routing_table[i].last_seen_s = get_timestamp_s();
            g_routing_table[i].valid = true;
            return;
        }
        if (!g_routing_table[i].valid) {
            g_routing_table[i].dest_addr = hdr->src_addr;
            g_routing_table[i].next_hop = hdr->src_addr;
            g_routing_table[i].hop_count = 1;
            g_routing_table[i].last_seen_s = get_timestamp_s();
            g_routing_table[i].seq_num = hdr->seq_num;
            g_routing_table[i].valid = true;
            return;
        }
    }
}

/* ======================================================================== *
 *  Process LTE uplink (gateway only)                                           *
 * ======================================================================== */
static void process_lte_uplink(void) {
    /* Only gateway node (ID=0x0000) performs LTE uplink */
    if (g_node_id != MESH_GATEWAY_ADDR) return;

    /* Power on BG95-M3 */
    GPIO_SET_PIN(GPIOD_BASE, PIN_LTE_EN);
    for (volatile uint32_t i = 0; i < 10000; i++) { __asm__("nop"); }

    /* Pulse PWRKEY for 1 second */
    GPIO_SET_PIN(GPIOC_BASE, PIN_BG95_PWRKEY);
    for (volatile uint32_t i = 0; i < 100000; i++) { __asm__("nop"); }
    GPIO_CLR_PIN(GPIOC_BASE, PIN_BG95_PWRKEY);

    /* Wait for module to register on network */
    for (volatile uint32_t i = 0; i < 500000; i++) { __asm__("nop"); }

    /* Send AT command to check network registration */
    const char *at_cmd = "AT+CREG?\r\n";
    for (const char *p = at_cmd; *p; p++) {
        while (!(REG_READ(USART_ISR(LPUART1_BASE)) & USART_ISR_TXE)) { __asm__("nop"); }
        REG_WRITE(USART_TDR(LPUART1_BASE), (uint8_t)*p);
    }

    /* Read response (simplified — would use DMA in production) */
    char response[64];
    uint32_t resp_idx = 0;
    for (volatile uint32_t i = 0; i < 50000; i++) {
        if (REG_READ(USART_ISR(LPUART1_BASE)) & USART_ISR_RXNE) {
            char c = (char)REG_READ(USART_RDR(LPUART1_BASE));
            if (resp_idx < sizeof(response) - 1) {
                response[resp_idx++] = c;
            }
        }
    }
    response[resp_idx] = '\0';

    debug_printf("[TM] LTE response: %s\r\n", response);

    /* Power off LTE */
    GPIO_CLR_PIN(GPIOD_BASE, PIN_LTE_EN);
}

/* ======================================================================== *
 *  Set LED based on classification                                             *
 * ======================================================================== */
static void led_set_classification(classification_t cls) {
    switch (cls) {
        case CLASS_NORMAL:
            GPIO_SET_PIN(GPIOD_BASE, PIN_LED_GREEN);
            GPIO_CLR_PIN(GPIOD_BASE, PIN_LED_RED);
            break;
        case CLASS_WARNING:
            GPIO_SET_PIN(GPIOD_BASE, PIN_LED_GREEN);
            GPIO_SET_PIN(GPIOD_BASE, PIN_LED_RED);
            break;
        case CLASS_CRITICAL:
            GPIO_CLR_PIN(GPIOD_BASE, PIN_LED_GREEN);
            GPIO_SET_PIN(GPIOD_BASE, PIN_LED_RED);
            break;
    }
}

/* ======================================================================== *
 *  Main entry point                                                             *
 * ======================================================================== */
int main(void) {
    /* Initialize all hardware */
    system_clock_init();
    gpio_init();
    spi1_init();
    spi2_init();
    i2c1_init();
    i2c2_init();
    usart2_init();
    lpuart1_init();
    adc_init();
    rtc_init();
    iwdg_init();
    systick_init();
    exti_init();
    nvic_init();
    sx1262_init();

    debug_printf("\r\n[TM] Terramesh v1.0 — Node 0x%04X\r\n", g_node_id);
    debug_printf("[TM] System initialized. Entering main loop.\r\n");

    /* Enable PMIC (3.3 V rail) */
    GPIO_SET_PIN(GPIOD_BASE, PIN_PMIC_EN);

    /* Main super-loop */
    while (1) {
        /* Reload watchdog */
        REG_WRITE(IWDG_KR, IWDG_KR_KEY_RELOAD);

        /* Check wake-up sources */
        if (g_rtc_alarm_flag) {
            g_rtc_alarm_flag = false;
            g_state = STATE_SENSE;
        }

        if (g_tilt_int_flag) {
            g_tilt_int_flag = false;
            /* Tilt interrupt triggers immediate sense cycle */
            g_state = STATE_SENSE;
        }

        if (g_lora_rx_flag) {
            g_lora_rx_flag = false;
            process_mesh_rx();
            /* Re-enter RX mode */
            sx1262_rx();
        }

        /* State machine */
        switch (g_state) {
            case STATE_SENSE: {
                uint32_t now = get_timestamp_s();

                /* Read all sensors */
                read_all_sensors(&g_last_record);

                /* Classify anomaly */
                g_last_classification = classify_anomaly(&g_last_record);
                g_last_record.classification = (uint8_t)g_last_classification;

                /* Update LED */
                led_set_classification(g_last_classification);

                /* Log to flash */
                log_to_flash(&g_last_record);

                /* Adjust intervals based on classification */
                if (is_anomaly(g_last_classification)) {
                    g_sample_interval_s = FAST_SAMPLE_INTERVAL_S;
                    g_lora_tx_interval_s = LORA_FAST_TX_INTERVAL_S;
                } else {
                    g_sample_interval_s = NORMAL_SAMPLE_INTERVAL_S;
                    g_lora_tx_interval_s = LORA_TX_INTERVAL_S;
                }

                /* Update RTC alarm for next sample */
                /* (In production, reprogram RTC alarm register) */

                debug_printf("[TM] Sample: t=%lu, P_sh=%.1f, P_de=%.1f, "
                            "M=%.1f%%, tilt=%.3f°, class=%d, bat=%umV\r\n",
                            (unsigned long)g_last_record.timestamp,
                            (float)g_last_record.pore_press_shallow * 0.01f,
                            (float)g_last_record.pore_press_deep * 0.01f,
                            (float)g_last_record.moisture * 0.01f,
                            (float)g_last_record.tilt_x * 0.001f,
                            g_last_classification,
                            g_battery_mv);

                g_last_sample_time_s = now;
                g_state = STATE_TX_QUEUE;
                break;
            }

            case STATE_TX_QUEUE: {
                uint32_t now = get_timestamp_s();

                /* Check if it's time to send LoRa */
                if ((now - g_last_lora_tx_time_s) >= g_lora_tx_interval_s) {
                    enqueue_mesh_tx(&g_last_record);
                    g_last_lora_tx_time_s = now;
                }

                /* Check if it's time for LTE uplink (gateway only) */
                if ((now - g_last_lte_uplink_time_s) >= LTE_UPLINK_INTERVAL_S) {
                    g_state = STATE_TX_LTE;
                } else {
                    g_state = STATE_SLEEP;
                }
                break;
            }

            case STATE_TX_LTE: {
                process_lte_uplink();
                g_last_lte_uplink_time_s = get_timestamp_s();
                g_state = STATE_SLEEP;
                break;
            }

            case STATE_SLEEP:
            default: {
                /* Enter RX mode for mesh listening */
                sx1262_rx();

                /* Enter STOP2 sleep */
                enter_stop2_sleep();

                /* Wakeup: restore clocks */
                wakeup_clock_restore();

                g_state = STATE_SENSE;
                break;
            }
        }
    }

    /* Should never reach here */
    // return 0;
}
