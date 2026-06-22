/*
 * main.c — BreathPrint firmware main application
 * BreathPrint — Portable Breath VOC Analyzer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 *
 * Architecture: Bare-metal super-loop with timer-driven sampling.
 * No RTOS — deterministic 20 Hz sensor sampling without context switch jitter.
 *
 * State machine:
 *   IDLE → WARMUP → SAMPLE → EXHALE_WAIT → ANALYZE → RESULT → IDLE
 */

#include "board.h"
#include "drivers/i2c_drv.h"
#include "drivers/adc_drv.h"
#include "drivers/sensor_array.h"
#include "drivers/ble.h"
#include "drivers/sdlog.h"
#include "drivers/display.h"
#include "drivers/ml_infer.h"
#include "drivers/breath_state.h"
#include "drivers/power.h"
#include "drivers/flash_drv.h"

/* ========================================================================
 * Global Context
 * ======================================================================== */

static device_ctx_t ctx;
static volatile uint32_t sys_tick_ms = 0;
static volatile uint8_t sample_flag = 0;

/* ========================================================================
 * SysTick Handler — 1 ms tick
 * ======================================================================== */

void SysTick_Handler(void) {
    sys_tick_ms++;
    ctx.state_timer_ms++;

    /* Trigger sensor sampling at 20 Hz (every 50 ms) */
    if (ctx.state == STATE_SAMPLE || ctx.state == STATE_EXHALE_WAIT) {
        static uint32_t last_sample_tick = 0;
        if (sys_tick_ms - last_sample_tick >= SAMPLE_PERIOD_MS) {
            last_sample_tick = sys_tick_ms;
            sample_flag = 1;
        }
    }
}

uint32_t millis(void) {
    return sys_tick_ms;
}

uint32_t micros(void) {
    uint32_t ms = sys_tick_ms;
    /* SysTick is down-counting from RELOAD */
    uint32_t ticks = SYST_RVR - SYST_CVR;
    return ms * 1000 + (ticks / (SYSTEM_CLOCK_HZ / 1000000));
}

void delay_ms(uint32_t ms) {
    uint32_t start = sys_tick_ms;
    while ((sys_tick_ms - start) < ms) {
        __asm volatile ("wfi");  /* Wait for interrupt in low power */
    }
}

void delay_us(uint32_t us) {
    uint32_t start = micros();
    while ((micros() - start) < us) {
        __asm volatile ("nop");
    }
}

/* ========================================================================
 * Clock Initialization
 *   HSE 16 MHz → PLL → 312 MHz SYSCLK
 *   AHB = 312 MHz, APB1 = 156 MHz, APB2 = 156 MHz
 * ======================================================================== */

void clock_init(void) {
    /* Enable HSE */
    RCC_CR |= RCC_CR_HSEON;
    while (!(RCC_CR & RCC_CR_HSERDY));

    /* Configure PLL1: M=4, N=156, P=2 → 312 MHz */
    RCC_PLLCFGR = RCC_PLLCFGR_PLLSRC_HSE |
                  (PLL_M << RCC_PLLCFGR_PLLM_SHIFT) |
                  (PLL_N << RCC_PLLCFGR_PLLN_SHIFT) |
                  (PLL_P << RCC_PLLCFGR_PLLP_SHIFT) |
                  (PLL_Q << RCC_PLLCFGR_PLLQ_SHIFT) |
                  (1U << 0);  /* PLL1 bypass disabled */

    /* Enable PLL1 */
    RCC_CR |= RCC_CR_PLL1ON;
    while (!(RCC_CR & RCC_CR_PLL1RDY));

    /* Set flash latency for 312 MHz */
    FLASH_ACR = FLASH_LATENCY | FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN;
    while ((FLASH_ACR & FLASH_ACR_LATENCY_MASK) != FLASH_LATENCY);

    /* Set bus prescalers: AHB=/1, APB1=/2, APB2=/2 */
    RCC_D1CFGR = 0x0;   /* D1CPRE=/1, HPRE=/1 → AHB=312 MHz */
    RCC_D2CFGR = 0x0;   /* APB1PRE=/2, APB2PRE=/2 → 156 MHz */

    /* Switch SYSCLK to PLL1 */
    RCC_CFGR = (RCC_CFGR & ~0x7U) | 0x3U;  /* SW = PLL1 */
    while ((RCC_CFGR & 0xFU) != (0x3U << 3));  /* Wait SWS = PLL1 */

    /* Enable FPU (CP10, CP11) */
    SCB_CPACR |= (0xFUL << 20);
    __asm volatile ("dsb");
    __asm volatile ("isb");
}

/* ========================================================================
 * GPIO Initialization
 * ======================================================================== */

void gpio_init(void) {
    /* Enable GPIO clocks */
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN |
                   RCC_AHB1ENR_GPIOCEN | RCC_AHB1ENR_GPIOHEN;

    /* Enable SYSCFG for EXTI */
    RCC_APB2ENR |= RCC_APB2ENR_SYSCFGEN;

    /* --- I2C1 pins (PB6/SCL, PB7/SDA) --- */
    gpio_config_af(I2C1_SCL_PORT, I2C1_SCL_PIN, I2C1_SCL_AF, GPIO_SPEED_HIGH);
    gpio_config_af(I2C1_SDA_PORT, I2C1_SDA_PIN, I2C1_SDA_AF, GPIO_SPEED_HIGH);
    /* Open-drain for I2C */
    I2C1_SCL_PORT->OTYPER |= (GPIO_OTYPE_OD << I2C1_SCL_PIN);
    I2C1_SDA_PORT->OTYPER |= (GPIO_OTYPE_OD << I2C1_SDA_PIN);
    I2C1_SCL_PORT->PUPDR |= (GPIO_PUPD_PULLUP << (I2C1_SCL_PIN * 2));
    I2C1_SDA_PORT->PUPDR |= (GPIO_PUPD_PULLUP << (I2C1_SDA_PIN * 2));

    /* --- I2C2 pins (PB10/SCL, PB11/SDA) --- */
    gpio_config_af(I2C2_SCL_PORT, I2C2_SCL_PIN, I2C2_SCL_AF, GPIO_SPEED_HIGH);
    gpio_config_af(I2C2_SDA_PORT, I2C2_SDA_PIN, I2C2_SDA_AF, GPIO_SPEED_HIGH);
    I2C2_SCL_PORT->OTYPER |= (GPIO_OTYPE_OD << I2C2_SCL_PIN);
    I2C2_SDA_PORT->OTYPER |= (GPIO_OTYPE_OD << I2C2_SDA_PIN);

    /* --- SPI1 pins (SD card: PA5/SCK, PA6/MISO, PA7/MOSI, PA4/CS) --- */
    gpio_config_af(SD_SCK_PORT, SD_SCK_PIN, SD_SCK_AF, GPIO_SPEED_VERY_HIGH);
    gpio_config_af(SD_MISO_PORT, SD_MISO_PIN, SD_MISO_AF, GPIO_SPEED_VERY_HIGH);
    gpio_config_af(SD_MOSI_PORT, SD_MOSI_PIN, SD_MOSI_AF, GPIO_SPEED_VERY_HIGH);
    gpio_config(SD_CS_PORT, SD_CS_PIN, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP,
                GPIO_SPEED_HIGH, GPIO_PUPD_PULLUP);
    gpio_set(SD_CS_PORT, SD_CS_PIN);  /* CS high (deselected) */

    /* --- SPI2 pins (Display: PB13/SCK, PB15/MOSI, PB12/CS, PB5/DC, PB4/RST) --- */
    gpio_config_af(DISP_SCK_PORT, DISP_SCK_PIN, DISP_SCK_AF, GPIO_SPEED_HIGH);
    gpio_config_af(DISP_MOSI_PORT, DISP_MOSI_PIN, DISP_MOSI_AF, GPIO_SPEED_HIGH);
    gpio_config(DISP_CS_PORT, DISP_CS_PIN, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP,
                GPIO_SPEED_HIGH, GPIO_PUPD_NONE);
    gpio_config(DISP_DC_PORT, DISP_DC_PIN, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP,
                GPIO_SPEED_HIGH, GPIO_PUPD_NONE);
    gpio_config(DISP_RST_PORT, DISP_RST_PIN, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP,
                GPIO_SPEED_HIGH, GPIO_PUPD_NONE);

    /* --- SPI4 pins (Flash: PE2/SCK, PE5/MISO, PE6/MOSI, PE4/CS) --- */
    /* Note: GPIOE clock — need to enable it */
    RCC_AHB1ENR |= (1U << 4);  /* GPIOEEN */
    gpio_config_af(FLASH_SCK_PORT, FLASH_SCK_PIN, FLASH_SCK_AF, GPIO_SPEED_VERY_HIGH);
    gpio_config_af(FLASH_MISO_PORT, FLASH_MISO_PIN, FLASH_MISO_AF, GPIO_SPEED_VERY_HIGH);
    gpio_config_af(FLASH_MOSI_PORT, FLASH_MOSI_PIN, FLASH_MOSI_AF, GPIO_SPEED_VERY_HIGH);
    gpio_config(FLASH_CS_PORT, FLASH_CS_PIN, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP,
                GPIO_SPEED_HIGH, GPIO_PUPD_PULLUP);
    gpio_set(FLASH_CS_PORT, FLASH_CS_PIN);

    /* --- USART2 (BLE: PA2/TX, PA3/RX) --- */
    gpio_config_af(BLE_TX_PORT, BLE_TX_PIN, BLE_TX_AF, GPIO_SPEED_HIGH);
    gpio_config_af(BLE_RX_PORT, BLE_RX_PIN, BLE_RX_AF, GPIO_SPEED_HIGH);
    gpio_config(BLE_RST_PORT, BLE_RST_PIN, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP,
                GPIO_SPEED_LOW, GPIO_PUPD_NONE);
    gpio_set(BLE_RST_PORT, BLE_RST_PIN);  /* BLE out of reset */

    /* --- USART3 (CH4 NDIR: PC10/TX, PC11/RX) --- */
    gpio_config_af(CH4_TX_PORT, CH4_TX_PIN, CH4_TX_AF, GPIO_SPEED_HIGH);
    gpio_config_af(CH4_RX_PORT, CH4_RX_PIN, CH4_RX_AF, GPIO_SPEED_HIGH);

    /* --- USART1 (Debug: PA9/TX, PA10/RX) --- */
    gpio_config_af(DBG_TX_PORT, DBG_TX_PIN, DBG_TX_AF, GPIO_SPEED_HIGH);
    gpio_config_af(DBG_RX_PORT, DBG_RX_PIN, DBG_RX_AF, GPIO_SPEED_HIGH);

    /* --- ADC pins (PC0/H2, PC1/Battery) --- */
    gpio_config(H2_ADC_PORT, H2_ADC_PIN, GPIO_MODE_ANALOG, 0, 0, GPIO_PUPD_NONE);
    gpio_config(BAT_ADC_PORT, BAT_ADC_PIN, GPIO_MODE_ANALOG, 0, 0, GPIO_PUPD_NONE);

    /* --- User button (PA0, input with pull-down) --- */
    gpio_config(BTN_PORT, BTN_PIN, GPIO_MODE_INPUT, 0, 0, GPIO_PUPD_PULLDOWN);

    /* --- Power button (PC13, input with pull-down) --- */
    gpio_config(PWR_BTN_PORT, PWR_BTN_PIN, GPIO_MODE_INPUT, 0, 0, GPIO_PUPD_PULLDOWN);

    /* --- Status LED (PB1, output) --- */
    gpio_config(LED_PORT, LED_PIN, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP,
                GPIO_SPEED_LOW, GPIO_PUPD_NONE);

    /* --- Sample LED (PB2, output) --- */
    gpio_config(SAMPLE_LED_PORT, SAMPLE_LED_PIN, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP,
                GPIO_SPEED_LOW, GPIO_PUPD_NONE);
    gpio_clear(SAMPLE_LED_PORT, SAMPLE_LED_PIN);

    /* --- Sensor power switch (PB8, output) --- */
    gpio_config(SENS_PWR_PORT, SENS_PWR_PIN, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP,
                GPIO_SPEED_LOW, GPIO_PUPD_NONE);
    gpio_clear(SENS_PWR_PORT, SENS_PWR_PIN);  /* Sensors off initially */

    /* --- Pump control (PB9, output) --- */
    gpio_config(PUMP_PORT, PUMP_PWR_PIN, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP,
                GPIO_SPEED_LOW, GPIO_PUPD_NONE);
    gpio_clear(PUMP_PORT, PUMP_PWR_PIN);

    /* --- Charger status (PA15, input) --- */
    gpio_config(CHRG_STAT_PORT, CHRG_STAT_PIN, GPIO_MODE_INPUT, 0,
                GPIO_SPEED_LOW, GPIO_PUPD_PULLUP);
}

/* ========================================================================
 * NVIC / Interrupt Initialization
 * ======================================================================== */

void nvic_init(void) {
    /* Set interrupt priorities (0 = highest) */
    NVIC_SET_PRIO(IRQn_TIM2, 0);       /* Sampling timer — highest */
    NVIC_SET_PRIO(IRQn_I2C1_EV, 1);    /* I2C sensor bus */
    NVIC_SET_PRIO(IRQn_DMA1_STR0, 1);  /* DMA for I2C */
    NVIC_SET_PRIO(IRQn_DMA1_STR1, 1);  /* DMA for SPI */
    NVIC_SET_PRIO(IRQn_USART2, 2);     /* BLE UART */
    NVIC_SET_PRIO(IRQn_USART3, 2);     /* CH4 sensor UART */
    NVIC_SET_PRIO(IRQn_ADC1, 3);       /* ADC */
    NVIC_SET_PRIO(IRQn_EXTI0, 4);      /* User button */

    /* Enable SysTick interrupt */
    SYST_RVR = (SYSTEM_CLOCK_HZ / 1000) - 1;  /* 1 ms tick */
    SYST_CVR = 0;
    SYST_CSR = SYST_CSR_ENABLE | SYST_CSR_TICKINT | SYST_CSR_CLKSOURCE;
}

/* ========================================================================
 * Board Initialization
 * ======================================================================== */

void board_init(void) {
    /* Disable interrupts during init */
    __asm volatile ("cpsid i");

    clock_init();
    gpio_init();

    /* Enable peripheral clocks */
    RCC_APB1ENR |= RCC_APB1ENR_I2C1EN | RCC_APB1ENR_I2C2EN |
                   RCC_APB1ENR_SPI2EN | RCC_APB1ENR_USART2EN |
                   RCC_APB1ENR_USART3EN | RCC_APB1ENR_TIM2EN |
                   RCC_APB1ENR_TIM3EN | RCC_APB1ENR_TIM6EN |
                   RCC_APB1ENR_PWREN;
    RCC_APB2ENR |= RCC_APB2ENR_SPI1EN | RCC_APB2ENR_SPI4EN |
                   RCC_APB2ENR_USART1EN | RCC_APB2ENR_ADC1EN |
                   RCC_APB2ENR_TIM1EN;
    RCC_AHB1ENR |= (1U << 21);  /* DMA1EN */
    RCC_AHB2ENR |= RCC_AHB2ENR_RNGEN | RCC_AHB2ENR_AESEN;
    RCC_AHB3ENR |= RCC_AHB3ENR_QSPIEN;

    /* Initialize drivers */
    i2c_init(I2C1, I2C_TIMING_400K);
    i2c_init(I2C2, I2C_TIMING_400K);
    adc_init(H2_ADC);
    flash_spi_init();
    sd_spi_init();
    display_init();
    ble_init();
    power_init();

    /* Initialize sensor array */
    sensor_array_init();

    /* Initialize ML model (load from flash) */
    ml_model_load();

    /* Load calibration from flash */
    calib_data_t calib;
    if (flash_read_calib(&calib) == 0 && calib.valid) {
        ctx.calib = calib;
    } else {
        /* Use default calibration (factory cal needed) */
        for (int i = 0; i < NUM_SENSORS; i++) {
            ctx.calib.zero_offset[i] = 0.0f;
            ctx.calib.span_gain[i] = 1.0f;
        }
        ctx.calib.valid = 0;
        ctx.error_code = ERR_CALIB_INVALID;
    }

    nvic_init();

    /* Enable RNG */
    RNG_CR = RNG_CR_RNGEN;

    /* Re-enable interrupts */
    __asm volatile ("cpsie i");
}

/* ========================================================================
 * State Machine: State Transitions
 * ======================================================================== */

static void transition_to(device_state_t new_state) {
    ctx.prev_state = ctx.state;
    ctx.state = new_state;
    ctx.state_timer_ms = 0;
}

static void start_breath_sample(void) {
    /* Enable sensor power domain */
    gpio_set(SENS_PWR_PORT, SENS_PWR_PIN);

    /* Start sensor heaters */
    sensor_array_heaters_on();

    /* Reset sample buffer */
    ctx.sample_index = 0;
    memset(ctx.samples, 0, sizeof(ctx.samples));

    /* Update BLE status */
    ble_notify_status(STATE_WARMUP, ctx.battery_pct, ctx.charging);

    /* Display warmup message */
    display_show_warmup();

    transition_to(STATE_WARMUP);
}

static void run_analysis(void) {
    feature_vector_t features;
    breath_result_t result;

    /* Extract features from sample buffer */
    sensor_array_extract_features(ctx.samples, ctx.sample_index,
                                  &ctx.calib, &features);

    /* Run ML inference */
    uint8_t metabolic_state = ml_infer(&features);

    /* Validate breath quality using CO2 and sensor response */
    uint8_t breath_quality = breath_validate(ctx.samples,
                                             ctx.sample_index,
                                             &ctx.calib);

    /* Estimate concentrations from sensor readings + features */
    breath_estimate(ctx.samples, ctx.sample_index, &ctx.calib,
                    &features, &result);

    result.timestamp = millis() / 1000;  /* TODO: RTC time */
    result.metabolic_state = metabolic_state;
    result.breath_quality = breath_quality;
    result.battery_pct = ctx.battery_pct;
    result.sensor_health = sensor_array_health();
    result.temperature = ctx.samples[ctx.sample_index - 1].temperature;
    result.humidity = ctx.samples[ctx.sample_index - 1].humidity;
    result.pressure = ctx.samples[ctx.sample_index - 1].pressure;

    ctx.last_result = result;

    /* Log to SD card */
    if (ctx.sd_present) {
        sdlog_write_result(&result);
    }

    /* Send result via BLE */
    ble_notify_result(&result);

    /* Display result */
    display_show_result(&result);

    /* Turn off heaters and sensors */
    sensor_array_heaters_off();
    gpio_clear(SENS_PWR_PORT, SENS_PWR_PIN);
    gpio_clear(SAMPLE_LED_PORT, SAMPLE_LED_PIN);

    transition_to(STATE_RESULT);
}

static void run_ambient_baseline(void) {
    if (ctx.state != STATE_IDLE) return;
    if (!ctx.calib.valid) return;
    if ((millis() - ctx.last_ambient_ms) < IDLE_AMBIENT_MS) return;

    /* Quick ambient sample (3 seconds, no heaters) */
    gpio_set(SENS_PWR_PORT, SENS_PWR_PIN);
    delay_ms(100);

    sensor_raw_t ambient;
    sensor_array_read_ambient(&ambient, &ctx.calib);

    /* Update zero offsets with exponential moving average */
    for (int i = 0; i < NUM_SENSORS; i++) {
        float reading = sensor_array_get_channel(&ambient, i);
        float offset = ctx.calib.zero_offset[i];
        ctx.calib.zero_offset[i] = offset * 0.95f + reading * 0.05f;
    }

    ctx.calib.ambient_count++;
    ctx.last_ambient_ms = millis();

    /* Save updated calibration */
    flash_write_calib(&ctx.calib);

    gpio_clear(SENS_PWR_PORT, SENS_PWR_PIN);
}

/* ========================================================================
 * State Machine: Main Loop
 * ======================================================================== */

static void state_machine_tick(void) {
    switch (ctx.state) {
    case STATE_IDLE:
        /* Check for button press to start sample */
        if (gpio_read(BTN_PORT, BTN_PIN)) {
            delay_ms(50);  /* Debounce */
            if (gpio_read(BTN_PORT, BTN_PIN)) {
                if (ctx.battery_pct < 10) {
                    display_show_error("Low battery");
                    ctx.error_code = ERR_LOW_BATTERY;
                } else {
                    start_breath_sample();
                }
            }
        }

        /* Check for BLE command */
        uint8_t cmd = ble_get_command();
        if (cmd == BLE_CMD_START_SAMPLE && ctx.battery_pct >= 10) {
            start_breath_sample();
        } else if (cmd == BLE_CMD_CALIBRATE) {
            transition_to(STATE_CALIBRATION);
        } else if (cmd == BLE_CMD_SET_TIME) {
            uint32_t new_time = ble_get_time();
            /* TODO: Set RTC */
            UNUSED(new_time);
        }

        /* Run ambient baseline if idle for >30 min */
        run_ambient_baseline();

        /* Display idle screen */
        if (ctx.state_timer_ms % 5000 == 0 && ctx.state_timer_ms > 0) {
            display_show_idle(ctx.battery_pct, ctx.ble_connected,
                              &ctx.last_result);
        }

        /* Enter sleep if idle for >60s and BLE not connected */
        if (ctx.state_timer_ms > 60000 && !ctx.ble_connected) {
            transition_to(STATE_SLEEP);
        }
        break;

    case STATE_WARMUP:
        /* Wait for sensor warmup */
        if (ctx.state_timer_ms >= WARMUP_DURATION_MS) {
            /* Check that sensors are responding */
            if (!sensor_array_check_ready()) {
                ctx.error_code = ERR_SENSOR_TIMEOUT;
                display_show_error("Sensor warmup failed");
                sensor_array_heaters_off();
                gpio_clear(SENS_PWR_PORT, SENS_PWR_PIN);
                transition_to(STATE_ERROR);
                return;
            }

            /* Light sample LED — user should exhale now */
            gpio_set(SAMPLE_LED_PORT, SAMPLE_LED_PIN);
            display_show_breathe_prompt();
            transition_to(STATE_SAMPLE);
        }

        /* Update display progress */
        if (ctx.state_timer_ms % 1000 == 0) {
            display_show_warmup_progress(ctx.state_timer_ms,
                                         WARMUP_DURATION_MS);
        }
        break;

    case STATE_SAMPLE:
        /* Process sensor sample if flag is set */
        if (sample_flag) {
            sample_flag = 0;

            if (ctx.sample_index < SAMPLE_COUNT) {
                sensor_raw_t *s = &ctx.samples[ctx.sample_index];
                sensor_array_sample_all(s, &ctx.calib);

                /* Stream raw data via BLE */
                ble_notify_sample(s, ctx.sample_index);

                ctx.sample_index++;
            }
        }

        /* Check for early termination (user stops exhaling) */
        if (ctx.sample_index > 20) {
            /* Check if CO2 is rising (valid exhalation) */
            uint16_t co2 = ctx.samples[ctx.sample_index - 1].scd41_co2;
            if (co2 > CO2_ALVEOLAR_MIN) {
                /* Valid alveolar breath detected — continue */
            }
        }

        /* Transition to exhalation wait after full sample window */
        if (ctx.state_timer_ms >= SAMPLE_DURATION_MS) {
            gpio_clear(SAMPLE_LED_PORT, SAMPLE_LED_PIN);
            display_show_analyzing();
            transition_to(STATE_EXHALE_WAIT);
        }
        break;

    case STATE_EXHALE_WAIT:
        /* Continue sampling for 5 more seconds (chamber equilibration) */
        if (sample_flag) {
            sample_flag = 0;
            if (ctx.sample_index < SAMPLE_COUNT) {
                sensor_raw_t *s = &ctx.samples[ctx.sample_index];
                sensor_array_sample_all(s, &ctx.calib);
                ctx.sample_index++;
            }
        }

        /* Flush chamber with pump after measurement */
        if (ctx.state_timer_ms == 2000) {
            gpio_set(PUMP_PORT, PUMP_PWR_PIN);
        }
        if (ctx.state_timer_ms == 5000) {
            gpio_clear(PUMP_PORT, PUMP_PWR_PIN);
        }

        if (ctx.state_timer_ms >= 6000) {
            transition_to(STATE_ANALYZE);
        }
        break;

    case STATE_ANALYZE:
        /* Run feature extraction + ML inference */
        run_analysis();
        break;

    case STATE_RESULT:
        /* Show result and wait for timeout or button press */
        if (ctx.state_timer_ms >= RESULT_DISPLAY_MS ||
            gpio_read(BTN_PORT, BTN_PIN)) {
            display_show_idle(ctx.battery_pct, ctx.ble_connected,
                              &ctx.last_result);
            transition_to(STATE_IDLE);
        }
        break;

    case STATE_CALIBRATION:
        /* Ambient air calibration mode */
        display_show_calibration();
        gpio_set(SENS_PWR_PORT, SENS_PWR_PIN);
        sensor_array_heaters_on();
        delay_ms(WARMUP_DURATION_MS);

        /* Take 60 samples over 3 seconds */
        sensor_raw_t calib_samples[60];
        for (int i = 0; i < 60; i++) {
            sensor_array_sample_all(&calib_samples[i], &ctx.calib);
            delay_ms(50);
        }

        /* Average and update zero offsets */
        for (int i = 0; i < NUM_SENSORS; i++) {
            float sum = 0;
            for (int j = 0; j < 60; j++) {
                sum += sensor_array_get_channel(&calib_samples[j], i);
            }
            ctx.calib.zero_offset[i] = sum / 60.0f;
        }

        ctx.calib.calibration_time = millis() / 1000;
        ctx.calib.ambient_count++;
        flash_write_calib(&ctx.calib);

        sensor_array_heaters_off();
        gpio_clear(SENS_PWR_PORT, SENS_PWR_PIN);

        display_show_calibration_done();
        ble_notify_status(STATE_IDLE, ctx.battery_pct, ctx.charging);
        transition_to(STATE_IDLE);
        break;

    case STATE_SLEEP:
        /* Deep sleep mode — wake on button or BLE */
        display_off();
        sensor_array_heaters_off();
        gpio_clear(SENS_PWR_PORT, SENS_PWR_PIN);

        /* Configure wake-up sources */
        enter_stop();

        /* Woke up — check reason */
        if (gpio_read(BTN_PORT, BTN_PIN) ||
            gpio_read(PWR_BTN_PORT, PWR_BTN_PIN)) {
            display_on();
            display_show_idle(ctx.battery_pct, ctx.ble_connected,
                              &ctx.last_result);
            transition_to(STATE_IDLE);
        }
        break;

    case STATE_ERROR:
        /* Error state — show error for 10s then return to idle */
        if (ctx.state_timer_ms >= 10000) {
            ctx.error_code = ERR_NONE;
            display_show_idle(ctx.battery_pct, ctx.ble_connected,
                              &ctx.last_result);
            transition_to(STATE_IDLE);
        }
        break;

    default:
        transition_to(STATE_IDLE);
        break;
    }
}

/* ========================================================================
 * Background Tasks (non-blocking, run each loop iteration)
 * ======================================================================== */

static void background_tasks(void) {
    /* Update battery monitoring every 5 seconds */
    static uint32_t last_bat_check = 0;
    if (millis() - last_bat_check > 5000) {
        last_bat_check = millis();
        ctx.battery_pct = power_read_battery();
        ctx.charging = power_is_charging();

        if (ctx.battery_pct < 10 && ctx.state == STATE_IDLE) {
            display_show_low_battery(ctx.battery_pct);
        }
    }

    /* BLE housekeeping */
    ble_poll();
    ctx.ble_connected = ble_is_connected();

    /* SD card housekeeping */
    static uint32_t last_sd_check = 0;
    if (millis() - last_sd_check > 10000) {
        last_sd_check = millis();
        ctx.sd_present = sd_detect();
    }

    /* Status LED update */
    static uint32_t last_led_update = 0;
    if (millis() - last_led_update > 500) {
        last_led_update = millis();
        if (ctx.state == STATE_IDLE) {
            if (ctx.ble_connected) {
                /* Solid dim green when connected and idle */
                gpio_set(LED_PORT, LED_PIN);
                delay_us(50);
                gpio_clear(LED_PORT, LED_PIN);
            } else {
                /* Slow blink when disconnected */
                gpio_toggle_led();
            }
        } else if (ctx.state == STATE_SAMPLE) {
            /* Fast blink during sampling */
            gpio_toggle_led();
        }
    }
}

static void gpio_toggle_led(void) {
    if (gpio_read(LED_PORT, LED_PIN)) {
        gpio_clear(LED_PORT, LED_PIN);
    } else {
        gpio_set(LED_PORT, LED_PIN);
    }
}

/* ========================================================================
 * Power Management
 * ======================================================================== */

void enter_sleep(void) {
    /* Enter Sleep mode (WFI wakes on any interrupt) */
    SCB_SCR &= ~SCB_SCR_SLEEPDEEP;
    __asm volatile ("dsb");
    __asm volatile ("wfi");
}

void enter_stop(void) {
    /* Enter Stop mode (low power, RAM retained, clocks stopped) */
    PWR_CR1 |= PWR_CR1_LPDS;
    SCB_SCR |= SCB_SCR_SLEEPDEEP;
    __asm volatile ("dsb");
    __asm volatile ("wfi");
    /* Woken up — reinit clocks */
    SCB_SCR &= ~SCB_SCR_SLEEPDEEP;
    clock_init();
}

/* ========================================================================
 * Main
 * ======================================================================== */

int main(void) {
    /* Initialize hardware and drivers */
    board_init();

    /* Initialize context */
    memset(&ctx, 0, sizeof(ctx));
    ctx.state = STATE_IDLE;
    ctx.state_timer_ms = 0;
    ctx.battery_pct = power_read_battery();
    ctx.charging = power_is_charging();
    ctx.sd_present = sd_detect();
    ctx.last_ambient_ms = millis();
    ctx.error_code = ERR_NONE;

    /* Show splash screen */
    display_show_splash();
    delay_ms(2000);
    display_show_idle(ctx.battery_pct, ctx.ble_connected, NULL);

    /* Start BLE advertising */
    ble_start_advertising();

    /* Main super-loop */
    while (1) {
        state_machine_tick();
        background_tasks();
    }

    return 0;  /* Never reached */
}