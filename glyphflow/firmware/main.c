/*
 * main.c — GlyphFlow firmware main loop and state machine
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 *
 * Bare-metal super-loop for the GlyphFlow air-handwriting wristband.
 *
 * The device lives in one of these states:
 *
 *   SLEEP      — MCU in STOP2, IMU in wake-on-motion. ~7 µA. Wakes on
 *                 IMU INT1 (motion) or the touch button (EXTI).
 *   IDLE       — MCU awake, IMU at 1 kHz, BLE connected/advertising.
 *                 Waiting for the user to begin a stroke.
 *   CAPTURE    — User is actively writing; IMU samples stream into the
 *                 ring buffer. Segmentation watches for pen-up.
 *   INFER      — A stroke just closed; trajectory is reconstructed and
 *                 classified; result is sent over BLE HID.
 *   GROUP      — After a stroke, we wait PEN_GROUP_MS for a possible
 *                 second stroke of the same character (dot of an i, etc.).
 *
 * The main loop is a tiny cooperative scheduler: each call polls the
 * current state and runs one step. Interrupts (IMU INT1, USART RX, touch
 * EXTI) set flags that the loop consumes.
 */
#include "board.h"
#include "registers.h"
#include "drivers/imu.h"
#include "drivers/trajectory.h"
#include "drivers/tflite.h"
#include "drivers/ble.h"
#include "drivers/haptic.h"
#include "drivers/fuel.h"
#include "drivers/touch.h"
#include <string.h>

/* ---- Global state -------------------------------------------------- */
typedef enum {
    STATE_SLEEP = 0,
    STATE_IDLE,
    STATE_CAPTURE,
    STATE_INFER,
    STATE_GROUP
} glyph_state_t;

static glyph_state_t g_state = STATE_SLEEP;
static volatile uint32_t g_tick_ms = 0;       /* SysTick millisecond counter */
static volatile uint8_t  g_imu_irq = 0;       /* IMU data-ready flag */
static volatile uint8_t  g_touch_irq = 0;     /* touch button flag */
static volatile uint8_t  g_motion_wake = 0;   /* WOM wake flag */

/* Sample ring buffer. We capture into this during CAPTURE. */
static imu_sample_t g_ring[STROKE_MAX_SAMP];
static volatile uint16_t g_ring_count = 0;

/* Active set mask: bit0 lowercase, bit1 uppercase, bit2 digits, bit3 punct,
 * bit4 gestures. */
static uint8_t g_active_set = 0x1F;

/* Stroke grouping: time of last stroke close (ms tick). */
static uint32_t g_last_stroke_close_ms = 0;
#define PEN_GROUP_MS  350

/* Idle timer: go back to SLEEP after this many ms with no motion. */
static uint32_t g_last_motion_ms = 0;

/* Battery telemetry counter. */
static uint32_t g_last_batt_ms = 0;

/* Boot count (persisted to RTC backup register in a real build). */
static uint16_t g_boot_count = 0;

/* ---- Forward declarations ------------------------------------------ */
static void clock_init(void);
static void systick_init(void);
static void gpio_init_all(void);
static void exti_init(void);
static void enter_stop2(void);
static uint16_t read_battery_mv(void);
static void emit_scancode(uint8_t class_idx);
static void segment_step(void);
static void infer_step(void);
static void group_step(void);
static void handle_touch(void);
static void go_idle(void);
static void go_sleep(void);

/* ---- Character map: class index → HID scancode ---------------------- */
/* HID scancodes: a-z = 4..29, 0-9 = 30..39 (1=30 ... 0=39).
 * We map 58 classes: 0-25 = a-z, 26-35 = 0-9, 36-45 = punctuation (10),
 * 46-57 = gestures (12). */
static const uint8_t g_class_to_scancode[N_CLASSES] = {
    /* a-z */  4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,
    /* 0-9 */ 39,30,31,32,33,34,35,36,37,38,
    /* punct: . , ? ! ' " ; : - _ */
    55,54,56,49,52,53,51,50,45,46,
    /* gestures */
    42,  /* backspace */
    40,  /* enter */
    44,  /* space */
    0xE0,/* shift-left modifier (special; handled in emit) */
    43,  /* tab */
    0,   /* undo: app translates to Ctrl+Z */
    0,   /* cut:  app translates to Ctrl+X */
    0,   /* copy: app translates to Ctrl+C */
    0,   /* paste: app translates to Ctrl+V */
    0,   /* select-all: Ctrl+A */
    0,   /* cancel: Esc */
    0    /* confirm: Enter (same as class 47) */
};

/* ---- Reset handler / entry ---------------------------------------- */
int main(void)
{
    clock_init();
    systick_init();
    gpio_init_all();

    /* Enable DWT cycle counter for touch timing and profiling. */
    DWT_CTRL |= DWT_CTRL_CYCCNTENA;
    DWT_CYCCNT = 0;

    /* Bring up peripherals. */
    if (imu_init() != 0) {
        /* IMU not responding: blink error pattern and halt. */
        while (1) { for (volatile int i=0;i<800000;i++){} GPIO_SET(GPIOA,4); for (volatile int i=0;i<800000;i++){} GPIO_CLR(GPIOA,4); }
    }
    imu_configure_wom(&((imu_wom_config_t){ .threshold_mg = 120, .duration_ms = 8 }));

    ble_init();
    ble_advertise_start();
    haptic_init();
    fuel_init();
    touch_init();
    exti_init();

    g_state = STATE_IDLE;
    g_boot_count++;

    /* Main super-loop. */
    while (1) {
        uint32_t now = g_tick_ms;

        /* Consume interrupts. */
        if (g_touch_irq) {
            g_touch_irq = 0;
            handle_touch();
        }

        switch (g_state) {
        case STATE_SLEEP:
            /* We only reach here after go_sleep() set WOM mode and cleared
             * flags; the actual WFI happens in go_sleep(). */
            break;

        case STATE_IDLE:
            if (g_motion_wake || g_imu_irq) {
                g_motion_wake = 0;
                g_imu_irq = 0;
                g_last_motion_ms = now;
                imu_configure_1khz();
                g_ring_count = 0;
                g_state = STATE_CAPTURE;
                haptic_tick();   /* pen-down feel */
            } else {
                /* After SLEEP_TIMEOUT_MS of no motion, return to SLEEP. */
                if ((now - g_last_motion_ms) > SLEEP_TIMEOUT_MS) {
                    go_sleep();
                }
            }
            break;

        case STATE_CAPTURE:
            /* Drain FIFO into the ring buffer. */
            {
                uint8_t got = 0;
                imu_sample_t tmp[32];
                imu_read_fifo(tmp, 32, &got);
                for (uint8_t i = 0; i < got && g_ring_count < STROKE_MAX_SAMP; i++) {
                    g_ring[g_ring_count++] = tmp[i];
                }
                g_last_motion_ms = now;
            }
            segment_step();
            /* If segmentation declared pen-up, move to INFER. */
            if (g_state == STATE_INFER) {
                infer_step();
            }
            break;

        case STATE_INFER:
            /* Already handled inline above. */
            g_state = STATE_GROUP;
            g_last_stroke_close_ms = now;
            break;

        case STATE_GROUP:
            group_step();
            break;
        }

        /* Battery telemetry every 60 s. */
        if ((now - g_last_batt_ms) > 60000) {
            g_last_batt_ms = now;
            uint8_t pct = fuel_percent();
            if (ble_is_connected()) ble_send_battery(pct);
        }
    }
}

/* ---- Clock init: HSI16 → PLL → 80 MHz SYSCLK ---------------------- */
static void clock_init(void)
{
    /* Enable HSI16. */
    RCC_CR |= (1U << 8);   /* HSION */
    while (!(RCC_CR & (1U << 10))) { }  /* HSIRDY */

    /* Flash latency 4 wait states for 80 MHz. */
    FLASH_ACR = (FLASH_ACR & ~FLASH_ACR_LATENCY_MASK) | 4U;
    /* Enable prefetch + cache. */
    FLASH_ACR |= (1U << 8) | (1U << 9) | (1U << 10);

    /* PLLCFGR: source = HSI16, M=1, N=20, R=2 → 16/1*20/2 = 160 MHz? No:
     * we want 80 MHz so N=10: 16/1*10/2 = 80. */
    RCC_PLLCFGR = (1U << 0)   /* PLLSRC = HSI16 */
                | (0U << 4)   /* PLLM = 1 (000 = 1 in L4? actually 000=3) — use RM value */
                | (20U << 8)  /* PLLN = 20 */
                | (0U << 25); /* PLLR = 2 (00 = 2) */
    /* Actually L4 PLLM is encoded as (value-1), so PLLM=1 → 0. */
    RCC_PLLCFGR &= ~(0x7U << 4);
    RCC_PLLCFGR |=  (0U << 4);

    /* Enable PLL. */
    RCC_CR |= (1U << 24);  /* PLLON */
    while (!(RCC_CR & (1U << 25))) { }  /* PLLRDY */

    /* Switch SYSCLK to PLL. */
    RCC_CFGR = (RCC_CFGR & ~0x3U) | (3U << 0);
    while (((RCC_CFGR >> 2) & 0x3) != 3) { }  /* SWS = PLL */
}

/* ---- SysTick: 1 ms interrupt ------------------------------------- */
static void systick_init(void)
{
    SYSTICK_RVR = (SYSCLK_HZ / 1000) - 1;   /* 80 000 - 1 */
    SYSTICK_CVR = 0;
    SYSTICK_CSR = SYSTICK_CSR_ENABLE | SYSTICK_CSR_TICKINT | SYSTICK_CSR_CLKSOURCE;
    NVIC_ISER0 = (1U << 15);  /* SysTick IRQ = -6 → bit 15 in ISER0 */
}

/* ---- GPIO init --------------------------------------------------- */
static void gpio_init_all(void)
{
    RCC_AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN | RCC_AHB2ENR_GPIOCEN;

    /* LED on PA4: output, off. */
    GPIOA->MODER = (GPIOA->MODER & ~(3U << (4*2))) | (GPIO_MODE_OUT << (4*2));
    GPIO_CLR(GPIOA, 4);

    /* Charge / standby LEDs on PB6, PB7: input (open-drain from TP4056). */
    GPIOB->MODER &= ~(3U << (6*2));
    GPIOB->MODER &= ~(3U << (7*2));
    GPIOB->PUPDR |= (1U << (6*2)) | (1U << (7*2));  /* pull-up */

    /* Haptic EN on PA11: output, low. */
    GPIOA->MODER = (GPIOA->MODER & ~(3U << (11*2))) | (GPIO_MODE_OUT << (11*2));
    GPIO_CLR(PIN_HAPTIC_EN_PORT, PIN_HAPTIC_EN_BIT);
}

/* ---- EXTI for IMU INT1 (PB4) and touch (PA8) --------------------- */
static void exti_init(void)
{
    RCC_AHB2ENR |= RCC_AHB2ENR_GPIOBEN | RCC_AHB2ENR_GPIOAEN;

    /* PB4 as input with pull-up (IMU INT1 is push-pull active high). */
    GPIOB->MODER &= ~(3U << (4*2));

    /* Map EXTI4 to PB4. On STM32L4, EXTI4 has a single mux; we set SYSCFG
     * EXTICR1 port B for line 4. We don't have SYSCFG in registers.h, so
     * write directly: SYSCFG base = APB2 + 0x0000. */
    #define SYSCFG_BASE  (APB2_BASE)
    (*(volatile uint32_t *)(SYSCFG_BASE + 0x0C)) =
        ((*(volatile uint32_t *)(SYSCFG_BASE + 0x0C)) & ~(0xFU << (4*4))) | (1U << (4*4));
    EXTI_IMR1 |= (1U << 4);
    EXTI_RTSR1 |= (1U << 4);   /* rising edge */
    NVIC_ISER0 = (1U << 10);   /* EXTI4 IRQ = 10 */

    /* PA8 (touch) as input, EXTI8. EXTI9_5 IRQ = 23. */
    GPIOA->MODER &= ~(3U << (8*2));
    /* Line 8 lives in EXTICR index 2 (offset 0x10), bits [3:0]. Port A = 0. */
    (*(volatile uint32_t *)(SYSCFG_BASE + 0x10)) =
        ((*(volatile uint32_t *)(SYSCFG_BASE + 0x10)) & ~(0xFU << 0)) | (0U << 0);
    EXTI_IMR1 |= (1U << 8);
    EXTI_RTSR1 |= (1U << 8);
    NVIC_ISER0 = (1U << 23);   /* EXTI9_5 IRQ */
}

/* ---- Interrupt handlers ------------------------------------------ */
void SysTick_Handler(void)
{
    g_tick_ms++;
}

void EXTI4_IRQHandler(void)
{
    /* IMU INT1: data-ready or wake-on-motion. */
    if (EXTI_PR1 & (1U << 4)) {
        EXTI_PR1 = (1U << 4);
        if (g_state == STATE_SLEEP) {
            g_motion_wake = 1;
            /* We can't change state here safely in this simple build;
             * the main loop transitions IDLE → CAPTURE on motion_wake. */
            g_state = STATE_IDLE;
            g_last_motion_ms = g_tick_ms;
        } else {
            g_imu_irq = 1;
        }
    }
}

void EXTI9_5_IRQHandler(void)
{
    /* Touch button on PA8 (EXTI line 8). */
    if (EXTI_PR1 & (1U << 8)) {
        EXTI_PR1 = (1U << 8);
        g_touch_irq = 1;
    }
}

/* ---- Segmentation ------------------------------------------------- */
static uint16_t seg_dwell = 0;   /* consecutive low-velocity samples */

static void segment_step(void)
{
    /* Compute wrist velocity magnitude from the most recent sample's
     * gravity-corrected accel. We use a simplified estimate: subtract a
     * running gravity estimate, then magnitude in milli-g. */
    if (g_ring_count == 0) return;
    imu_sample_t *last = &g_ring[g_ring_count - 1];
    const float lsb_g = 4096.0f;
    static float gx = 0, gy = 0, gz = 0;
    const float a = 0.02f;
    float ax = (float)last->ax / lsb_g;
    float ay = (float)last->ay / lsb_g;
    float az = (float)last->az / lsb_g;
    gx = gx*(1-a) + ax*a; gy = gy*(1-a) + ay*a; gz = gz*(1-a) + az*a;
    float dx = (ax - gx) * 1000.0f;   /* milli-g */
    float dy = (ay - gy) * 1000.0f;
    float dz = (az - gz) * 1000.0f;
    float mag = dx*dx + dy*dy + dz*dz;
    /* mag is in (milli-g)^2; compare to threshold squared. */
    if (mag < (float)(PEN_UP_VEL_MPS_X1K * PEN_UP_VEL_MPS_X1K)) {
        seg_dwell++;
        if (seg_dwell >= PEN_UP_DWELL_SAMP && g_ring_count >= STROKE_MIN_SAMP) {
            /* Pen-up declared. */
            g_state = STATE_INFER;
            seg_dwell = 0;
        }
    } else {
        seg_dwell = 0;
    }

    /* Safety cap: if we've filled the ring, force an infer. */
    if (g_ring_count >= STROKE_MAX_SAMP) {
        g_state = STATE_INFER;
    }
}

/* ---- Inference ---------------------------------------------------- */
static void infer_step(void)
{
    if (g_ring_count < STROKE_MIN_SAMP) {
        g_state = STATE_IDLE;
        return;
    }
    trajectory_t traj;
    if (trajectory_reconstruct(g_ring, g_ring_count, &traj) != 0) {
        g_state = STATE_IDLE;
        g_ring_count = 0;
        return;
    }
    trajectory_normalize(&traj);

    inference_result_t res;
    tflite_classify(&traj, &res);

    /* Only emit if confidence is reasonable. */
    if (res.confidence_x1k > 250) {
        emit_scancode((uint8_t)res.pred_class);
        haptic_confirm();
    } else {
        haptic_error();
    }

    /* Reset for next stroke. */
    g_ring_count = 0;
    g_state = STATE_GROUP;
    g_last_stroke_close_ms = g_tick_ms;
}

/* ---- Stroke grouping --------------------------------------------- */
static void group_step(void)
{
    /* If the user started a new stroke within PEN_GROUP_MS, treat it as a
     * continuation; otherwise, stay in GROUP until the window expires, then
     * go IDLE. */
    if (g_ring_count > 0) {
        /* A new stroke has begun (the capture path refilled the ring);
         * go back to CAPTURE so segmentation can close this stroke too. */
        g_state = STATE_CAPTURE;
        return;
    }
    if ((g_tick_ms - g_last_stroke_close_ms) > PEN_GROUP_MS) {
        go_idle();
    }
}

/* ---- Emit a scancode over BLE HID --------------------------------- */
static void emit_scancode(uint8_t class_idx)
{
    if (class_idx >= N_CLASSES) return;
    uint8_t code = g_class_to_scancode[class_idx];
    uint8_t mods = 0;
    if (code == 0xE0) {
        /* Shift gesture: send a no-op report with the shift modifier set,
         * then a key report with shift held. For simplicity we just send
         * shift-left as a modifier-only report. */
        hid_report_t r = { .modifiers = 0x22, .reserved = 0, .keys = {0} };
        ble_send_hid(&r);
        return;
    }
    /* Uppercase if the uppercase set is active and the class is a letter. */
    if ((g_active_set & 0x02) && class_idx < 26) {
        mods |= 0x02;   /* left shift */
    }
    if (ble_is_connected()) {
        ble_send_char(code, mods);
    }
}

/* ---- Touch handling ----------------------------------------------- */
static void handle_touch(void)
{
    uint8_t pressed = touch_debounce();
    if (!pressed) return;

    switch (g_state) {
    case STATE_SLEEP:
        /* Wake into IDLE. */
        go_idle();
        break;
    case STATE_IDLE:
        /* In IDLE, a tap toggles between listening-for-motion and paused.
         * We treat any tap in IDLE as a pen-down trigger that starts capture
         * if motion follows shortly. */
        g_last_motion_ms = g_tick_ms;
        haptic_tick();
        break;
    case STATE_CAPTURE:
        /* Tap during capture forces a pen-up (useful if segmentation misses). */
        g_state = STATE_INFER;
        infer_step();
        break;
    case STATE_GROUP:
        /* Tap during group cancels the pending character. */
        g_state = STATE_IDLE;
        break;
    default:
        break;
    }
}

/* ---- State transitions -------------------------------------------- */
static void go_idle(void)
{
    if (g_state == STATE_SLEEP) {
        /* Re-configure IMU for 1 kHz on wake. */
        imu_configure_1khz();
        ble_wake();
    }
    g_state = STATE_IDLE;
    g_last_motion_ms = g_tick_ms;
    g_ring_count = 0;
    seg_dwell = 0;
}

static void go_sleep(void)
{
    /* Switch IMU to wake-on-motion and put MCU in STOP2. */
    imu_configure_wom(&((imu_wom_config_t){ .threshold_mg = 120, .duration_ms = 8 }));
    ble_sleep();
    g_state = STATE_SLEEP;
    enter_stop2();
    /* On wake (EXTI), the ISR sets g_state = STATE_IDLE. */
}

static void enter_stop2(void)
{
    /* PWR_CR1 LPMS = STOP2 (0b010). */
    PWR_CR1 = (PWR_CR1 & ~0x7U) | 0x2U;
    /* Set SLEEPDEEP. */
    SCB_SCR |= SCB_SCR_SLEEPDEEP;
    /* WFI instruction enters STOP2. */
    __asm volatile ("wfi");
    /* On wake, clear SLEEPDEEP. */
    SCB_SCR &= ~SCB_SCR_SLEEPDEEP;
}

/* ---- Battery voltage via ADC (simplified) ----------------------- */
static uint16_t read_battery_mv(void)
{
    /* In a full build this reads the VBAT divider on PA0. Here we delegate
     * to the MAX17048 fuel gauge for the voltage. */
    return fuel_voltage_mv();
}