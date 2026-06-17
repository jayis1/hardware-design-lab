/**
 * @file    main.c
 * @brief   CarbonFlux soil CO₂ flux monitor — main state machine and scheduler.
 * @author  jayis1
 * @copyright Copyright © 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 *
 * CarbonFlux measures soil CO₂ efflux using the closed dynamic chamber method.
 * This file implements the main state machine, 1 kHz SysTick scheduler,
 * initialization sequence, USB CDC command interface, and power management.
 *
 * Target: STM32U5A9ZG (Cortex-M33 @ 160 MHz)
 * Build:  arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "board.h"
#include "registers.h"
#include "drivers/scd41.h"
#include "drivers/dps310.h"
#include "drivers/ds18b20.h"
#include "drivers/sx1262.h"
#include "drivers/w25q128.h"
#include "drivers/rv8803.h"
#include "drivers/servo.h"
#include "drivers/adc.h"
#include "drivers/flux_engine.h"
#include "drivers/power.h"

/* ======================================================================== */
/*  FORWARD DECLARATIONS                                                    */
/* ======================================================================== */

static void system_init(void);
static void gpio_init(void);
static void clock_init(void);
static void iwdg_init(void);
static void usart_init(void);
static void i2c_init(void);
static void spi1_init(void);
static void spi2_init(void);
static void tim2_init(void);
static void adc_init(void);
static void sensor_init(void);
static void flash_init(void);
static void config_load(void);
static void usb_cdc_init(void);
static void scheduler_init(void);

static void state_machine_run(void);
static void state_equilibrate(void);
static void state_close_lid(void);
static void state_accumulate(void);
static void state_compute_flux(void);
static void state_log_data(void);
static void state_tx_lora(void);
static void state_open_lid(void);
static void state_sleep(void);
static void state_error(flux_state_t src, int err_code);

static void enter_stop2_mode(void);
static void process_usb_cmd(void);
static void process_ble_cmd(void);
static void led_set(uint8_t r, uint8_t g, uint8_t b);
static void led_blink_error(int err_code);
static void hardfault_handler_c(void);

/* ======================================================================== */
/*  GLOBAL STATE                                                            */
/* ======================================================================== */

/** Main system state machine. */
static volatile flux_state_t g_state = STATE_POWER_UP;

/** Last error code (0 = no error). */
static volatile int g_last_error = 0;

/** Error source state. */
static volatile flux_state_t g_error_src = STATE_POWER_UP;

/** System tick counter (1 kHz). */
static volatile uint32_t g_sys_tick_ms = 0;

/** Accumulation timer (milliseconds). */
static volatile uint32_t g_accum_timer_ms = 0;

/** Equilibration timer (milliseconds). */
static volatile uint32_t g_equil_timer_ms = 0;

/** Servo move timer (milliseconds). */
static volatile uint32_t g_servo_timer_ms = 0;

/** LoRaWAN join retry counter. */
static volatile uint8_t g_join_retries = 0;

/** Measurement interval (seconds). */
static volatile uint32_t g_measure_interval_s = DEFAULT_INTERVAL_S;

/** Accumulation duration (seconds). */
static volatile uint32_t g_accum_duration_s = ACCUM_DEFAULT_S;

/** Burst mode flag. */
static volatile bool g_burst_mode = false;

/** Burst mode remaining samples. */
static volatile uint32_t g_burst_remaining = 0;

/** Configuration DIP switch value. */
static volatile uint8_t g_dip_switch = 0;

/** Last flux measurement result. */
static flux_result_t g_last_flux;

/** Last raw sensor readings for telemetry. */
static struct {
    float co2_ppm;
    float pressure_hpa;
    float air_temp_c;
    float soil_temp_5cm;
    float soil_temp_15cm;
    float soil_temp_30cm;
    float vwc_pct;
    float par_umol;
    float battery_v;
    uint8_t battery_soc;
} g_sensor_data;

/** USB CDC receive buffer. */
static char g_usb_rx_buf[USB_CDC_RX_BUF_SIZE];
static volatile uint8_t g_usb_rx_idx = 0;

/** Measurement counter — increments each completed cycle. */
static volatile uint32_t g_measurement_count = 0;

/* ======================================================================== */
/*  SYSTICK HANDLER (1 kHz)                                                 */
/* ======================================================================== */

void SysTick_Handler(void) {
    g_sys_tick_ms++;

    /* Update accumulation timer if active */
    if (g_state == STATE_ACCUMULATE) {
        g_accum_timer_ms++;
    }

    /* Update equilibration timer if active */
    if (g_state == STATE_EQUILIBRATE) {
        g_equil_timer_ms++;
    }

    /* Update servo move timer if active */
    if (g_state == STATE_CLOSE_LID || g_state == STATE_OPEN_LID) {
        g_servo_timer_ms++;
    }

    /* Reload IWDG every 10 ms */
    if ((g_sys_tick_ms % 10) == 0) {
        IWDG->KR = IWDG_KEY_RELOAD;
    }

    /* Blink green LED heartbeat (500 ms on/off) */
    if ((g_sys_tick_ms % 500) < 250) {
        GPIO_SET_PIN(GPIOA, 10); /* LED green on */
    } else {
        GPIO_RESET_PIN(GPIOA, 10);
    }
}

/* ======================================================================== */
/*  SYSTEM INITIALIZATION                                                   */
/* ======================================================================== */

/**
 * @brief System clock initialization.
 *
 * Configures PLL from HSI16 to achieve:
 *   SYSCLK = 160 MHz
 *   HCLK   = 160 MHz (AHB)
 *   APB1   = 160 MHz
 *   APB2   = 160 MHz
 *   USB    = 48 MHz  (via PLL1Q output)
 *   RTC    = 32.768 kHz (LSE)
 */
static void clock_init(void) {
    /* Enable HSI16 */
    RCC->CR |= RCC_CR_HSI16ON;
    while (!(RCC->CR & RCC_CR_HSI16RDYF));

    /* Configure flash wait states for 160 MHz */
    /* STM32U5: 4 wait states at 160 MHz with VOS1 */
    FLASH->ACR = (4 << 0) | FLASH_ACR_PRFTEN;

    /* Configure PLL1: HSI16 → /M=1 → *N=20 → /P=2 → 160 MHz */
    RCC->PLL1CFGR = (0 << 0)                  /* Source: HSI16           */
                  | RCC_PLL1CFGR_PLL1P_EN     /* Enable P output         */
                  | RCC_PLL1CFGR_PLL1Q_EN     /* Enable Q output (USB)   */
                  | RCC_PLL1CFGR_PLL1R_EN;    /* Enable R output (unused)*/

    /* PLL1 dividers: N=20, M=1, P=2, Q=4 (48 MHz for USB) */
    RCC->PLL1DIVR = ((20 - 1) << 8)           /* N divider               */
                  | ((1 - 1)  << 0)           /* M divider               */
                  | ((2 - 1)  << 24)          /* P divider               */
                  | ((4 - 1)  << 16);         /* Q divider               */

    /* Enable PLL1 and wait for lock */
    RCC->CR |= RCC_CR_PLL1ON;
    while (!(RCC->CR & RCC_CR_PLL1RDYF));

    /* Switch system clock to PLL1 */
    RCC->CFGR = (3 << 0); /* SW = PLL1 (0=HSI16, 1=HSE, 2=MSI, 3=PLL1) */

    /* Update Core Clock global variable for HAL compatibility */
    SystemCoreClock = SYSCLK_FREQ_HZ;

    /* Enable LSE for RTC (if not already running) */
    /* LSE is enabled in the Backup Domain Control Register (BDCR) */
    RCC->BDCR |= (1UL << 0);  /* LSEON */
    for (volatile int i = 0; i < 100000; i++); /* Wait for startup */
}

/**
 * @brief GPIO initialization.
 *
 * All unused pins are set to ANALOG mode at reset for lowest power.
 * Only actively used pins are configured here.
 */
static void gpio_init(void) {
    /* Enable GPIO clocks */
    RCC->AHB1ENR |= (1U << 0)  /* GPIOA */
                  | (1U << 1)  /* GPIOB */
                  | (1U << 2); /* GPIOC */

    /* === SPI1 (LoRa) — PB3(PB3=SCK), PB4(MISO), PB5(MOSI), PB0(NSS) === */
    GPIOB->MODER &= ~(0x3F0F0UL); /* Clear bits for PB0,PB3,PB4,PB5 */
    GPIOB->MODER |=  (0x280A0UL); /* PB0=output, PB3=AF, PB4=AF, PB5=AF */
    GPIOB->AFRL   = (GPIOB->AFRL & ~0xF0FF0000UL) | (0x55050000UL);

    /* === I2C1 — PB6(SCL), PB7(SDA) === */
    GPIOB->MODER &= ~(0xF0000000UL);
    GPIOB->MODER |=  (0xA0000000UL); /* AF for both */
    GPIOB->AFRL   = (GPIOB->AFRL & ~0xFF000000UL) | (0x44000000UL); /* AF4 */
    GPIOB->OTYPER |= (3UL << 6);  /* Open-drain */
    GPIOB->PUPDR  |= (0x50000UL);  /* Pull-up (10kΩ internal) */

    /* === Sensor enables (PB8, PB9, PA0) — output, drive HIGH === */
    GPIOB->MODER &= ~(0x0F000000UL);
    GPIOB->MODER |=  (0x05000000UL); /* PB8, PB9 output */
    GPIOA->MODER &= ~(0x0000000FUL);
    GPIOA->MODER |=  (0x00000005UL); /* PA0 output */

    /* Start with all sensors disabled */
    GPIO_RESET_PIN(GPIOB, 8); /* EN_SCD41 = LOW */
    GPIO_RESET_PIN(GPIOB, 9); /* EN_DPS310 = LOW */
    GPIO_RESET_PIN(GPIOA, 0); /* EN_TMP117 = LOW */

    /* === Status LEDs === */
    GPIOA->MODER &= ~(0x003C0000UL);  /* PA9(red), PA10(green) */
    GPIOA->MODER |=  (0x00140000UL);  /* Output */
    GPIOC->MODER &= ~(0x00030000UL);  /* PC8(blue) */
    GPIOC->MODER |=  (0x00010000UL);  /* Output */

    /* === USB D+/D− === */
    /* Leave PA11, PA12 at reset default (analog) — USB IP controls these */

    /* === ADC inputs (PC0, PC1, PC2) — analog mode === */
    GPIOC->MODER &= ~(0x0000003FUL);
    GPIOC->MODER |=  (0x0000003FUL); /* All analog */

    /* === Servo PWM (PC4) — AF mode === */
    GPIOC->MODER &= ~(0x00000F00UL);
    GPIOC->MODER |=  (0x00000A00UL); /* AF */
    GPIOC->AFRL   = (GPIOC->AFRL & ~0x0000F000UL) | (0x00001000UL); /* AF1=TIM2 */

    /* === 1-Wire (PC6) — open-drain output === */
    GPIOC->MODER &= ~(0x00003000UL);
    GPIOC->MODER |=  (0x00001000UL); /* Output */
    GPIOC->OTYPER |= (1UL << 6);     /* Open-drain */
    GPIOC->PUPDR  |= (1UL << 12);    /* Pull-up */

    /* === USART2 (PA2=TX, PA3=RX) — AF7 === */
    GPIOA->MODER &= ~(0x000000F0UL);
    GPIOA->MODER |=  (0x000000A0UL); /* AF */
    GPIOA->AFRL   = (GPIOA->AFRL & ~0xFF00UL) | (0x7700UL); /* AF7 */
}

/**
 * @brief Enable peripheral clocks.
 */
static void periph_clk_enable(void) {
    RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN
                   | RCC_APB1ENR1_USART2EN
                   | RCC_APB1ENR1_I2C1EN
                   | RCC_APB1ENR1_SPI2EN;
    RCC->APB2ENR  |= RCC_APB2ENR_SPI1EN
                   | RCC_APB2ENR_ADC1EN;
    RCC->AHB1ENR  |= RCC_AHB1ENR_CRCEN;
}

/**
 * @brief IWDG initialization.
 * 25.6 second timeout at LSI ~32 kHz, prescaler 256.
 */
static void iwdg_init(void) {
    IWDG->KR = IWDG_KEY_UNLOCK;
    IWDG->PR = 5;      /* Prescaler = 256 */
    IWDG->RLR = 3200;  /* ~25.6 s at 32 kHz / 256 */
    IWDG->KR = IWDG_KEY_START;
    IWDG->KR = IWDG_KEY_RELOAD;
}

/**
 * @brief USART2 initialization for USB CDC VCP.
 */
static void usart_init(void) {
    USART2->CR1 = 0;              /* Disable during config */
    USART2->BRR = SYSCLK_FREQ_HZ / 115200;  /* 115200 baud */
    USART2->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

/**
 * @brief I2C1 initialization at 400 kHz.
 */
static void i2c_init(void) {
    /* Reset I2C1 */
    RCC->APB1RSTR1 |= (1U << 21);
    RCC->APB1RSTR1 &= ~(1U << 21);

    /* Timing configuration for 400 kHz at 160 MHz APB1 */
    /* Based on STM32U5 I2C timing calculation: */
    /* tSCL = (PRESC+1) * (SCLL+1) * tAPB + (SCLH+1) * tAPB */
    I2C1->TIMINGR = 0x00201D2B;  /* ~400 kHz with 160 MHz APB */
    I2C1->CR1 = I2C_CR1_PE;     /* Enable peripheral */
}

/**
 * @brief SPI1 initialization for SX1262 (8 MHz, mode 0).
 */
static void spi1_init(void) {
    RCC->APB1RSTR1 |= (1U << 25);  /* Reset SPI1 */
    RCC->APB1RSTR1 &= ~(1U << 25);

    SPI1->CFG1 = (4 << 28)          /* MBR = 4 (divide by 16: 160/16=10 MHz) */
               | (7 << 0);          /* DSIZE = 8-bit */
    SPI1->CFG2 = SPI_CFG2_SSM       /* Software slave management */
               | SPI_CFG2_SSIOP;    /* SS output enable */
    SPI1->CR2 = 1;                  /* TSIZE = 1 byte (auto) */
    SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_SPE;  /* Master, enable */
}

/**
 * @brief SPI2 initialization for W25Q128 (40 MHz, mode 0).
 */
static void spi2_init(void) {
    RCC->APB1RSTR1 |= (1U << 28);  /* Reset SPI2 */
    RCC->APB1RSTR1 &= ~(1U << 28);

    SPI2->CFG1 = (0 << 28)          /* MBR = 0 (divide by 2: 160/2=80—use /4) */
               | (2 << 28)          /* MBR = 2 (divide by 4: 40 MHz) */
               | (7 << 0);          /* DSIZE = 8-bit */
    SPI2->CFG2 = SPI_CFG2_SSM
               | SPI_CFG2_SSIOP;
    SPI2->CR2 = 1;
    SPI2->CR1 = SPI_CR1_MSTR | SPI_CR1_SPE;
}

/**
 * @brief TIM2 initialization for servo PWM (50 Hz).
 */
static void tim2_init(void) {
    /* TIM2 clock = APB1 = 160 MHz */
    /* Prescaler: 160 MHz / 1600 = 100 kHz timer clock */
    /* Auto-reload: 100 kHz / 2000 = 50 Hz PWM */
    TIM2->PSC = 1600 - 1;   /* Prescaler for 100 kHz */
    TIM2->ARR = 2000 - 1;   /* Auto-reload for 50 Hz (period = 20 ms) */
    TIM2->CCMR1 = (6 << 4); /* OC1M = PWM mode 1 */
    TIM2->CCER |= TIM_CCER_CC1E;  /* CC1 output enable */
    TIM2->CCR1 = 1000;      /* Initial: closed position (1 ms pulse) */
    TIM2->CR1 |= TIM_CR1_ARPE;  /* Auto-reload preload */
    TIM2->EGR |= 1;         /* Generate update event */
    TIM2->CR1 |= TIM_CR1_CEN;   /* Start counter */
}

/**
 * @brief ADC initialization (12-bit, 3 channels, continuous scan).
 */
static void adc_init(void) {
    /* ADC configuration: 12-bit, single-ended, continuous conversion mode */
    ADC1->CR = (1U << 28);             /* ADVREGEN */
    for (volatile int i = 0; i < 10000; i++); /* Wait for regulator */

    ADC1->CFGR = (1U << 5)             /* CONT = continuous */
               | (6 << 16);            /* OVFS = 64 oversampling (6 bits) */
    ADC1->CFGR2 = 0;                   /* Default: single-ended */

    /* Regular sequence: 3 channels (IN0=PAR, IN1=Vbat, IN2=Spare) */
    ADC1->SQR1 = (3 << 0);             /* Sequence length = 3 */
    ADC1->SQR3 = 0x00020100;           /* SQ1=IN0, SQ2=IN1, SQ3=IN2 */

    /* Sampling time: 640.5 cycles (~4 µs @ 160 MHz) */
    ADC1->SMPR1 = 0x0000B6DB;          /* SMP=7 for all (640.5 cycles) */

    /* Calibrate */
    ADC1->CR |= (1U << 31);            /* ADCAL */
    while (ADC1->CR & (1U << 31));

    /* Enable */
    ADC1->ISR |= (1U << 0);            /* Clear ADRDY */
    ADC1->CR |= (1U << 0);             /* ADEN */
    while (!(ADC1->ISR & (1U << 0)));  /* Wait for ADRDY */
}

/* ======================================================================== */
/*  SENSOR INITIALIZATION                                                   */
/* ======================================================================== */

static void sensor_init(void) {
    /* Enable sensor power one at a time */
    GPIO_SET_PIN(GPIOB, 8);  /* EN_SCD41 */
    delay_ms(10);
    scd41_init();

    GPIO_SET_PIN(GPIOB, 9);  /* EN_DPS310 */
    delay_ms(10);
    dps310_init();

    GPIO_SET_PIN(GPIOA, 0);  /* EN_TMP117 */
    delay_ms(10);
    tmp117_init();

    /* RTC (RV-8803) always powered; no enable pin needed */
    rv8803_init();

    /* 1-Wire DS18B20 sensors */
    ds18b20_init();

    /* LoRa module */
    sx1262_init();
}

static void flash_init(void) {
    w25q128_init();
    flash_log_init();
}

static void config_load(void) {
    /* Read configuration from flash (FLASH_CONFIG region) */
    uint32_t crc_stored, crc_calc;
    uint8_t config_buf[256];

    /* Read first 256 bytes of config sector */
    flash_read_config(config_buf, 256);

    /* Validate CRC32 */
    crc_stored = *(uint32_t *)&config_buf[252];
    crc_calc = crc32_hw(config_buf, 252);
    if (crc_calc != crc_stored) {
        g_last_error = ERR_CONFIG_CRC;
        /* Fall back to defaults */
        return;
    }

    /* Parse measurement interval (stored at offset 0) */
    g_measure_interval_s = *(uint32_t *)&config_buf[0];
    if (g_measure_interval_s < MIN_INTERVAL_S)
        g_measure_interval_s = MIN_INTERVAL_S;
    if (g_measure_interval_s > MAX_INTERVAL_S)
        g_measure_interval_s = MAX_INTERVAL_S;

    /* Parse accumulation duration (stored at offset 4) */
    g_accum_duration_s = *(uint32_t *)&config_buf[4];
    if (g_accum_duration_s < ACCUM_MIN_DURATION_S)
        g_accum_duration_s = ACCUM_MIN_DURATION_S;
    if (g_accum_duration_s > ACCUM_MAX_DURATION_S)
        g_accum_duration_s = ACCUM_MAX_DURATION_S;

    /* LoRaWAN keys and calibration data follow... */
    /* (Implementation omitted for brevity — same pattern) */
}

/* ======================================================================== */
/*  UTILITY FUNCTIONS                                                       */
/* ======================================================================== */

static void delay_ms(volatile uint32_t ms) {
    uint32_t start = g_sys_tick_ms;
    while ((g_sys_tick_ms - start) < ms) {
        __asm volatile("wfi"); /* Wait for interrupt (saves power) */
    }
}

static uint32_t crc32_hw(const uint8_t *data, uint32_t len) {
    CRC->CR = CRC_CR_RESET;              /* Reset CRC unit */
    CRC->INIT = 0xFFFFFFFF;              /* Standard CRC-32 init */
    CRC->POL = 0x04C11DB7;               /* Standard polynomial */
    CRC->CR = (3 << 5) | (1 << 7);       /* REV_IN = byte, REV_OUT = 1 */

    for (uint32_t i = 0; i < len; i++) {
        *(volatile uint8_t *)&CRC->DR = data[i];
    }
    return CRC->DR ^ 0xFFFFFFFF;
}

/* ======================================================================== */
/*  MAIN — ENTRY POINT                                                     */
/* ======================================================================== */

int main(void) {
    /* === Hard fault handler registration === */
    /* SCB->CCR |= SCB_CCR_DIV_0_TRP; */  /* Trap division by zero */

    /* === Core initialization === */
    system_init();

    /* === Print startup banner === */
    printf("\r\n══════════════════════════════════════════\r\n");
    printf("  CarbonFlux Soil CO₂ Flux Monitor v1.0\r\n");
    printf("  Author: jayis1\r\n");
    printf("  Compiled: " __DATE__ " at " __TIME__ "\r\n");
    printf("══════════════════════════════════════════\r\n");
    printf("State: INIT done — entering main loop\r\n");

    /* === Main state machine loop === */
    while (1) {
        state_machine_run();

        /* Process incoming USB CDC commands (non-blocking) */
        process_usb_cmd();

        /* Feed watchdog */
        IWDG->KR = IWDG_KEY_RELOAD;
    }
}

static void system_init(void) {
    /* Clear BSS section (initialized by startup code) */
    /* SCB->VTOR = 0x08000000; */  /* Vector table offset (bootloader) */

    clock_init();
    periph_clk_enable();
    gpio_init();
    iwdg_init();
    usart_init();
    i2c_init();
    spi1_init();
    spi2_init();
    tim2_init();
    adc_init();

    /* Driver-level initialization */
    sensor_init();
    flash_init();
    config_load();

    /* Read DIP switch */
    g_dip_switch = GPIO_READ_PIN(GPIOA, 15) & 1;

    scheduler_init();
}

/* ======================================================================== */
/*  SCHEDULER                                                               */
/* ======================================================================== */

static void scheduler_init(void) {
    /* Set SysTick to 1 ms */
    SysTick_Config(SYSCLK_FREQ_HZ / SYSTICK_FREQ_HZ);
}

/* ======================================================================== */
/*  STATE MACHINE                                                           */
/* ======================================================================== */

static void state_machine_run(void) {
    switch (g_state) {

    case STATE_POWER_UP:
        /* Wait for power rail stabilization */
        delay_ms(100);
        g_state = STATE_INIT;
        break;

    case STATE_INIT:
        /* Already handled by system_init() */
        g_state = STATE_EQUILIBRATE;
        break;

    case STATE_EQUILIBRATE:
        state_equilibrate();
        break;

    case STATE_CLOSE_LID:
        state_close_lid();
        break;

    case STATE_ACCUMULATE:
        state_accumulate();
        break;

    case STATE_COMPUTE_FLUX:
        state_compute_flux();
        break;

    case STATE_LOG_DATA:
        state_log_data();
        break;

    case STATE_TX_LORA:
        state_tx_lora();
        break;

    case STATE_OPEN_LID:
        state_open_lid();
        break;

    case STATE_SLEEP:
        state_sleep();
        break;

    case STATE_ERROR:
        /* Error stuck state — system halts with blink pattern */
        led_blink_error(g_last_error);
        break;

    default:
        g_state = STATE_ERROR;
        g_last_error = ERR_TIMEOUT;
        break;
    }
}

/* ======================================================================== */
/*  STATE: EQUILIBRATE                                                      */
/* ======================================================================== */

static void state_equilibrate(void) {
    /* Lid should already be open. Wait for equilibration period. */
    g_equil_timer_ms = 0;

    /* Print state */
    printf("STATE: EQUILIBRATE (lid open, %d seconds)\r\n", EQUILIBRATION_S);

    /* Wait for equilibration timer to expire */
    while (g_equil_timer_ms < (uint32_t)(EQUILIBRATION_S * 1000)) {
        process_usb_cmd();               /* Keep accepting commands */
        IWDG->KR = IWDG_KEY_RELOAD;      /* Feed watchdog */
        __asm volatile("wfi");           /* Sleep until next SysTick */
    }

    g_state = STATE_CLOSE_LID;
}

/* ======================================================================== */
/*  STATE: CLOSE LID                                                        */
/* ======================================================================== */

static void state_close_lid(void) {
    printf("STATE: CLOSE_LID\r\n");

    /* Enable servo power */
    GPIO_SET_PIN(GPIOC, 3); /* EN_5V = HIGH */

    /* Move servo to closed position (0°) — 1 ms pulse */
    servo_set_position(SERVO_PULSE_CLOSED_US);
    g_servo_timer_ms = 0;

    /* Wait for servo to move */
    while (g_servo_timer_ms < SERVO_MOVE_TIME_MS) {
        process_usb_cmd();
        IWDG->KR = IWDG_KEY_RELOAD;

        /* Check stall detection */
        if (GPIO_READ_PIN(GPIOC, 5) == 0) {
            state_error(STATE_CLOSE_LID, ERR_SERVO_STALL);
            return;
        }
        __asm volatile("wfi");
    }

    /* Reset accumulation timer */
    g_accum_timer_ms = 0;

    /* Start accumulation */
    g_state = STATE_ACCUMULATE;
}

/* ======================================================================== */
/*  STATE: ACCUMULATE                                                       */
/* ======================================================================== */

static void state_accumulate(void) {
    printf("STATE: ACCUMULATE (%lu seconds)\r\n", g_accum_duration_s);
    g_measurement_count++;

    /* Reset flux engine */
    flux_engine_reset();

    /* Sample interval counter */
    uint32_t last_sample_ms = 0;

    while (g_accum_timer_ms < (g_accum_duration_s * 1000)) {
        process_usb_cmd();
        IWDG->KR = IWDG_KEY_RELOAD;

        /* Check if it's time to sample CO₂ */
        if ((g_accum_timer_ms - last_sample_ms) >= (CO2_SAMPLE_INTERVAL_S * 1000)) {
            last_sample_ms = g_accum_timer_ms;

            /* Read CO₂ sensor */
            uint16_t co2_raw;
            int ret = scd41_read_co2(&co2_raw);

            if (ret == 0) {
                /* Read barometric pressure and air temperature for correction */
                float pressure = dps310_read_pressure_hpa();
                float temp_c = tmp117_read_temp_c();

                /* Feed to flux engine */
                flux_engine_add_sample(
                    (float)(g_accum_timer_ms / 1000),  /* time (seconds) */
                    (float)co2_raw,                      /* CO₂ (ppm) */
                    pressure,
                    temp_c
                );
            } else {
                printf("WARN: SCD41 read error %d\r\n", ret);
            }
        }

        __asm volatile("wfi");
    }

    g_state = STATE_COMPUTE_FLUX;
}

/* ======================================================================== */
/*  STATE: COMPUTE FLUX                                                     */
/* ======================================================================== */

static void state_compute_flux(void) {
    printf("STATE: COMPUTE_FLUX\r\n");

    /* Read ancillary sensors */
    g_sensor_data.pressure_hpa = dps310_read_pressure_hpa();
    g_sensor_data.air_temp_c = tmp117_read_temp_c();
    g_sensor_data.soil_temp_5cm = ds18b20_read_temp(0);
    g_sensor_data.soil_temp_15cm = ds18b20_read_temp(1);
    g_sensor_data.soil_temp_30cm = ds18b20_read_temp(2);
    g_sensor_data.vwc_pct = adc_read_par() * 0.01f; /* Placeholder for SDI-12 */
    g_sensor_data.par_umol = adc_read_par();
    g_sensor_data.battery_v = adc_read_vbat();
    g_sensor_data.battery_soc = batt_mv_to_pct(g_sensor_data.battery_v * 1000.0f);

    /* Run flux computation */
    flux_engine_compute(&g_last_flux);

    g_sensor_data.co2_ppm = g_last_flux.intercept + 400.0f; /* Baseline-adjusted */

    printf("  Flux result:\r\n");
    printf("    Samples: %lu\r\n", g_last_flux.sample_count);
    printf("    dC/dt:   %.6f ppm/s\r\n", g_last_flux.slope);
    printf("    R²:      %.4f\r\n", g_last_flux.r_squared);
    printf("    Flux:    %.4f µmol·m⁻²·s⁻¹\r\n", g_last_flux.flux_umol);

    /* Validate flux quality */
    if (g_last_flux.r_squared < R2_MIN_THRESHOLD) {
        printf("  WARN: R² below threshold (%.4f < %.2f)\r\n",
               g_last_flux.r_squared, R2_MIN_THRESHOLD);
        g_last_flux.flux_umol = 0.0f; /* Mark invalid */
        g_last_error = ERR_FLUX_R2_LOW;
    }
    if (g_last_flux.sample_count < MIN_VALID_SAMPLES) {
        printf("  WARN: Too few samples (%lu < %d)\r\n",
               g_last_flux.sample_count, MIN_VALID_SAMPLES);
        g_last_error = ERR_FLUX_R2_LOW;
    }

    g_state = STATE_LOG_DATA;
}

/* ======================================================================== */
/*  STATE: LOG DATA                                                         */
/* ======================================================================== */

static void state_log_data(void) {
    printf("STATE: LOG_DATA\r\n");

    /* Build binary record */
    flux_record_t record;
    record.timestamp = rv8803_read_unix();
    record.co2_ppm_x1000 = (int32_t)(g_sensor_data.co2_ppm * 1000.0f);
    record.flux_umol_x1000 = (int32_t)(g_last_flux.flux_umol * 1000.0f);
    record.soil_temp_5cm_x100 = (int16_t)(g_sensor_data.soil_temp_5cm * 100.0f);
    record.soil_temp_15cm_x100 = (int16_t)(g_sensor_data.soil_temp_15cm * 100.0f);
    record.soil_temp_30cm_x100 = (int16_t)(g_sensor_data.soil_temp_30cm * 100.0f);
    record.vwc_x100 = (uint16_t)(g_sensor_data.vwc_pct * 100.0f);
    record.pressure_x10 = (uint16_t)(g_sensor_data.pressure_hpa * 10.0f);
    record.air_temp_c_x100 = (int16_t)(g_sensor_data.air_temp_c * 100.0f);
    record.par_umol = (uint16_t)(g_sensor_data.par_umol);
    record.battery_soc = g_sensor_data.battery_soc;
    record.flags = 0;

    if (g_last_error != 0) {
        record.flags |= FLUX_FLAG_ERROR;
    }

    /* Write to flash ring buffer */
    int ret = flash_log_write(&record);
    if (ret < 0) {
        printf("  ERROR: Flash write failed (%d)\r\n", ret);
        state_error(STATE_LOG_DATA, ERR_FLASH_WRITE_FAIL);
        return;
    }

    printf("  Record written @ offset 0x%08lX\r\n",
           (uint32_t)(flash_log_get_head() * sizeof(flux_record_t)));

    g_state = STATE_TX_LORA;
}

/* ======================================================================== */
/*  STATE: TX LoRa                                                          */
/* ======================================================================== */

static void state_tx_lora(void) {
    printf("STATE: TX_LORA\r\n");

    /* Build LoRaWAN payload (port 1: flux measurement, 32 bytes) */
    uint8_t payload[32];
    memset(payload, 0, sizeof(payload));

    *(uint32_t *)&payload[0]  = rv8803_read_unix();
    *(float *)&payload[4]     = g_last_flux.flux_umol;
    *(float *)&payload[8]     = g_sensor_data.co2_ppm;
    *(float *)&payload[12]    = g_sensor_data.air_temp_c;
    payload[16] = g_sensor_data.battery_soc;
    payload[17] = (uint8_t)(g_sensor_data.vwc_pct);
    payload[18] = g_last_error != 0 ? 0x01 : 0x00;
    payload[19] = g_measurement_count & 0xFF;

    /* Attempt LoRaWAN confirmed uplink */
    int ret = sx1262_lora_send(payload, 20, 1, true); /* FPort=1, confirmed */

    if (ret < 0) {
        printf("  WARN: LoRa TX failed (%d), saving for next cycle\r\n", ret);
        /* Don't error — log locally and retry next cycle */
    } else {
        printf("  LoRa TX OK (bytes: 20)\r\n");
        /* Blink blue LED briefly */
        GPIO_SET_PIN(GPIOC, 8);
        delay_ms(50);
        GPIO_RESET_PIN(GPIOC, 8);
    }

    g_state = STATE_OPEN_LID;
}

/* ======================================================================== */
/*  STATE: OPEN LID                                                         */
/* ======================================================================== */

static void state_open_lid(void) {
    printf("STATE: OPEN_LID\r\n");

    /* Move servo to open position (180°) — 2 ms pulse */
    servo_set_position(SERVO_PULSE_OPEN_US);
    g_servo_timer_ms = 0;

    while (g_servo_timer_ms < SERVO_MOVE_TIME_MS) {
        process_usb_cmd();
        IWDG->KR = IWDG_KEY_RELOAD;
        __asm volatile("wfi");
    }

    /* Disable servo power to save energy */
    GPIO_RESET_PIN(GPIOC, 3); /* EN_5V = LOW */

    g_state = STATE_SLEEP;
}

/* ======================================================================== */
/*  STATE: SLEEP (Deep Sleep — Stop 2 Mode)                                 */
/* ======================================================================== */

static void state_sleep(void) {
    printf("STATE: SLEEP (%lu seconds)\r\n", g_measure_interval_s);
    printf("Entering Stop 2 mode...\r\n");

    /* Flush USB CDC TX before sleeping */
    delay_ms(5);

    /* Set RTC alarm for next measurement */
    rv8803_set_alarm_s(g_measure_interval_s);

    /* Enter Stop 2 mode */
    enter_stop2_mode();

    /* We wake here after RTC alarm or external interrupt */
    printf("Wake from Stop 2\r\n");

    g_state = STATE_EQUILIBRATE;
}

/* ======================================================================== */
/*  POWER MANAGEMENT                                                        */
/* ======================================================================== */

/**
 * @brief Enter STM32U5 Stop 2 mode (~2.5 µA typical).
 *
 * All SRAM is retained. Wake sources:
 *   - RTC alarm (RV-8803 NINT → PA4 EXTI)
 *   - USB connect/disconnect
 *   - DIP switch change
 */
static void enter_stop2_mode(void) {
    /* Disable peripherals that should not draw current in sleep */
    /* Sensors already powered off via enable pins */

    /* Configure EXTI for RTC interrupt (PA4) */
    SYSCFG->EXTICR[0] = (SYSCFG->EXTICR[0] & ~0xF000UL) | 0x0000UL; /* EXTI4 = PA */
    EXTI->IMR1 |= (1U << 4);    /* Interrupt mask on line 4 */
    EXTI->RTSR1 |= (1U << 4);   /* Rising edge trigger */
    EXTI->FTSR1 &= ~(1U << 4);

    /* Clear pending interrupt */
    EXTI->PR1 = (1U << 4);

    /* Configure Power Control for Stop 2 */
    PWR->CR1 |= PWR_CR1_FLPS;     /* Flash low-power mode */
    PWR->CR2 |= PWR_CR2_STOP2;    /* Stop 2 entry request */

    /* Ensure all GPIOs are in safe low-power state */
    /* (Already handled in gpio_init — unused pins in analog) */

    /* Disable SysTick before sleep (re-enabled on wake via RTC) */
    SysTick->CTRL &= ~1;

    /* Wait for interrupt — enters Stop 2 */
    __asm volatile("dsb");
    __asm volatile("wfi");
    __asm volatile("isb");

    /* --- Wake from interrupt --- */
    /* Re-enable SysTick */
    SysTick->CTRL |= 1;

    /* Restore system clock (PLL may have been disabled) */
    /* In Stop 2, PLL is disabled; HSII6 remains on. Re-lock PLL: */
    RCC->CR |= RCC_CR_PLL1ON;
    while (!(RCC->CR & RCC_CR_PLL1RDYF));
    RCC->CFGR = (3 << 0); /* Re-select PLL1 */

    /* Disable EXTI */
    EXTI->IMR1 &= ~(1U << 4);
    EXTI->PR1 = (1U << 4);

    /* Re-initialize sensors (they were power-cycled) */
    GPIO_SET_PIN(GPIOB, 8);   /* SCD41 on */
    GPIO_SET_PIN(GPIOB, 9);   /* DPS310 on */
    GPIO_SET_PIN(GPIOA, 0);   /* TMP117 on */

    /* Quick re-init (skip full init — sensors often retain config) */
    scd41_wake();
    dps310_wake();
}

/* ======================================================================== */
/*  USB CDC COMMAND PROCESSING                                              */
/* ======================================================================== */

static void process_usb_cmd(void) {
    /* Read from USART2 (USB CDC VCP) */
    if (USART2->ISR & USART_ISR_RXNE) {
        char c = (char)(USART2->RDR & 0xFF);

        if (c == '\n' || c == '\r') {
            /* Null-terminate and process */
            g_usb_rx_buf[g_usb_rx_idx] = '\0';
            g_usb_rx_idx = 0;

            /* Parse command */
            if (strcmp(g_usb_rx_buf, "STATUS") == 0 || strcmp(g_usb_rx_buf, "status") == 0) {
                char resp[128];
                snprintf(resp, sizeof(resp),
                    "OK %d:%.1f:%.1f:%.4f:%d:%d\r\n",
                    g_state,
                    g_sensor_data.co2_ppm,
                    g_sensor_data.air_temp_c,
                    g_last_flux.flux_umol,
                    g_sensor_data.battery_soc,
                    g_measurement_count);
                usb_cdc_send(resp, strlen(resp));
            }
            else if (strcmp(g_usb_rx_buf, "CONFIG GET") == 0) {
                char resp[64];
                snprintf(resp, sizeof(resp), "OK interval=%lu accum=%lu\r\n",
                         g_measure_interval_s, g_accum_duration_s);
                usb_cdc_send(resp, strlen(resp));
            }
            else if (strncmp(g_usb_rx_buf, "CONFIG SET interval=", 20) == 0) {
                uint32_t val = 0;
                /* Quick parse of digits */
                for (const char *p = g_usb_rx_buf + 20; *p >= '0' && *p <= '9'; p++) {
                    val = val * 10 + (*p - '0');
                }
                if (val >= MIN_INTERVAL_S && val <= MAX_INTERVAL_S) {
                    g_measure_interval_s = val;
                    usb_cdc_send("OK\r\n", 4);
                } else {
                    usb_cdc_send("ERR out of range\r\n", 18);
                }
            }
            else if (strncmp(g_usb_rx_buf, "CONFIG SET accum=", 17) == 0) {
                uint32_t val = 0;
                for (const char *p = g_usb_rx_buf + 17; *p >= '0' && *p <= '9'; p++) {
                    val = val * 10 + (*p - '0');
                }
                if (val >= ACCUM_MIN_DURATION_S && val <= ACCUM_MAX_DURATION_S) {
                    g_accum_duration_s = val;
                    usb_cdc_send("OK\r\n", 4);
                } else {
                    usb_cdc_send("ERR out of range\r\n", 18);
                }
            }
            else if (strcmp(g_usb_rx_buf, "CAL ZERO") == 0) {
                usb_cdc_send("OK calibrating_zero\r\n", 21);
                g_state = STATE_CAL_ZERO;
            }
            else if (strcmp(g_usb_rx_buf, "CAL SPAN") == 0) {
                usb_cdc_send("OK calibrating_span\r\n", 21);
                g_state = STATE_CAL_SPAN;
            }
            else if (strcmp(g_usb_rx_buf, "LOG DUMP") == 0) {
                /* Dump all log entries (max 100 at a time for USB) */
                usb_cdc_send("OK LOG DUMP START\r\n", 19);
                flash_log_dump(100);
                usb_cdc_send("OK LOG DUMP END\r\n", 17);
            }
            else if (strcmp(g_usb_rx_buf, "RESET") == 0) {
                usb_cdc_send("OK resetting\r\n", 14);
                NVIC_SystemReset();
            }
            else if (strcmp(g_usb_rx_buf, "HELP") == 0) {
                usb_cdc_send(
                    "Commands:\r\n"
                    "  STATUS          — Show device state\r\n"
                    "  CONFIG GET      — Show config\r\n"
                    "  CONFIG SET interval=N — Set interval (s)\r\n"
                    "  CONFIG SET accum=N     — Set accumulation (s)\r\n"
                    "  CAL ZERO        — Zero calibration\r\n"
                    "  CAL SPAN        — Span calibration\r\n"
                    "  LOG DUMP        — Dump stored records\r\n"
                    "  RESET           — Soft reset\r\n"
                    "  HELP            — This help\r\n", 305);
            }
            else if (g_usb_rx_idx > 0) {
                usb_cdc_send("ERR unknown command\r\n", 21);
            }
        }
        else if (g_usb_rx_idx < USB_CDC_RX_BUF_SIZE - 1) {
            g_usb_rx_buf[g_usb_rx_idx++] = c;
        }
    }
}

static void usb_cdc_send(const char *data, uint32_t len) {
    for (uint32_t i = 0; i < len; i++) {
        while (!(USART2->ISR & USART_ISR_TXE));
        USART2->TDR = data[i];
    }
}

/* ======================================================================== */
/*  LED CONTROL                                                             */
/* ======================================================================== */

static inline void led_set(uint8_t r, uint8_t g, uint8_t b) {
    if (r) GPIO_SET_PIN(GPIOA, 9); else GPIO_RESET_PIN(GPIOA, 9);
    if (g) GPIO_SET_PIN(GPIOA, 10); else GPIO_RESET_PIN(GPIOA, 10);
    if (b) GPIO_SET_PIN(GPIOC, 8); else GPIO_RESET_PIN(GPIOC, 8);
}

static void led_blink_error(int err_code) {
    /* Blink red LED in pattern: N blinks = error code, pause, repeat */
    int count = (err_code > 0) ? err_code : -err_code;
    if (count > 9) count = 9;

    while (1) {
        for (int i = 0; i < count; i++) {
            led_set(1, 0, 0);
            delay_ms(200);
            led_set(0, 0, 0);
            delay_ms(200);
        }
        delay_ms(1500);  /* Pause between repeats */
        IWDG->KR = IWDG_KEY_RELOAD;
    }
}

/* ======================================================================== */
/*  ERROR HANDLER                                                           */
/* ======================================================================== */

static void state_error(flux_state_t src, int err_code) {
    printf("ERROR: state=%d code=%d\r\n", src, err_code);
    g_error_src = src;
    g_last_error = err_code;
    g_state = STATE_ERROR;
}

/* ======================================================================== */
/*  HARD FAULT HANDLER                                                      */
/* ======================================================================== */

void HardFault_Handler(void) {
    hardfault_handler_c();
}

static void hardfault_handler_c(void) {
    /* Try to preserve fault context for debugging */
    uint32_t cfsr = SCB->CFSR;
    uint32_t hfsr = SCB->HFSR;
    uint32_t mmfar = SCB->MMFAR;
    uint32_t bfar = SCB->BFAR;

    /* Log to flash if possible */
    (void)cfsr;
    (void)hfsr;
    (void)mmfar;
    (void)bfar;

    /* Blink red LED rapidly */
    while (1) {
        led_set(1, 0, 0);
        for (volatile int i = 0; i < 500000; i++);
        led_set(0, 0, 0);
        for (volatile int i = 0; i < 500000; i++);
    }
}

/* ======================================================================== */
/*  STUBS / WEAK HANDLERS                                                   */
/* ======================================================================== */

void NMI_Handler(void)         { while (1); }
void MemManage_Handler(void)   { while (1); }
void BusFault_Handler(void)    { while (1); }
void UsageFault_Handler(void)  { while (1); }
void SecureFault_Handler(void) { while (1); }
void SVC_Handler(void)         { /* Not used */ }
void DebugMon_Handler(void)    { /* Not used */ }
void PendSV_Handler(void)      { /* Not used */ }

/* ======================================================================== */
/*  INITIALIZATION TABLE                                                    */
/* ======================================================================== */

/* SCB vector table offset register */
#define SCB_VTOR ((volatile uint32_t *)0xE000ED08UL)

/* Default vector table in Flash (placed by linker) */
extern uint32_t _estack;
void Reset_Handler(void) __attribute__((naked));

/* Startup code */
void Reset_Handler(void) {
    __asm volatile(
        ".syntax unified\n"
        "  ldr   r0, =_estack\n"
        "  mov   sp, r0\n"
        "  bl    main\n"
        "  b     .\n"
        ".syntax divided\n"
    );
}

/* Minimal vector table for standalone operation */
/* (In production, the linker script + startup_stm32u5a9.s handles this) */