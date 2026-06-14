/*
 * SkyPilot — Dual-IMU Drone Flight Controller with LTE
 * main.c — SPL/Board init entry point
 *
 * Target: STM32H743VIT6 @ 480MHz
 * Toolchain: arm-none-eabi-gcc
 */

#include "board.h"
#include "registers.h"
#include "imu_icm42688.h"
#include "lte_lara_r6.h"

/* ---- linker symbols ---- */
extern uint32_t _estack;     /* from linker script */
extern uint32_t _sidata;      /* .data init values in Flash */
extern uint32_t _sdata;       /* .data start in RAM */
extern uint32_t _edata;       /* .data end in RAM */
extern uint32_t _sbss;        /* .bss start */
extern uint32_t _ebss;        /* .bss end */

/* ---- watchdog kick ---- */
static void watchdog_init(void) {
    /* IWDG: LSI ≈ 32kHz, prescaler /256 → ~125Hz, reload 250 → 2s */
    IWDG->KR  = 0x5555;                    /* enable register access */
    IWDG->PR  = 0x06;                      /* prescaler /256 */
    IWDG->RLR = 250;                        /* reload value */
    while (!(IWDG->SR & (1UL << 0)));      /* wait PVU */
    IWDG->KR  = 0xCCCC;                    /* start watchdog */
}

static inline void watchdog_kick(void) {
    IWDG->KR = 0xAAAA;
}

/* ---- SysTick (1ms tick for FreeRTOS / scheduling) ---- */
static volatile uint32_t systick_ms = 0;

void SysTick_Handler(void) {
    systick_ms++;
}

static void systick_init(void) {
    /* SysTick: 480MHz / 480000 = 1000Hz = 1ms tick */
    *(volatile uint32_t *)0xE000E014 = 480000 - 1;  /* RELOAD */
    *(volatile uint32_t *)0xE000E018 = 0;             /* VAL = 0 */
    *(volatile uint32_t *)0xE000E010 = 0x7;           /* ENABLE, TICKINT, CLKSOURCE */
}

uint32_t get_ms(void) {
    return systick_ms;
}

void delay_ms(uint32_t ms) {
    uint32_t start = systick_ms;
    while ((systick_ms - start) < ms);
}

/* ---- LED helpers ---- */
static inline void led_power_on(void)  { GPIOD->ODR |=  (1UL << LED_POWER_PIN); }
static inline void led_power_off(void) { GPIOD->ODR &= ~(1UL << LED_POWER_PIN); }
static inline void led_status_toggle(void) { GPIOE->ODR ^= (1UL << LED_STATUS_PIN); }
static inline void led_lte_toggle(void)   { GPIOD->ODR ^= (1UL << LED_LTE_PIN); }

/* ---- ADC init (battery / current sensing) ---- */
static void adc_init(void) {
    /* Enable ADC1 clock */
    RCC->AHB1ENR |= (1UL << 0);   /* DMA1 clock */
    RCC->APB2ENR |= (1UL << 16);  /* ADC1 clock */

    /* Configure PA0 (ADC1_INP16 = battery) and PA1 (ADC1_INP17 = current) */
    /* Set analog mode for PA0, PA1 */
    GPIOA->MODER = (GPIOA->MODER & ~(3UL << (0*2))) | (3UL << (0*2));  /* PA0 analog */
    GPIOA->MODER = (GPIOA->MODER & ~(3UL << (1*2))) | (3UL << (1*2));  /* PA1 analog */

    /* Disable deep-powerDown */
    ADC1_COMMON->CCR = 0;  /* No prescaler */

    /* Enable ADC voltage regulator */
    *(volatile uint32_t *)0x40022000 &= ~(3UL << 28);  /* ADVREGEN = 00 → enabled */
    for (volatile int i = 0; i < 1000; i++);  /* Wait for regulator startup */

    /* Calibrate ADC (single-ended) */
    *(volatile uint32_t *)0x40022008 |= (1UL << 16);  /* ADCAL */
    while (*(volatile uint32_t *)0x40022008 & (1UL << 16));  /* Wait for calibration */

    /* Enable ADC */
    *(volatile uint32_t *)0x40022008 |= (1UL << 0);  /* ADEN */
    while (!(*(volatile uint32_t *)0x40022008 & (1UL << 0)));  /* Wait for ready */
}

static uint16_t adc_read_channel(int channel) {
    /* Configure channel */
    *(volatile uint32_t *)0x4002202C = (channel << 6) | (0UL << 3) | (0UL << 2);
    /* 12-bit resolution, single conversion */
    *(volatile uint32_t *)0x40022014 = 1;  /* L = 1 conversion */
    *(volatile uint32_t *)0x40022020 = (channel << 6);  /* SQR1: channel selection */

    /* Start conversion */
    *(volatile uint32_t *)0x40022008 |= (1UL << 2);  /* ADSTART */

    /* Wait for end of conversion */
    while (!(*(volatile uint32_t *)0x40022008 & (1UL << 2)));  /* EOC */

    /* Read data */
    return (uint16_t)(*(volatile uint32_t *)0x40022040 & 0xFFFF);
}

/* ---- Battery voltage reading ---- */
float battery_read_voltage(void) {
    uint16_t raw = adc_read_channel(16);  /* ADC1_INP16 = PA0 */
    /* Voltage divider: R5=10kΩ, R6=1.5kΩ → Vadc = Vbat × R6/(R5+R6) = Vbat/7.33 */
    /* ADC: Vref = 3.3V, 12-bit → Vadc = raw × 3.3 / 4095 */
    float vadc = (float)raw * 3.3f / 4095.0f;
    float vbat = vadc * 7.333f;  /* Restore original voltage */
    return vbat;
}

/* ---- Reset handler (entry point) ---- */
void Reset_Handler(void) {
    uint32_t *src, *dst;

    /* Copy .data from Flash to DTCM */
    src = &_sidata;
    dst = &_sdata;
    while (dst < &_edata) {
        *dst++ = *src++;
    }

    /* Zero .bss */
    dst = &_sbss;
    while (dst < &_ebss) {
        *dst++ = 0;
    }

    /* Set stack pointer (MSP = top of DTCM) */
    *(volatile uint32_t *)0xE000ED08 = (uint32_t)&_estack;  /* VTOR */

    /* System init */
    board_init_clocks();
    board_init_gpio();

    /* Enable FPU */
    *(volatile uint32_t *)0xE000ED88 |= (0xFUL << 20);  /* CP10=11, CP11=11 */

    /* Init SysTick */
    systick_init();

    /* Init watchdog */
    watchdog_init();

    /* Power LED on */
    led_power_on();

    /* Init ADC for battery sensing */
    adc_init();

    /* Init primary IMU */
    int imu1_ok = icm42688_init();
    if (imu1_ok != 0) {
        /* IMU1 failed — blink status LED fast */
        for (int i = 0; i < 10; i++) {
            led_status_toggle();
            delay_ms(100);
        }
    }

    /* Init LTE modem */
    int lte_ok = lte_init();
    if (lte_ok != 0) {
        /* LTE init failed — blink LTE LED fast */
        for (int i = 0; i < 10; i++) {
            led_lte_toggle();
            delay_ms(100);
        }
    }

    /* Power on LTE modem */
    lte_power_on();

    /* Main loop */
    imu_scaled_t imu1_data;
    float battery_voltage;
    uint32_t last_imu_read = 0;
    uint32_t last_battery_read = 0;
    uint32_t last_telemetry = 0;
    uint32_t loop_count = 0;

    while (1) {
        watchdog_kick();
        loop_count++;

        /* Read IMU1 at 8kHz (every 125µs — we oversample slightly here) */
        uint32_t now = systick_ms;
        if (now - last_imu_read >= 1) {  /* ~1ms polling (simplified) */
            icm42688_read_scaled(&imu1_data);
            last_imu_read = now;
            led_status_toggle();  /* Heartbeat */
        }

        /* Read battery voltage every 500ms */
        if (now - last_battery_read >= 500) {
            battery_voltage = battery_read_voltage();
            last_battery_read = now;
        }

        /* Send telemetry every 100ms */
        if (now - last_telemetry >= 100) {
            /* In production: send MAVLink packet via LTE */
            last_telemetry = now;
        }
    }
}

/* ---- Default interrupt handlers ---- */
void NMI_Handler(void)           { while (1); }
void HardFault_Handler(void)     { while (1); }
void MemManage_Handler(void)     { while (1); }
void BusFault_Handler(void)      { while (1); }
void UsageFault_Handler(void)    { while (1); }
void SVC_Handler(void)           { /* FreeRTOS SVC */ }
void DebugMon_Handler(void)      { /* Debug monitor */ }
void PendSV_Handler(void)        { /* FreeRTOS context switch */ }

/* Vector table (placed at start of Flash) */
__attribute__((section(".isr_vector")))
void (* const g_pfnVectors[])(void) = {
    /* Stack pointer */
    (void (*)(void))((uint32_t)&_estack),
    /* Cortex-M7 core exceptions */
    Reset_Handler,
    NMI_Handler,
    HardFault_Handler,
    MemManage_Handler,
    BusFault_Handler,
    UsageFault_Handler,
    0, 0, 0, 0,    /* Reserved */
    SVC_Handler,
    DebugMon_Handler,
    0,               /* Reserved */
    PendSV_Handler,
    SysTick_Handler,
    /* STM32H7 external interrupts */
    0, 0, 0, 0, 0, 0, 0, 0,  /* WWDG → PVD */
    0, 0, 0, 0, 0, 0, 0, 0,  /* TAMPER → SPI1 */
    0, 0, 0, 0, 0, 0, 0, 0,  /* SPI2 → USART1 */
    0, 0, 0, 0, 0, 0, 0, 0,  /* USART2 → USART3/4 */
    0, 0, 0, 0, 0, 0, 0, 0,  /* TIM1 → TIM2 */
    0, 0, 0, 0, 0, 0, 0, 0,  /* TIM3 → I2C1 */
    0, 0, 0, 0, 0, 0, 0, 0,  /* I2C2 → SPI3 */
    0, 0, 0, 0, 0, 0, 0, 0,  /* TIM6 → TIM7 */
    dma1_stream0_irqhandler,   /* DMA1 Stream0 (SPI1 RX) */
    0, 0, 0, 0, 0, 0, 0, 0,  /* DMA1 Stream1-8 */
    0, 0, 0, 0, 0, 0, 0, 0,  /* FMC → SDMMC1 */
    0, 0, 0, 0, 0, 0, 0, 0,  /* TIM8 → TIM12 */
    0, 0, 0, 0, 0, 0, 0, 0,  /* TIM13 → TIM14 */
    0, 0, 0, 0, 0, 0, 0, 0,  /* DMA2 → DMA2D */
    0, 0, 0, 0, 0,             /* ETH → Reserved */
    0, 0, 0, 0, 0, 0, 0,       /* Reserved */
    0, 0, 0, 0, 0, 0, 0, 0,  /* Reserved */
    0, 0, 0, 0, 0, 0, 0, 0,  /* FPU → UART8 */
    lte_uart8_irqhandler,      /* UART8 global interrupt */
    0, 0, 0, 0, 0, 0, 0, 0,  /* SPI6 → SAI2 */
    0, 0, 0, 0, 0, 0,          /* Reserved */
};