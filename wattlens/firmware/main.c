/*
 * main.c — WattLens firmware main application
 * WattLens — 3-Phase Power Quality Analyzer & NILM
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 *
 * Architecture: Bare-metal super-loop driven by ADS131M08 DRDY interrupt.
 * No RTOS — deterministic 2048 Hz sampling without context-switch jitter.
 *
 * State machine:
 *   BOOT → IDLE → MEASURE → (CALIBRATE | CAPTURE) → IDLE
 *
 * Each 1-second window (2048 samples × 8 channels) is processed through:
 *   1. power_calc   — real/reactive/apparent power, RMS, PF
 *   2. harmonics    — FFT → IEC 61000-4-7 harmonic grouping + THD
 *   3. flicker      — IEC 61000-4-15 Pst digital filter
 *   4. event_detect — sag/swell/interruption detection
 *   5. nilm         — int8 neural network appliance disaggregation
 *
 * Results → display, SD log, BLE notify, LoRa (throttled).
 */

#include "board.h"
#include "registers.h"
#include "drivers/adc_drv.h"
#include "drivers/dsp_fft.h"
#include "drivers/power_calc.h"
#include "drivers/harmonics.h"
#include "drivers/flicker.h"
#include "drivers/event_detect.h"
#include "drivers/nilm.h"
#include "drivers/ble.h"
#include "drivers/lora.h"
#include "drivers/display.h"
#include "drivers/sdlog.h"
#include "drivers/flash_drv.h"
#include "drivers/power.h"

/* ========================================================================
 * Global Context
 * ======================================================================== */

static device_ctx_t ctx;
static volatile uint32_t sys_tick_ms = 0;
static volatile uint8_t window_ready = 0;       /* set when 2048 samples collected */
static volatile uint16_t sample_idx = 0;

/* Ping-pong sample buffers (DMA fills one while DSP processes the other) */
static float sample_buf[2][NUM_CHANNELS][WINDOW_SAMPLES];
static uint8_t active_buf = 0;                  /* buffer being filled by DMA */

/* Raw int24 ADC data (DMA target before conversion to float) */
static uint8_t raw_frame[2][NUM_CHANNELS * 3 * WINDOW_SAMPLES];

/* Processing buffers */
static float fft_in[FFT_SIZE];
static float fft_out[FFT_SIZE];
static float window_coef[FFT_SIZE];             /* Hann window coefficients */

/* ========================================================================
 * SysTick Handler — 1 ms tick
 * ======================================================================== */
void SysTick_Handler(void) {
    sys_tick_ms++;
}

uint32_t millis(void) {
    return sys_tick_ms;
}

uint32_t micros(void) {
    uint32_t ms = sys_tick_ms;
    uint32_t cnt = SYST_CVR;
    return ms * 1000 + ((SYST_RVR - cnt) * 1000 / (SYST_RVR + 1));
}

/* ========================================================================
 * ADC DRDY interrupt handler
 *
 * The ADS131M08 asserts DRDY at 2048 Hz.  Each edge triggers a DMA SPI
 * transfer of the 8-channel × 3-byte frame.  We just count samples here;
 * the DMA completion interrupt does the heavy lifting of buffer management.
 * ======================================================================== */
void EXTI9_5_IRQHandler(void) {
    if (EXTI_PR1 & BIT(5)) {
        EXTI_PR1 = BIT(5);                     /* clear pending */

        /* Start a DMA-driven SPI read of one ADC frame (24 bytes) */
        adc_start_frame_read(&raw_frame[active_buf][sample_idx * (NUM_CHANNELS * 3)]);

        sample_idx++;
        if (sample_idx >= WINDOW_SAMPLES) {
            sample_idx = 0;
            window_ready = 1;
            active_buf ^= 1;                   /* swap ping-pong buffer */
        }
    }
}

/* ========================================================================
 * Clock initialization
 * Configures HSE → PLL → 480 MHz SYSCLK
 * ======================================================================== */
static void clock_init(void) {
    /* Enable HSE (external 16 MHz crystal) */
    RCC_CR |= BIT(16);                          /* HSEON */
    while (!(RCC_CR & BIT(17))) { }             /* wait HSERDY */

    /* Configure PLL1: HSE / 2 × 120 / 2 = 480 MHz (M=2, N=120, P=2) */
    RCC_PLLCFGR = (2 << 4) | (120 << 8) | (0 << 17) | (1 << 24); /* PLLP=2, PLLSRC=HSE */

    /* Voltage scale 1 (required for 480 MHz) */
    PWR_D3CR = (3 << 14);                       /* VOS = scale 1 */
    while (!(PWR_CSR1 & BIT(13))) { }           /* VOSRDY */

    /* Configure D1/D2/D3 bus dividers before switching SYSCLK */
    RCC_D1CFGR = (0 << 0) | (4 << 4) | (4 << 8);  /* D1CPRE=1, HPRE=2, D1PPRE=16→ actual: /1 /4 /2 */
    /* Simplified: HPRE=1, D1PPRE=/2, D2PPRE=/4 → set below */

    /* Enable PLL1 */
    RCC_CR |= BIT(24);                          /* PLL1ON */
    while (!(RCC_CR & BIT(25))) { }             /* wait PLL1RDY */

    /* Switch SYSCLK to PLL1 */
    RCC_CFGR = (3 << 0);                        /* SW = PLL1 */
    while (((RCC_CFGR >> 3) & 3) != 3) { }     /* wait SWS = PLL1 */

    /* Enable peripheral clocks */
    RCC_AHB4ENR |= RCC_AHB4ENR_GPIOAEN | RCC_AHB4ENR_GPIOBEN |
                   RCC_AHB4ENR_GPIOCEN | RCC_AHB4ENR_GPIODEN |
                   RCC_AHB4ENR_GPIOEEN;
    RCC_AHB1ENR |= RCC_AHB1ENR_DMA1EN | RCC_AHB1ENR_DMA2EN;
    RCC_APB1LENR |= RCC_APB1LENR_SPI2EN | RCC_APB1LENR_SPI3EN |
                    RCC_APB1LENR_USART2EN | RCC_APB1LENR_USART3EN |
                    RCC_APB1LENR_I2C1EN | RCC_APB1LENR_TIM6EN;
    RCC_APB2ENR |= RCC_APB2ENR_USART1EN;
}

/* ========================================================================
 * GPIO initialization
 * ======================================================================== */
static void gpio_config(uint32_t port, uint8_t pin, uint8_t mode,
                        uint8_t speed, uint8_t pupd, uint8_t af) {
    volatile uint32_t *moder = (volatile uint32_t *)(port + GPIO_MODER_OFF);
    volatile uint32_t *ospeedr = (volatile uint32_t *)(port + GPIO_OSPEEDR_OFF);
    volatile uint32_t *pupdr = (volatile uint32_t *)(port + GPIO_PUPDR_OFF);
    volatile uint32_t *afrl = (volatile uint32_t *)(port + GPIO_AFRL_OFF);
    volatile uint32_t *afrh = (volatile uint32_t *)(port + GPIO_AFRH_OFF);

    *moder &= ~(3 << (pin * 2));
    *moder |= (mode << (pin * 2));

    *ospeedr &= ~(3 << (pin * 2));
    *ospeedr |= (speed << (pin * 2));

    *pupdr &= ~(3 << (pin * 2));
    *pupdr |= (pupd << (pin * 2));

    if (pin < 8) {
        *afrl &= ~(0xF << (pin * 4));
        *afrl |= (af << (pin * 4));
    } else {
        *afrh &= ~(0xF << ((pin - 8) * 4));
        *afrh |= (af << ((pin - 8) * 4));
    }
}

static void gpio_init_all(void) {
    /* --- SPI2 (ADS131M08): PB12=NSS, PB13=SCK, PB14=MISO, PB15=MOSI --- */
    gpio_config(GPIOB_BASE, 12, GPIO_MODE_AF, GPIO_SPEED_VHIGH, GPIO_PUPD_PU, 5);
    gpio_config(GPIOB_BASE, 13, GPIO_MODE_AF, GPIO_SPEED_VHIGH, GPIO_PUPD_NONE, 5);
    gpio_config(GPIOB_BASE, 14, GPIO_MODE_AF, GPIO_SPEED_VHIGH, GPIO_PUPD_NONE, 5);
    gpio_config(GPIOB_BASE, 15, GPIO_MODE_AF, GPIO_SPEED_VHIGH, GPIO_PUPD_NONE, 5);

    /* --- SPI3 (ILI9341): PA15=NSS, PB5=MOSI, PB4=MISO --- */
    gpio_config(GPIOA_BASE, 15, GPIO_MODE_AF, GPIO_SPEED_VHIGH, GPIO_PUPD_PU, 6);
    gpio_config(GPIOB_BASE, 4,  GPIO_MODE_AF, GPIO_SPEED_VHIGH, GPIO_PUPD_NONE, 6);
    gpio_config(GPIOB_BASE, 5,  GPIO_MODE_AF, GPIO_SPEED_VHIGH, GPIO_PUPD_NONE, 6);
    /* SPI3 SCK is on PC3 in some configs, but we use PB3 remapped */
    gpio_config(GPIOB_BASE, 3,  GPIO_MODE_AF, GPIO_SPEED_VHIGH, GPIO_PUPD_NONE, 6);

    /* --- USART2 (BLE): PA2=TX, PA3=RX --- */
    gpio_config(GPIOA_BASE, 2, GPIO_MODE_AF, GPIO_SPEED_HIGH, GPIO_PUPD_NONE, 7);
    gpio_config(GPIOA_BASE, 3, GPIO_MODE_AF, GPIO_SPEED_HIGH, GPIO_PUPD_NONE, 7);

    /* --- USART3 (LoRa): PB10=TX, PB11=RX --- */
    gpio_config(GPIOB_BASE, 10, GPIO_MODE_AF, GPIO_SPEED_HIGH, GPIO_PUPD_NONE, 7);
    gpio_config(GPIOB_BASE, 11, GPIO_MODE_AF, GPIO_SPEED_HIGH, GPIO_PUPD_NONE, 7);

    /* --- I2C1 (RTC/IMU/fuel): PB6=SCL, PB7=SDA --- */
    gpio_config(GPIOB_BASE, 6, GPIO_MODE_AF, GPIO_SPEED_HIGH, GPIO_PUPD_PU, 4);
    gpio_config(GPIOB_BASE, 7, GPIO_MODE_AF, GPIO_SPEED_HIGH, GPIO_PUPD_PU, 4);

    /* --- ADC DRDY input (PC5, EXTI5 rising) --- */
    gpio_config(GPIOC_BASE, 5, GPIO_MODE_IN, GPIO_SPEED_HIGH, GPIO_PUPD_PD, 0);
    EXTI_IMR1 |= BIT(5);
    EXTI_RTSR1 |= BIT(5);
    NVIC_IP(IRQ_EXTI9_5) = 5;
    NVIC_ISER(0) |= BIT(IRQ_EXTI9_5);

    /* --- LoRa DIO1 (PD4, EXTI4 rising) --- */
    gpio_config(GPIOD_BASE, 4, GPIO_MODE_IN, GPIO_SPEED_HIGH, GPIO_PUPD_PD, 0);
    EXTI_IMR1 |= BIT(4);
    EXTI_RTSR1 |= BIT(4);
    NVIC_IP(IRQ_EXTI4) = 6;
    NVIC_ISER(0) |= BIT(IRQ_EXTI4);

    /* --- Display touch IRQ (PE0, EXTI0) --- */
    gpio_config(GPIOE_BASE, 0, GPIO_MODE_IN, GPIO_SPEED_HIGH, GPIO_PUPD_PU, 0);
    EXTI_IMR1 |= BIT(0);
    EXTI_FTSR1 |= BIT(0);                       /* falling edge (active low) */
    NVIC_IP(IRQ_EXTI0) = 7;
    NVIC_ISER(0) |= BIT(IRQ_EXTI0);

    /* --- Output pins: ADC reset, display DC/RS/reset, SD CS, LEDs --- */
    gpio_config(GPIOC_BASE, 4, GPIO_MODE_OUT, GPIO_SPEED_LOW, GPIO_PUPD_NONE, 0);   /* ADC RESET */
    gpio_config(GPIOC_BASE, 6, GPIO_MODE_OUT, GPIO_SPEED_LOW, GPIO_PUPD_NONE, 0);   /* DISP DC */
    gpio_config(GPIOC_BASE, 7, GPIO_MODE_OUT, GPIO_SPEED_LOW, GPIO_PUPD_NONE, 0);   /* DISP RST */
    gpio_config(GPIOC_BASE, 8, GPIO_MODE_OUT, GPIO_SPEED_LOW, GPIO_PUPD_PU, 0);     /* SD CS */
    gpio_config(GPIOD_BASE, 0, GPIO_MODE_OUT, GPIO_SPEED_LOW, GPIO_PUPD_NONE, 0);   /* LED red */
    gpio_config(GPIOD_BASE, 1, GPIO_MODE_OUT, GPIO_SPEED_LOW, GPIO_PUPD_NONE, 0);   /* LED green */
    gpio_config(GPIOD_BASE, 2, GPIO_MODE_OUT, GPIO_SPEED_LOW, GPIO_PUPD_NONE, 0);   /* charge en */
    gpio_config(GPIOD_BASE, 5, GPIO_MODE_OUT, GPIO_SPEED_LOW, GPIO_PUPD_NONE, 0);   /* charge LED */
    gpio_config(GPIOE_BASE, 1, GPIO_MODE_OUT, GPIO_SPEED_LOW, GPIO_PUPD_NONE, 0);   /* ADC PWDN */
    gpio_config(GPIOE_BASE, 7, GPIO_MODE_OUT, GPIO_SPEED_LOW, GPIO_PUPD_NONE, 0);   /* USB VBUS en */
    gpio_config(GPIOE_BASE, 8, GPIO_MODE_OUT, GPIO_SPEED_LOW, GPIO_PUPD_NONE, 0);   /* main power en */

    /* --- USB (PA11=DM, PA12=DP) --- */
    gpio_config(GPIOA_BASE, 11, GPIO_MODE_AF, GPIO_SPEED_HIGH, GPIO_PUPD_NONE, 10);
    gpio_config(GPIOA_BASE, 12, GPIO_MODE_AF, GPIO_SPEED_HIGH, GPIO_PUPD_NONE, 10);

    /* --- Battery voltage ADC (PB0) --- */
    gpio_config(GPIOB_BASE, 0, GPIO_MODE_ANALOG, GPIO_SPEED_LOW, GPIO_PUPD_NONE, 0);
}

/* ========================================================================
 * SysTick init — 1 ms tick at 480 MHz
 * ======================================================================== */
static void systick_init(void) {
    SYST_RVR = (SYSCLK_HZ / 1000) - 1;          /* 480000 - 1 */
    SYST_CVR = 0;
    SYST_CSR = 0x7;                              /* CLKSOURCE=processor, TICKINT=1, ENABLE=1 */
}

/* ========================================================================
 * Window coefficient initialization (Hann window)
 * ======================================================================== */
static void window_init(void) {
    for (int i = 0; i < FFT_SIZE; i++) {
        window_coef[i] = 0.5f * (1.0f - cosf(2.0f * 3.14159265f * (float)i / (float)(FFT_SIZE - 1)));
    }
}

/* ========================================================================
 * Convert raw int24 ADC data to float volts/amps
 * Applies calibration gains, offsets, and phase corrections.
 * ======================================================================== */
static void convert_samples(uint8_t buf_idx) {
    cal_data_t *cal = &ctx.cal;
    int8_t *raw = (int8_t *)&raw_frame[buf_idx][0];

    for (int s = 0; s < WINDOW_SAMPLES; s++) {
        for (int ch = 0; ch < NUM_CHANNELS; ch++) {
            int32_t v24 = ((int32_t)raw[s * NUM_CHANNELS * 3 + ch * 3] << 16) |
                          ((int32_t)(uint8_t)raw[s * NUM_CHANNELS * 3 + ch * 3 + 1] << 8) |
                          ((int32_t)(uint8_t)raw[s * NUM_CHANNELS * 3 + ch * 3 + 2]);
            /* Sign-extend 24-bit to 32-bit */
            if (v24 & 0x800000) v24 |= 0xFF000000;

            float fv = (float)v24;
            if (ch < 3) {
                /* voltage channels */
                fv = (fv - cal->v_offset[ch]) * cal->v_gain[ch];
            } else {
                /* current channels */
                fv = (fv - cal->i_offset[ch - 3]) * cal->i_gain[ch - 3] * cal->ct_ratio[ch - 3];
            }
            sample_buf[buf_idx][ch][s] = fv;
        }
    }
}

/* ========================================================================
 * Process one 1-second measurement window
 * Runs the full DSP pipeline: power → harmonics → flicker → events → NILM
 * ======================================================================== */
static void process_window(uint8_t buf_idx) {
    float (*buf)[WINDOW_SAMPLES] = sample_buf[buf_idx];

    /* 1. Power calculation: real, reactive, apparent, RMS, PF */
    power_calc_compute(&buf[0][0], &buf[3][0], &ctx.metrics, &ctx.cal);

    /* 2. Harmonic analysis via FFT (per phase) */
    for (int phase = 0; phase < 3; phase++) {
        /* Window and load voltage channel */
        for (int i = 0; i < FFT_SIZE; i++) {
            fft_in[i] = buf[phase][i] * window_coef[i];
        }
        dsp_fft_run(fft_in, fft_out);
        harmonics_extract_v(fft_out, phase, &ctx.harmonics, ctx.metrics.freq);

        /* Window and load current channel */
        for (int i = 0; i < FFT_SIZE; i++) {
            fft_in[i] = buf[phase + 3][i] * window_coef[i];
        }
        dsp_fft_run(fft_in, fft_out);
        harmonics_extract_i(fft_out, phase, &ctx.harmonics, ctx.metrics.freq);
    }

    /* Compute THD from harmonic magnitudes */
    harmonics_compute_thd(&ctx.harmonics, &ctx.metrics);

    /* 3. Flicker Pst (IEC 61000-4-15) — updated incrementally */
    flicker_update(&buf[0][0], &buf[1][0], &buf[2][0], ctx.metrics.freq);

    /* 4. Event detection: sag/swell/interruption */
    event_detect_check(&ctx.metrics, &ctx);

    /* 5. NILM inference — appliance disaggregation */
    nilm_infer(&ctx.metrics, &ctx.harmonics, &ctx.nilm);
    event_detect_nilm(&ctx.nilm, &ctx);
}

/* ========================================================================
 * Calibration load / validate
 * ======================================================================== */
static void calibration_load(void) {
    flash_drv_read_cal(&ctx.cal);
    /* Validate CRC32 */
    if (!flash_drv_validate_cal(&ctx.cal)) {
        /* Load defaults */
        for (int i = 0; i < 3; i++) {
            ctx.cal.v_gain[i] = DEFAULT_V_GAIN;
            ctx.cal.v_offset[i] = 0.0f;
            ctx.cal.v_phase[i] = 0.0f;
        }
        for (int i = 0; i < 4; i++) {
            ctx.cal.i_gain[i] = DEFAULT_I_GAIN;
            ctx.cal.i_offset[i] = 0.0f;
            ctx.cal.i_phase[i] = 0.0f;
            ctx.cal.ct_ratio[i] = DEFAULT_CT_RATIO;
            ctx.cal.ct_burden[i] = DEFAULT_CT_BURDEN;
        }
        ctx.cal.grid_freq = GRID_FREQ_HZ;
        ctx.error_flags |= ERR_CAL_INVALID;
    }
}

/* ========================================================================
 * Main
 * ======================================================================== */
int main(void) {
    /* --- Boot --- */
    clock_init();
    gpio_init_all();
    systick_init();

    /* Initialize status LEDs: red on during boot */
    GPIO_REG(GPIOD_BASE, GPIO_ODR_OFF) |= BIT(0);

    ctx.state = STATE_BOOT;
    ctx.uptime_s = 0;
    ctx.sample_count = 0;

    /* Initialize drivers */
    display_init();
    display_show_boot("WattLens v1.0  jayis1");

    flash_drv_init();
    calibration_load();

    adc_drv_init();
    window_init();

    power_calc_init();
    harmonics_init();
    flicker_init();
    event_detect_init();

    sdlog_init();
    ble_init();
    lora_init();
    power_mgmt_init();

    /* Load NILM model from QSPI flash */
    if (nilm_load_model() != 0) {
        ctx.error_flags |= ERR_NILM_MODEL;
        display_show_error("NILM model missing");
    }
    nilm_init();

    /* Enable ADC sampling */
    adc_drv_start();
    ctx.state = STATE_MEASURE;
    GPIO_REG(GPIOD_BASE, GPIO_ODR_OFF) &= ~BIT(0);  /* red off */
    GPIO_REG(GPIOD_BASE, GPIO_ODR_OFF) |= BIT(1);   /* green on */

    display_show_main();

    /* --- Main super-loop --- */
    uint32_t last_ble_tx = 0;
    uint32_t last_lora_tx = 0;
    uint32_t last_display = 0;
    uint32_t last_sdlog = 0;
    uint32_t uptime_last = 0;

    while (1) {
        /* Check for completed measurement window */
        if (window_ready) {
            window_ready = 0;
            uint8_t done_buf = active_buf ^ 1;     /* process the just-filled buffer */

            /* Convert raw int24 → float (calibrated) */
            convert_samples(done_buf);

            /* Run full DSP pipeline on the completed window */
            process_window(done_buf);

            ctx.sample_count++;
        }

        /* Uptime counter (1 Hz) */
        if (sys_tick_ms - uptime_last >= 1000) {
            uptime_last = sys_tick_ms;
            ctx.uptime_s++;
        }

        /* BLE transmit real-time metrics (1 Hz) */
        if (ctx.ble_connected && (sys_tick_ms - last_ble_tx >= BLE_TX_INTERVAL_MS)) {
            last_ble_tx = sys_tick_ms;
            ble_send_metrics(&ctx.metrics, &ctx.nilm);
        }

        /* LoRa transmit (every 30 s) */
        if (ctx.lora_joined && (sys_tick_ms - last_lora_tx >= LORA_TX_INTERVAL_MS)) {
            last_lora_tx = sys_tick_ms;
            lora_send_summary(&ctx.metrics, &ctx.nilm);
        }

        /* Display update (2 Hz) */
        if (sys_tick_ms - last_display >= DISPLAY_UPDATE_MS) {
            last_display = sys_tick_ms;
            display_update_metrics(&ctx.metrics, &ctx.nilm, ctx.battery_pct);
        }

        /* SD log (1 Hz) */
        if (ctx.sd_present && (sys_tick_ms - last_sdlog >= 1000)) {
            last_sdlog = sys_tick_ms;
            sdlog_write_metrics(&ctx.metrics, &ctx.nilm, ctx.uptime_s);
        }

        /* Poll BLE command queue */
        ble_poll_commands(&ctx);

        /* Poll LoRa downlink */
        lora_poll_downlink(&ctx);

        /* Battery / power management */
        power_mgmt_poll(&ctx);

        /* Error handling */
        if (ctx.error_flags & ERR_BATTERY_LOW) {
            display_show_warning("Battery low");
        }
        if (ctx.error_flags & ERR_OVERTEMP) {
            display_show_warning("Over-temperature");
        }

        /* Low-power WFI between windows (interrupt wakes us) */
        __asm volatile ("wfi");
    }
}

/* ========================================================================
 * Math helpers (exposed for other modules)
 * ======================================================================== */
float cosf(float x) {
    /* Taylor series approximation (simplified — CMSIS-DSP provides arm_sin_cos_f32) */
    /* In production, use CMSIS-DSP arm_sin_cos_f32.  This stub is for compilation. */
    float x2 = x * x;
    float x4 = x2 * x2;
    return 1.0f - x2 * 0.5f + x4 * 0.0416667f - x4 * x2 * 0.00138889f;
}

float sinf(float x) {
    return cosf(x - 1.5707963f);
}

float sqrtf(float x) {
    if (x <= 0.0f) return 0.0f;
    float r = x * 0.5f;
    for (int i = 0; i < 8; i++) {
        r = 0.5f * (r + x / r);
    }
    return r;
}

float atan2f(float y, float x) {
    /* Approximation */
    float z = y / x;
    if (x > 0) return z / (1.0f + 0.28f * z * z);
    if (y >= 0 && x < 0) return 3.14159265f + z / (1.0f + 0.28f * z * z);
    if (y < 0 && x < 0) return -3.14159265f + z / (1.0f + 0.28f * z * z);
    if (y > 0 && x == 0) return 1.5707963f;
    if (y < 0 && x == 0) return -1.5707963f;
    return 0.0f;
}