/*
 * main.c — LithoCore top-level firmware.
 *
 * State machine, button handling, power management, and the main
 * measurement orchestration loop that ties together the EIS sweep,
 * DCIR pulse, OCV relaxation, CNLS fitting, and SoH classification.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#include <stdint.h>
#include <string.h>
#include "board.h"
#include "registers.h"

/* driver headers */
#include "drivers/ads1256.h"
#include "drivers/dds.h"
#include "drivers/lockin.h"
#include "drivers/eis_sweep.h"
#include "drivers/randles.h"
#include "drivers/cnls.h"
#include "drivers/soh.h"
#include "drivers/dcir.h"
#include "drivers/coulomb.h"
#include "drivers/safety.h"
#include "drivers/power.h"
#include "drivers/ble.h"
#include "drivers/usb.h"
#include "drivers/storage.h"

/* -------------------------------------------------------------------------
 * Global state
 * ------------------------------------------------------------------------- */

/* Chemistry baseline table — indexed by board.h chemistry enum.
 * Values are representative of fresh, healthy cells at 25 °C.
 * Author: jayis1 */
const chemistry_baseline_t chemistry_table[5] = {
    /* 0: NMC 18650 3500 mAh */
    { "NMC-18650", 3700, 3500,  35,  8, 25, 800 },
    /* 1: NMC 21700 5000 mAh */
    { "NMC-21700", 3700, 5000,  18,  5, 15, 1200 },
    /* 2: LFP 26650 3300 mAh */
    { "LFP-26650", 3300, 3300,  12,  4, 10, 1500 },
    /* 3: NCA 18650 3500 mAh */
    { "NCA-18650", 3600, 3500,  30,  7, 22, 900 },
    /* 4: LCO laptop pack */
    { "LCO-pack",  3700, 2200,  60, 12, 30, 600 },
};

static volatile sys_state_t g_state = STATE_IDLE;
static volatile uint32_t    g_ticks = 0;       /* ms counter (SysTick) */
static volatile uint32_t    g_btn_press_ts = 0;
static volatile uint8_t     g_btn_held = 0;
static volatile uint8_t     g_btn_long_press = 0;
static volatile uint8_t     g_fault_flag = 0;

static litho_config_t g_config = DEFAULT_CONFIG;

/* Latest measurement result — filled by the measurement pipeline,
 * reported via BLE/USB, and stored in flash. */
static soh_result_t g_last_result;
static uint8_t      g_result_valid = 0;

/* Sweep progress (for BLE status notifications) */
static volatile uint8_t  g_sweep_progress = 0;   /* 0-100 */
static volatile uint16_t g_sweep_freq_idx = 0;
static volatile uint16_t g_sweep_freq_count = 0;

/* -------------------------------------------------------------------------
 * Forward declarations
 * ------------------------------------------------------------------------- */
static void     clock_init(void);
static void     gpio_init(void);
static void     systick_init(void);
static void     enter_stop2(void);
static void     handle_button(void);
static void     run_fast_sweep(void);
static void     run_full_sweep(void);
static void     run_measurement_cycle(uint8_t full);
static void     update_leds(void);
static void     fault_handler(const char *msg);
static uint32_t millis(void);
static void     delay_ms(uint32_t ms);

/* -------------------------------------------------------------------------
 * SysTick interrupt — 1 ms tick
 * ------------------------------------------------------------------------- */
void SysTick_Handler(void)
{
    g_ticks++;
}

/* -------------------------------------------------------------------------
 * EXTI9_5 handler — button (PB9) + ADS DRDY (PB14) + OVP fault (PC2)
 *
 * PB9  = user button (falling edge)
 * PB14 = ADS1256 data ready (falling edge) — handled in ads1256.c ISR part
 * PC2  = over-voltage fault (rising edge) — hardware safety
 * ------------------------------------------------------------------------- */
void EXTI9_5_Handler(void)
{
    /* Check which line triggered (simplified — in real HW we'd check EXTI PR) */
    /* For this firmware we poll the button in the main loop instead of
       relying on EXTI for the button, to keep the ISR simple. The ADS DRDY
       and OVP fault have dedicated polling in their drivers. */
}

/* -------------------------------------------------------------------------
 * Millis / delay
 * ------------------------------------------------------------------------- */
static uint32_t millis(void)
{
    return g_ticks;
}

static void delay_ms(uint32_t ms)
{
    uint32_t start = g_ticks;
    while ((g_ticks - start) < ms) {
        /* wait — interrupts still fire */
    }
}

/* -------------------------------------------------------------------------
 * Clock initialization
 *
 * HSE = 16.384 MHz TCXO → PLL: M=4, N=41, R=1 → 16.384/4*41/1 = 167.9 MHz
 * We round to 170 MHz by using N=41.5 via the fractional PLL (G474 supports
 * fractional N with 0.5 step). For simplicity we use N=41 → 167.9 MHz,
 * which is within spec and keeps the ADC/timer math exact relative to the
 * TCXO.
 *
 * Author: jayis1
 * ------------------------------------------------------------------------- */
static void clock_init(void)
{
    /* Enable HSE (external TCXO) */
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY)) { }

    /* Configure PLL: source = HSE, M=4, N=41, R=2 → 167.936 MHz on R output.
       But we want SYSCLK = 167.936 MHz. Set PLLCFGR:
         PLLM = 4 (bits[3:0])
         PLLN = 41 (bits[14:6])
         PLLR = 2 (bits[26:25] = 0b00)
         PLLSRC = HSE (bit 22 = 1)
       Note: G474 supports PLLR values 2,4,6,8 and fractional N.
       16.384 MHz / 4 * 41 / 2 = 83.968 MHz → too low for R=2.
       Use R=1? G474 PLLR: 00=2, 01=4, 10=6, 11=8. Minimum is 2.
       So use M=2, N=41, R=4 → 16.384/2*41/4 = 83.97 MHz... still low.
       Actually G474 PLL: input 1-16 MHz after M, VCO 96-344 MHz, output = VCO/R.
       M=4 → PLLIN = 4.096 MHz (valid 1-16). N=41 → VCO = 167.9 MHz (valid).
       R=2 → 83.97 MHz. To get ~170 MHz we need VCO/R = 170.
       M=2, N=21, R=1? R min is 2.
       M=1, N=21, R=2 → 16.384*21/2 = 172 MHz. VCO = 344 MHz (max). OK!
       Actually VCO = PLLIN * N = 16.384 * 21 = 344.064 MHz (just under 344 max).
       R=2 → 172.032 MHz. Close to 170 but slightly over. Use N=20 → 327.68 MHz VCO.
       R=2 → 163.84 MHz. Under 170 max but safe.
       Let's use M=1, N=20, R=2 → 163.84 MHz. This keeps all timer math
       as exact powers of 2 relative to the 16.384 MHz TCXO. */
    RCC->PLLCFGR = (1U << 0)          /* PLLM = 1 */
                 | (20U << 6)         /* PLLN = 20 */
                 | (0x0U << 25)       /* PLLR = 2 */
                 | (1U << 22);        /* PLLSRC = HSE */

    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY)) { }

    /* Set flash latency for 163 MHz (3 wait states) */
    FLASH_REG->ACR = 0x3U << 0;  /* 3 wait states */

    /* Switch SYSCLK to PLL */
    RCC->CFGR = (RCC->CFGR & ~0x3U) | RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS_MASK) != RCC_CFGR_SWS_PLL) { }

    /* Disable HSI to save power */
    RCC->CR &= ~RCC_CR_HSION;
}

/* -------------------------------------------------------------------------
 * GPIO initialization
 * ------------------------------------------------------------------------- */
static void gpio_init(void)
{
    /* Enable clocks for GPIOA, GPIOB, GPIOC, GPIOF */
    /* RCC AHB2ENR is at RCC_BASE + 0x4C (G4) */
    volatile uint32_t *ahb2enr = (volatile uint32_t *)(RCC_BASE + 0x4C);
    *ahb2enr |= (1U << 0) | (1U << 1) | (1U << 2) | (1U << 5);

    /* Enable SPI1, USART1, ADC, TIM1 clocks (APB2ENR at 0x60, APB1ENR1 at 0x58) */
    volatile uint32_t *apb2enr = (volatile uint32_t *)(RCC_BASE + 0x60);
    *apb2enr |= (1U << 12)  /* SPI1 */
              | (1U << 14)  /* USART1 */
              | (1U << 11)  /* TIM1 */
              | (1U << 8)   /* ADC */
              | (1U << 20); /* CORDIC */

    volatile uint32_t *apb1enr1 = (volatile uint32_t *)(RCC_BASE + 0x58);
    *apb1enr1 |= (1U << 16) /* USART3 */
              |  (1U << 4)  /* TIM6 */
              |  (1U << 5)  /* TIM7 */
              |  (1U << 15); /* SPI3 (on APB1) */

    /* CORDIC + FMAC on AHB1 */
    volatile uint32_t *ahb1enr = (volatile uint32_t *)(RCC_BASE + 0x48);
    *ahb1enr |= (1U << 21)  /* FMAC */
              | (1U << 20); /* CORDIC (AHB1) — actually check ref manual */

    /* --- Configure pins --- */

    /* PA0-PA3: analog (ADC inputs) */
    GPIOA->MODER |= (GPIO_MODER_ANALOG << 0)  |   /* PA0 */
                     (GPIO_MODER_ANALOG << 2)  |   /* PA1 */
                     (GPIO_MODER_ANALOG << 4)  |   /* PA2 */
                     (GPIO_MODER_ANALOG << 6);    /* PA3 */

    /* PA4: analog (DAC output) */
    GPIOA->MODER |= (GPIO_MODER_ANALOG << 8);

    /* PA5: output (SPI1 NCS for ADS1256) — start high (deselected) */
    GPIOA->MODER |= (GPIO_MODER_OUTPUT << 10);
    GPIOA->ODR |= (1U << PIN_SPI1_NCS_ADC);

    /* PA6, PA7: AF (SPI1 MISO, MOSI) */
    GPIOA->MODER |= (GPIO_MODER_AF << 12) | (GPIO_MODER_AF << 14);
    GPIOA->AFRL |= (5U << 24) | (5U << 28);  /* AF5 = SPI1 */

    /* PA8: AF (TIM1_CH1 for DCIR pulse gate) */
    GPIOA->MODER |= (GPIO_MODER_AF << 16);
    GPIOA->AFRL |= (6U << 0);  /* AF6 = TIM1 */

    /* PA9, PA10: AF (USART1 TX, RX) */
    GPIOA->MODER |= (GPIO_MODER_AF << 18) | (GPIO_MODER_AF << 20);
    GPIOA->AFRL |= (7U << 4) | (7U << 8);  /* AF7 = USART1 */

    /* PA11, PA12: AF (USB) */
    GPIOA->MODER |= (GPIO_MODER_AF << 22) | (GPIO_MODER_AF << 24);
    GPIOA->AFRL |= (10U << 12) | (10U << 16);  /* AF10 = USB */

    /* PA15: output (SPI3 NCS for DDS) — start high */
    GPIOA->MODER |= (GPIO_MODER_OUTPUT << 30);
    GPIOA->ODR |= (1U << PIN_SPI3_NCS_DDS);

    /* PB0, PB1: output (DDS SPI bit-bang: SCK, MOSI) */
    GPIOB->MODER |= (GPIO_MODER_OUTPUT << 0) | (GPIO_MODER_OUTPUT << 2);

    /* PB3-PB8: output (LEDs) */
    for (int i = 3; i <= 8; i++) {
        GPIOB->MODER |= (GPIO_MODER_OUTPUT << (i * 2));
    }
    /* All LEDs off initially */
    GPIOB->ODR &= ~((1U << 3) | (1U << 4) | (1U << 5) | (1U << 6) |
                    (1U << 7) | (1U << 8));

    /* PB9: input with pull-up (button, active low) */
    GPIOB->MODER &= ~(0x3U << 18);
    GPIOB->PUPDR |= (GPIO_PULL_UP << 18);

    /* PB12: output (ANALOG_EN — analog rail power) */
    GPIOB->MODER |= (GPIO_MODER_OUTPUT << 24);
    GPIOB->ODR &= ~(1U << PIN_ANALOG_EN);  /* off initially */

    /* PB13: output (DDS reset) */
    GPIOB->MODER |= (GPIO_MODER_OUTPUT << 26);

    /* PB14: input (ADS DRDY) */
    GPIOB->MODER &= ~(0x3U << 28);
    GPIOB->PUPDR |= (GPIO_PULL_UP << 28);

    /* PB15: input (supercap OK) */
    GPIOB->MODER &= ~(0x3U << 30);

    /* PC0, PC1: analog (NTC, VBUS) */
    GPIOC->MODER |= (GPIO_MODER_ANALOG << 0) | (GPIO_MODER_ANALOG << 2);

    /* PC2: input (OVP fault) */
    GPIOC->MODER &= ~(0x3U << 4);

    /* PC3: input (reverse polarity) */
    GPIOC->MODER &= ~(0x3U << 6);

    /* PC4-PC12: outputs (BLE control, cal switch, USB VBUS, charge, fault) */
    for (int i = 4; i <= 12; i++) {
        if (i == 10) continue;  /* PC10 reserved */
        GPIOC->MODER |= (GPIO_MODER_OUTPUT << (i * 2));
    }

    /* BLE module in reset initially (hold nRESET_BLE low) */
    GPIOC->ODR &= ~(1U << PIN_nRESET_BLE);
}

/* -------------------------------------------------------------------------
 * SysTick init — 1 ms tick from 163.84 MHz / 8 = 20.48 MHz → reload = 20480
 * ------------------------------------------------------------------------- */
static void systick_init(void)
{
    SysTick->LOAD = 20480U - 1U;
    SysTick->VAL = 0;
    SysTick->CTRL = SysTick_CTRL_ENABLE | SysTick_CTRL_TICKINT |
                    SysTick_CTRL_CLKSOURCE;
}

/* -------------------------------------------------------------------------
 * Enter STOP2 low-power mode
 *
 * In STOP2: the MCU core, most peripherals, and the analog rails are off.
 * Wake sources: RTC wake timer, button (EXTI), BLE CTS edge.
 *
 * Author: jayis1
 * ------------------------------------------------------------------------- */
static void enter_stop2(void)
{
    /* Power down analog rails */
    power_analog_off();

    /* Configure PWR for STOP2 */
    PWR->CR1 &= ~0x7U;
    PWR->CR1 |= PWR_CR1_LPMS_STOP2;

    /* Set SLEEPDEEP bit */
    SCB_SCR |= SCB_SCR_SLEEPDEEP;

    /* Wait for interrupt — this enters STOP2 */
    __asm volatile ("wfi");

    /* --- Woken up --- */
    SCB_SCR &= ~SCB_SCR_SLEEPDEEP;

    /* Re-init clocks (HSI is used temporarily after STOP2 wake, then switch
       back to PLL) */
    RCC->CR |= RCC_CR_HSION;
    while (!(RCC->CR & RCC_CR_HSIRDY)) { }
    RCC->CFGR = (RCC->CFGR & ~0x3U) | RCC_CFGR_SW_HSI;
    /* Re-enable PLL */
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY)) { }
    RCC->CFGR = (RCC->CFGR & ~0x3U) | RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS_MASK) != RCC_CFGR_SWS_PLL) { }
    RCC->CR &= ~RCC_CR_HSION;

    /* Power up analog rails (will be done by measurement start) */
}

/* -------------------------------------------------------------------------
 * Button handling — debounce + long press detection
 * ------------------------------------------------------------------------- */
static void handle_button(void)
{
    static uint32_t last_read = 0;
    uint32_t now = millis();

    /* Read button (PB9, active low) */
    uint8_t btn_state = (GPIOB->IDR & (1U << PIN_BTN_USER)) ? 0 : 1;

    if (btn_state && !g_btn_held) {
        /* Button just pressed */
        if ((now - last_read) > DEBOUNCE_MS) {
            g_btn_held = 1;
            g_btn_press_ts = now;
            g_btn_long_press = 0;
        }
    } else if (btn_state && g_btn_held) {
        /* Still held — check for long press */
        if (!g_btn_long_press && (now - g_btn_press_ts) > LONG_PRESS_MS) {
            g_btn_long_press = 1;
            /* Start full sweep immediately on long-press detection */
            if (g_state == STATE_IDLE) {
                g_state = STATE_SWEEP_FULL;
            }
        }
    } else if (!btn_state && g_btn_held) {
        /* Button released */
        g_btn_held = 0;
        uint32_t held_duration = now - g_btn_press_ts;

        if (held_duration < LONG_PRESS_MS && g_state == STATE_IDLE) {
            /* Short press — start fast sweep */
            g_state = STATE_SWEEP_FAST;
        } else if (g_state == STATE_SWEEP_FAST ||
                   g_state == STATE_SWEEP_FULL ||
                   g_state == STATE_DCIR_MEASURE ||
                   g_state == STATE_OCV_RELAX) {
            /* Press during measurement = abort */
            g_state = STATE_IDLE;
        }
        last_read = now;
    }
}

/* -------------------------------------------------------------------------
 * Run a complete measurement cycle
 *
 * Steps:
 *   1. Power up analog rails, check safety (voltage, temp, polarity)
 *   2. Measure OCV
 *   3. Auto-detect chemistry (if enabled)
 *   4. Run EIS sweep (fast or full)
 *   5. Run DCIR pulse
 *   6. OCV relaxation capture (self-discharge estimate)
 *   7. CNLS fit on the Randles model
 *   8. SoH scoring + degradation classification
 *   9. Report via BLE + USB, store in flash
 *  10. Power down analog rails
 *
 * Author: jayis1
 * ------------------------------------------------------------------------- */
static void run_measurement_cycle(uint8_t full)
{
    uint16_t ocv_mv;
    uint16_t temp_dc;
    uint8_t  chem_idx;
    int      ret;

    /* Step 1: power up and safety check */
    power_analog_on();
    delay_ms(50);  /* let analog rails settle */

    ret = safety_check(&ocv_mv, &temp_dc);
    if (ret != SAFETY_OK) {
        if (ret == SAFETY_REVERSE_POLARITY) {
            fault_handler("REVERSE POLARITY");
        } else if (ret == SAFETY_OVP) {
            fault_handler("OVER VOLTAGE");
        } else if (ret == SAFETY_UVP) {
            fault_handler("UNDER VOLTAGE");
        } else if (ret == SAFETY_OVERTEMP) {
            fault_handler("OVER TEMP");
        }
        power_analog_off();
        g_state = STATE_FAULT;
        return;
    }

    /* Step 2: OCV already measured by safety_check */
    g_last_result.ocv_mv = ocv_mv;
    g_last_result.temp_dc = temp_dc;

    /* Step 3: auto-detect chemistry from OCV */
    if (g_config.auto_chemistry) {
        if (ocv_mv >= 3250 && ocv_mv <= 3380) {
            chem_idx = 2;  /* LFP: 3.30 V plateau */
        } else if (ocv_mv >= 3550 && ocv_mv <= 3650) {
            chem_idx = 3;  /* NCA: 3.60 V */
        } else if (ocv_mv >= 3650 && ocv_mv <= 3800) {
            chem_idx = 0;  /* NMC: 3.70 V */
        } else {
            chem_idx = 0;  /* default to NMC 18650 */
        }
    } else {
        chem_idx = g_config.chemistry;
    }
    g_last_result.chemistry_idx = chem_idx;

    /* Initialize DDS and ADC for the sweep */
    dds_init();
    ads1256_init();

    /* Step 4: EIS sweep */
    g_state = full ? STATE_SWEEP_FULL : STATE_SWEEP_FAST;
    g_sweep_freq_count = full ? EIS_FULL_POINT_COUNT : EIS_FAST_POINT_COUNT;
    g_sweep_freq_idx = 0;

    ret = eis_sweep_run(full, &g_last_result.sweep_data);
    if (ret != EIS_OK) {
        fault_handler("EIS SWEEP FAILED");
        power_analog_off();
        g_state = STATE_FAULT;
        return;
    }

    /* Step 5: DCIR measurement */
    g_state = STATE_DCIR_MEASURE;
    ret = dcir_measure(&g_last_result.dcir_mohm);
    if (ret != DCIR_OK) {
        /* DCIR failure is non-fatal — continue with EIS data only */
        g_last_result.dcir_mohm = 0;
    }

    /* Step 6: OCV relaxation (30-second window for self-discharge rate) */
    g_state = STATE_OCV_RELAX;
    {
        uint16_t ocv_start = g_last_result.ocv_mv;
        uint16_t ocv_end;
        delay_ms(30000);  /* 30-second relaxation */
        safety_read_voltage(&ocv_end);
        /* Self-discharge rate in µV/min (negative = voltage dropping) */
        int32_t dv = (int32_t)ocv_end - (int32_t)ocv_start;
        g_last_result.self_discharge_uv_per_min = dv * 2;  /* 30s → 1min ×2 */
    }

    /* Step 7: CNLS fit */
    g_state = STATE_CNLS_FIT;
    randles_params_t fitted;
    ret = cnls_fit(&g_last_result.sweep_data, &fitted);
    if (ret == CNLS_OK) {
        g_last_result.randles = fitted;
        g_last_result.fit_valid = 1;
    } else {
        g_last_result.fit_valid = 0;
        /* CNLS failed — use raw EIS data for a rough SoH estimate */
    }

    /* Step 8: SoH scoring */
    g_state = STATE_REPORT;
    soh_compute(&g_last_result, &chemistry_table[chem_idx]);

    /* Step 9: report + store */
    g_result_valid = 1;
    ble_send_result(&g_last_result);
    usb_send_result(&g_last_result);
    storage_save_result(&g_last_result);

    /* Step 10: power down */
    power_analog_off();

    /* Update LEDs to show verdict */
    update_leds();

    /* Return to idle after a display period */
    delay_ms(5000);
    g_state = STATE_IDLE;
}

/* -------------------------------------------------------------------------
 * Fast / full sweep wrappers
 * ------------------------------------------------------------------------- */
static void run_fast_sweep(void)
{
    run_measurement_cycle(0);
}

static void run_full_sweep(void)
{
    run_measurement_cycle(1);
}

/* -------------------------------------------------------------------------
 * LED update based on state and last result
 * ------------------------------------------------------------------------- */
static void update_leds(void)
{
    /* Clear all LEDs */
    GPIOB->ODR &= ~((1U << PIN_LED_STATUS1) | (1U << PIN_LED_STATUS2) |
                    (1U << PIN_LED_STATUS3) | (1U << PIN_LED_STATUS4) |
                    (1U << PIN_LED_GOOD) | (1U << PIN_LED_BAD));

    switch (g_state) {
    case STATE_IDLE:
        GPIOB->ODR |= (1U << PIN_LED_STATUS1);  /* LED1: idle/ready */
        break;
    case STATE_SWEEP_FAST:
    case STATE_SWEEP_FULL:
    case STATE_DCIR_MEASURE:
    case STATE_OCV_RELAX:
        GPIOB->ODR |= (1U << PIN_LED_STATUS2);  /* LED2: sweeping */
        break;
    case STATE_CNLS_FIT:
        GPIOB->ODR |= (1U << PIN_LED_STATUS2) | (1U << PIN_LED_STATUS3);
        break;
    case STATE_REPORT:
        if (g_result_valid) {
            GPIOB->ODR |= (1U << PIN_LED_STATUS3);  /* LED3: done */
            if (g_last_result.soh_score >= 70) {
                GPIOB->ODR |= (1U << PIN_LED_GOOD);  /* green */
            } else if (g_last_result.soh_score >= 40) {
                GPIOB->ODR |= (1U << PIN_LED_GOOD) | (1U << PIN_LED_BAD);
            } else {
                GPIOB->ODR |= (1U << PIN_LED_BAD);   /* red */
            }
        }
        break;
    case STATE_FAULT:
        GPIOB->ODR |= (1U << PIN_LED_STATUS4) | (1U << PIN_LED_BAD);
        break;
    case STATE_SLEEP:
        /* All off */
        break;
    default:
        break;
    }
}

/* -------------------------------------------------------------------------
 * Fault handler
 * ------------------------------------------------------------------------- */
static void fault_handler(const char *msg)
{
    g_fault_flag = 1;
    g_state = STATE_FAULT;
    ble_send_error(msg);
    update_leds();

    /* Latch the hardware fault and wait for button to clear */
    delay_ms(2000);
    g_state = STATE_IDLE;
    g_fault_flag = 0;
}

/* -------------------------------------------------------------------------
 * BLE periodic handler — service incoming commands
 * ------------------------------------------------------------------------- */
static void ble_service(void)
{
    uint8_t cmd;
    if (ble_get_command(&cmd) == BLE_OK) {
        switch (cmd) {
        case BLE_CMD_START_FAST:
            if (g_state == STATE_IDLE)
                g_state = STATE_SWEEP_FAST;
            break;
        case BLE_CMD_START_FULL:
            if (g_state == STATE_IDLE)
                g_state = STATE_SWEEP_FULL;
            break;
        case BLE_CMD_ABORT:
            if (g_state == STATE_SWEEP_FAST || g_state == STATE_SWEEP_FULL)
                g_state = STATE_IDLE;
            break;
        case BLE_CMD_GET_STATUS:
            ble_send_status(g_state, g_sweep_progress, g_result_valid);
            break;
        case BLE_CMD_GET_RESULT:
            if (g_result_valid)
                ble_send_result(&g_last_result);
            break;
        case BLE_CMD_GET_HISTORY:
            storage_send_history_ble();
            break;
        case BLE_CMD_SET_CONFIG:
            ble_receive_config(&g_config);
            break;
        case BLE_CMD_CALIBRATE:
            /* Run calibration routine */
            break;
        default:
            break;
        }
    }
}

/* -------------------------------------------------------------------------
 * Main
 * ------------------------------------------------------------------------- */
int main(void)
{
    /* Boot: initialize hardware */
    clock_init();
    gpio_init();
    systick_init();

    /* Initialize drivers */
    power_init();
    safety_init();
    ble_init(&g_config);
    usb_init();
    storage_init();

    /* Load config from flash (or use defaults if first boot) */
    storage_load_config(&g_config);

    /* Release BLE module from reset */
    delay_ms(10);
    GPIOC->ODR |= (1U << PIN_nRESET_BLE);

    /* Brief power-on self-test: flash all LEDs */
    for (int i = 3; i <= 8; i++) {
        GPIOB->ODR |= (1U << i);
    }
    delay_ms(200);
    for (int i = 3; i <= 8; i++) {
        GPIOB->ODR &= ~(1U << i);
    }

    g_state = STATE_IDLE;
    update_leds();

    /* ---------------------------------------------------------------------
     * Main super-loop
     * ------------------------------------------------------------------- */
    uint32_t last_idle_ts = millis();
    uint32_t last_led_ts = millis();

    for (;;) {
        /* Service BLE and USB every iteration */
        ble_service();
        usb_service();

        /* Handle button */
        handle_button();

        /* State machine */
        switch (g_state) {
        case STATE_IDLE:
            /* If idle for > 60 seconds and no BLE connection, go to sleep */
            if ((millis() - last_idle_ts) > 60000U && !ble_is_connected()) {
                g_state = STATE_SLEEP;
            }
            break;

        case STATE_SWEEP_FAST:
            run_fast_sweep();
            last_idle_ts = millis();
            break;

        case STATE_SWEEP_FULL:
            run_full_sweep();
            last_idle_ts = millis();
            break;

        case STATE_FAULT:
            fault_handler("FAULT");
            last_idle_ts = millis();
            break;

        case STATE_SLEEP:
            update_leds();
            enter_stop2();
            /* Woken up — back to idle */
            g_state = STATE_IDLE;
            last_idle_ts = millis();
            break;

        default:
            /* Sub-states (DCIR_MEASURE, OCV_RELAX, CNLS_FIT, REPORT) are
               handled inside run_measurement_cycle() and will transition
               back to IDLE when done. */
            break;
        }

        /* Update LEDs at 10 Hz */
        if ((millis() - last_led_ts) > 100U) {
            update_leds();
            last_led_ts = millis();
        }
    }

    /* Never reached */
    return 0;
}