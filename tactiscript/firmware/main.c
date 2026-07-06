/*
 * main.c — TactiScript firmware main entry point
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 *
 * TactiScript is a wearable fingertip haptic display ring that renders
 * Braille text, tactile graphics, and navigation cues using a 4×6 array
 * of piezoelectric bimorph micro-actuators.
 *
 * This file implements:
 *   - System initialization (clocks, GPIO, peripherals)
 *   - Main scheduler / super-loop
 *   - Device mode state machine (Reader, Navigation, Music, Tutor, Texture)
 *   - Inter-module coordination
 *   - Low-power management
 */

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "board.h"
#include "registers.h"

/* Driver headers */
#include "drivers/actuator_drv.h"
#include "drivers/braille.h"
#include "drivers/imu_drv.h"
#include "drivers/ble_svc.h"
#include "drivers/oled_drv.h"
#include "drivers/power.h"
#include "drivers/haptic_render.h"

/* ----------------------------------------------------------------
 * Device mode enumeration
 * ---------------------------------------------------------------- */
typedef enum {
    MODE_INIT = 0,
    MODE_READER,
    MODE_NAVIGATION,
    MODE_MUSIC,
    MODE_TUTOR,
    MODE_TEXTURE,
    MODE_SLEEP,
    MODE_COUNT
} device_mode_t;

/* ----------------------------------------------------------------
 * Global system state
 * ---------------------------------------------------------------- */
typedef struct {
    device_mode_t mode;
    device_mode_t prev_mode;        /* for wake-from-sleep restore */

    /* Reader mode */
    char    text_ring[2048];         /* circular text buffer */
    uint16_t text_head;             /* write index */
    uint16_t text_tail;             /* read index (next char to render) */
    bool    text_playing;           /* streaming active */
    uint16_t reader_char_delay_ms; /* delay between Braille chars */

    /* General settings */
    uint8_t drive_intensity;        /* 0-100 % of HV voltage */
    uint8_t scroll_speed;           /* 1 (slow) to 10 (fast) */
    uint8_t braille_grade;          /* 1 or 2 (UEB) */
    bool    skin_contact;          /* ring on finger? */
    int8_t  temperature_c;         /* NTC reading for PZT compensation */

    /* Navigation mode */
    uint8_t nav_direction;         /* current nav vector (0=stop, 1-8=dir) */

    /* Status */
    uint8_t battery_pct;
    bool    charging;
    bool    ble_connected;
    bool    error_flag;
    uint32_t uptime_s;
} system_state_t;

static system_state_t g_state;

/* Pointer to the frame being rendered (actuator driver reads this).
 * 4×6 array of uint8_t values 0-255 representing drive intensity per element.
 */
static uint8_t g_frame_a[ACT_COUNT];
static uint8_t g_frame_b[ACT_COUNT];
static volatile uint8_t *g_render_frame = g_frame_a;
static volatile uint8_t *g_prepare_frame = g_frame_b;

/* ----------------------------------------------------------------
 * Forward declarations
 * ---------------------------------------------------------------- */
static void system_init(void);
static void clocks_init(void);
static void gpio_init(void);
static void timers_init(void);
static void scheduler_run(void);
static void mode_enter(device_mode_t mode);
static void mode_reader_tick(void);
static void mode_navigation_tick(void);
static void mode_music_tick(void);
static void mode_tutor_tick(void);
static void mode_texture_tick(void);
static void handle_ble_command(uint8_t cmd);
static void handle_gesture(int gesture);
static void update_status_display(void);
static void swap_frames(void);
static void check_battery(void);
static void enter_sleep(void);
static void handle_error(const char *msg);

/* ----------------------------------------------------------------
 * Application entry
 * ---------------------------------------------------------------- */
int main(void)
{
    /* Initialize all hardware and drivers */
    system_init();

    /* Default mode: Reader */
    mode_enter(MODE_READER);

    /* Main super-loop */
    scheduler_run();

    /* Should never reach here */
    for (;;) ;
}

/* ----------------------------------------------------------------
 * System initialization
 * ---------------------------------------------------------------- */
static void system_init(void)
{
    memset(&g_state, 0, sizeof(g_state));
    g_state.mode = MODE_INIT;
    g_state.drive_intensity = 80;     /* default 80% drive */
    g_state.scroll_speed = 5;          /* default mid speed */
    g_state.braille_grade = 1;         /* default UEB Grade 1 */
    g_state.text_playing = false;
    g_state.reader_char_delay_ms = 400; /* 400ms per char default */

    clocks_init();
    gpio_init();
    timers_init();

    /* Initialize drivers (order matters: actuator needs HV rail stable) */
    power_init();
    oled_init();
    oled_clear();
    oled_draw_string(0, 0, "TactiScript");
    oled_draw_string(0, 1, "by jayis1");
    oled_display_flush();

    actuator_init();
    imu_init();
    braille_init();
    haptic_render_init();
    ble_init();

    /* Enable global interrupts */
    __asm volatile ("cpsie i");

    /* Initial battery check */
    g_state.battery_pct = power_read_battery_pct();
    g_state.charging = power_is_charging();

    /* Brief startup haptic pulse: full-array buzz for 200 ms */
    memset((void *)g_prepare_frame, 255, ACT_COUNT);
    swap_frames();
    actuator_refresh_enable(true);
    power_delay_ms(200);
    actuator_refresh_enable(false);
    memset((void *)g_prepare_frame, 0, ACT_COUNT);

    /* Clear OLED and show status */
    update_status_display();
}

/* ----------------------------------------------------------------
 * Clock initialization
 * ---------------------------------------------------------------- */
static void clocks_init(void)
{
    /* Start LFCLK (32.768 kHz external crystal) for RTC + BLE */
    NRF_CLOCK_LFCLKSRC = 0x01;  /* external crystal */
    NRF_CLOCK_TASKS_LFCLKSTART = 1;
    while (!NRF_CLOCK_EVENTS_LFCLKSTARTED) ;
    NRF_CLOCK_EVENTS_LFCLKSTARTED = 0;

    /* Start HFCLK (32 MHz external crystal) for radio + SPI */
    NRF_CLOCK_TASKS_HFCLKSTART = 1;
    while (!NRF_CLOCK_EVENTS_HFCLKSTARTED) ;
    NRF_CLOCK_EVENTS_HFCLKSTARTED = 0;
}

/* ----------------------------------------------------------------
 * GPIO initialization
 * ---------------------------------------------------------------- */
static void gpio_init(void)
{
    /* Configure LED outputs */
    GPIO_DIRSET(NRF_GPIO_P1_BASE) = (1u << 10) | (1u << 11); /* LED R, G */
    GPIO_OUTCLR(NRF_GPIO_P1_BASE) = (1u << 10) | (1u << 11); /* off */

    /* Configure HV control pins as outputs, default low */
    GPIO_DIRSET(NRF_GPIO_P1_BASE) = (1u << 0);   /* HV_BOOST_EN */
    GPIO_DIRSET(NRF_GPIO_P1_BASE) = (1u << 1);    /* HV_GATE */
    GPIO_DIRSET(NRF_GPIO_P1_BASE) = (1u << 2);   /* DEMUX_A0 */
    GPIO_DIRSET(NRF_GPIO_P1_BASE) = (1u << 3);   /* DEMUX_A1 */
    GPIO_DIRSET(NRF_GPIO_P1_BASE) = (1u << 4);   /* DEMUX_A2 */
    GPIO_DIRSET(NRF_GPIO_P1_BASE) = (1u << 5);   /* DRV_LATCH */
    GPIO_OUTCLR(NRF_GPIO_P1_BASE) = (1u << 0);
    GPIO_OUTCLR(NRF_GPIO_P1_BASE) = (1u << 1);

    /* LRA control */
    GPIO_DIRSET(NRF_GPIO_P1_BASE) = (1u << 6) | (1u << 7);
    GPIO_OUTCLR(NRF_GPIO_P1_BASE) = (1u << 6);

    /* Inputs: charge detect, USB detect, status */
    GPIO_DIRCLR(NRF_GPIO_P0_BASE) = (1u << 28) | (1u << 29) | (1u << 31);

    /* Configure IMU interrupt pins as input */
    GPIO_DIRCLR(NRF_GPIO_P0_BASE) = (1u << 19) | (1u << 20);
}

/* ----------------------------------------------------------------
 * Timer initialization
 *   - TIMER0: 200 Hz actuator refresh interrupt
 *   - RTC0: 1 kHz system tick
 *   - PWM0: LRA vibration motor drive
 * ---------------------------------------------------------------- */
static void timers_init(void)
{
    /* TIMER0: 200 Hz refresh, 16 MHz / 16 = 1 MHz timer */
    TIMER_MODE(NRF_TIMER0_BASE) = 0;   /* timer mode */
    TIMER_BITMODE(NRF_TIMER0_BASE) = 2; /* 16-bit */
    TIMER_PRESCALER(NRF_TIMER0_BASE) = 4; /* /16 = 1 MHz */
    TIMER_CC(NRF_TIMER0_BASE, 0) = ACT_REFRESH_US; /* 5000 µs = 200 Hz */
    TIMER_INTENSET(NRF_TIMER0_BASE) = (1u << 16); /* CC[0] interrupt */

    /* RTC0: 1 kHz system tick (LFCLK / 1 = 32.768 kHz, prescaler 32 ≈ 1.024 kHz) */
    RTC_PRESCALER(NRF_RTC0_BASE) = 31; /* /32 → ~1.024 kHz */
    RTC_INTENSET(NRF_RTC0_BASE) = (1u << 0); /* TICK interrupt */
    RTC_TASKS_START(NRF_RTC0_BASE) = 1;

    /* PWM0: LRA at 170 Hz resonant frequency */
    PWM_ENABLE(NRF_PWM0_BASE) = 1;
    PWM_COUNTERTOP(NRF_PWM0_BASE) = 1000; /* 16 MHz / 1000 = 16 kHz carrier */
}

/* ----------------------------------------------------------------
 * Main scheduler — super-loop with cooperative task scheduling
 * ---------------------------------------------------------------- */
static void scheduler_run(void)
{
    uint32_t last_status_update = 0;
    uint32_t last_battery_check = 0;
    uint32_t last_tick = 0;

    while (1) {
        uint32_t now = g_state.uptime_s;

        /* 1. Process BLE events (non-blocking, runs in net core but we poll) */
        ble_poll();

        /* 2. Process IMU gestures (non-blocking) */
        int gesture = imu_get_gesture();
        if (gesture != IMU_GESTURE_NONE) {
            handle_gesture(gesture);
        }

        /* 3. Run mode-specific tick */
        switch (g_state.mode) {
        case MODE_READER:     mode_reader_tick();     break;
        case MODE_NAVIGATION: mode_navigation_tick(); break;
        case MODE_MUSIC:      mode_music_tick();      break;
        case MODE_TUTOR:      mode_tutor_tick();     break;
        case MODE_TEXTURE:    mode_texture_tick();    break;
        case MODE_SLEEP:      /* nothing — wait for wake interrupt */ break;
        default: break;
        }

        /* 4. Periodic status update (every 2 s) */
        if (now - last_status_update >= 2) {
            update_status_display();
            last_status_update = now;
        }

        /* 5. Periodic battery check (every 30 s) */
        if (now - last_battery_check >= 30) {
            check_battery();
            last_battery_check = now;
        }

        /* 6. Idle: enter WFE to save power until next interrupt */
        __asm volatile ("wfe");
    }
}

/* ----------------------------------------------------------------
 * Mode management
 * ---------------------------------------------------------------- */
static void mode_enter(device_mode_t mode)
{
    /* Tear down current mode */
    if (g_state.mode == MODE_SLEEP && mode != MODE_SLEEP) {
        /* Waking from sleep */
        actuator_init();
        imu_init();
    }

    g_state.prev_mode = g_state.mode;
    g_state.mode = mode;

    switch (mode) {
    case MODE_READER:
        g_state.text_playing = true;
        actuator_refresh_enable(true);
        break;

    case MODE_NAVIGATION:
        g_state.nav_direction = 0;
        actuator_refresh_enable(true);
        break;

    case MODE_MUSIC:
        actuator_refresh_enable(true);
        break;

    case MODE_TUTOR:
        g_state.text_playing = false;
        actuator_refresh_enable(true);
        break;

    case MODE_TEXTURE:
        actuator_refresh_enable(true);
        break;

    case MODE_SLEEP:
        actuator_refresh_enable(false);
        imu_enter_low_power();
        oled_display_off();
        break;

    default:
        break;
    }

    update_status_display();
}

/* ----------------------------------------------------------------
 * Reader mode tick — render next Braille char from ring buffer
 * ---------------------------------------------------------------- */
static void mode_reader_tick(void)
{
    static uint32_t last_render_ms = 0;
    uint32_t now_ms = g_state.uptime_s * 1000;

    if (!g_state.text_playing)
        return;

    if (now_ms - last_render_ms < g_state.reader_char_delay_ms)
        return;

    last_render_ms = now_ms;

    /* Check if there's text in the buffer */
    if (g_state.text_head == g_state.text_tail)
        return; /* buffer empty */

    /* Read one character from the ring buffer */
    char c = g_state.text_ring[g_state.text_tail];
    g_state.text_tail = (g_state.text_tail + 1) % sizeof(g_state.text_ring);

    /* Translate to Braille and render */
    uint8_t braille_pattern = braille_char_to_pattern(c, g_state.braille_grade);

    /* Map 6-dot Braille (2×3) to 4×6 array (centered) */
    memset((void *)g_prepare_frame, 0, ACT_COUNT);
    /* Braille dots layout (standard 6-dot: positions 1-6):
     * 1 4
     * 2 5
     * 3 6
     * Map to 4×6 grid (row 0-3, col 0-5):
     * Place Braille in center columns 2-3, rows 0-2.
     */
    if (braille_pattern & 0x01) g_prepare_frame[0 * 6 + 2] = g_state.drive_intensity;
    if (braille_pattern & 0x02) g_prepare_frame[1 * 6 + 2] = g_state.drive_intensity;
    if (braille_pattern & 0x04) g_prepare_frame[2 * 6 + 2] = g_state.drive_intensity;
    if (braille_pattern & 0x08) g_prepare_frame[0 * 6 + 3] = g_state.drive_intensity;
    if (braille_pattern & 0x10) g_prepare_frame[1 * 6 + 3] = g_state.drive_intensity;
    if (braille_pattern & 0x20) g_prepare_frame[2 * 6 + 3] = g_state.drive_intensity;

    swap_frames();

    /* Scroll effect: brief transition where we shift the pattern */
    /* For simplicity here we just swap; haptic_render adds motion */
}

/* ----------------------------------------------------------------
 * Navigation mode tick — render directional arrow haptically
 * ---------------------------------------------------------------- */
static void mode_navigation_tick(void)
{
    static uint32_t last_render_ms = 0;
    uint32_t now_ms = g_state.uptime_s * 1000;

    if (now_ms - last_render_ms < 800)
        return;
    last_render_ms = now_ms;

    /* Get the current navigation pattern for the direction */
    const uint8_t *pattern = haptic_render_nav_pattern(g_state.nav_direction);
    if (pattern) {
        memcpy((void *)g_prepare_frame, pattern, ACT_COUNT);
        /* Scale by drive intensity */
        for (int i = 0; i < ACT_COUNT; i++) {
            g_prepare_frame[i] = (uint16_t)g_prepare_frame[i] *
                                 g_state.drive_intensity / 100;
        }
        swap_frames();
    }
}

/* ----------------------------------------------------------------
 * Music mode tick — render rhythm/melody haptic pattern
 * ---------------------------------------------------------------- */
static void mode_music_tick(void)
{
    static uint32_t last_render_ms = 0;
    uint32_t now_ms = g_state.uptime_s * 1000;

    /* Music mode renders at a faster cadence (50 ms per frame) */
    if (now_ms - last_render_ms < 50)
        return;
    last_render_ms = now_ms;

    /* Pull a pre-synthesized music haptic frame from the renderer */
    const uint8_t *frame = haptic_render_music_next();
    if (frame) {
        memcpy((void *)g_prepare_frame, frame, ACT_COUNT);
        swap_frames();
    }
}

/* ----------------------------------------------------------------
 * Tutor mode tick — show a Braille char and quiz
 * ---------------------------------------------------------------- */
static void mode_tutor_tick(void)
{
    static uint32_t last_render_ms = 0;
    static uint8_t tutor_char = 'A';
    uint32_t now_ms = g_state.uptime_s * 1000;

    if (now_ms - last_render_ms < 3000)
        return;
    last_render_ms = now_ms;

    /* Render the next letter in sequence */
    uint8_t pattern = braille_char_to_pattern(tutor_char, 1);
    memset((void *)g_prepare_frame, 0, ACT_COUNT);
    if (pattern & 0x01) g_prepare_frame[0 * 6 + 2] = g_state.drive_intensity;
    if (pattern & 0x02) g_prepare_frame[1 * 6 + 2] = g_state.drive_intensity;
    if (pattern & 0x04) g_prepare_frame[2 * 6 + 2] = g_state.drive_intensity;
    if (pattern & 0x08) g_prepare_frame[0 * 6 + 3] = g_state.drive_intensity;
    if (pattern & 0x10) g_prepare_frame[1 * 6 + 3] = g_state.drive_intensity;
    if (pattern & 0x20) g_prepare_frame[2 * 6 + 3] = g_state.drive_intensity;
    swap_frames();

    /* Advance to next letter A-Z, then 0-9 */
    if (tutor_char < 'Z')
        tutor_char++;
    else if (tutor_char == 'Z')
        tutor_char = '0';
    else if (tutor_char < '9')
        tutor_char++;
    else
        tutor_char = 'A';
}

/* ----------------------------------------------------------------
 * Texture mode tick — render custom tactile texture frame
 * ---------------------------------------------------------------- */
static void mode_texture_tick(void)
{
    /* Texture frames are pushed directly via BLE_SVC_TEXTURE;
     * here we just animate them with a slight wave effect. */
    static uint32_t last_render_ms = 0;
    uint32_t now_ms = g_state.uptime_s * 1000;

    if (now_ms - last_render_ms < 100)
        return;
    last_render_ms = now_ms;

    haptic_render_texture_animate((uint8_t *)g_prepare_frame, ACT_COUNT,
                                   g_state.drive_intensity);
    swap_frames();
}

/* ----------------------------------------------------------------
 * BLE command handler
 * ---------------------------------------------------------------- */
static void handle_ble_command(uint8_t cmd)
{
    switch (cmd) {
    case CMD_MODE_READER:   mode_enter(MODE_READER);     break;
    case CMD_MODE_NAV:      mode_enter(MODE_NAVIGATION); break;
    case CMD_MODE_MUSIC:    mode_enter(MODE_MUSIC);      break;
    case CMD_MODE_TUTOR:    mode_enter(MODE_TUTOR);      break;
    case CMD_MODE_TEXTURE:  mode_enter(MODE_TEXTURE);    break;
    case CMD_MODE_SLEEP:    enter_sleep();               break;

    case CMD_SPEED_UP:
        if (g_state.scroll_speed < 10) g_state.scroll_speed++;
        g_state.reader_char_delay_ms = 800 - g_state.scroll_speed * 70;
        break;
    case CMD_SPEED_DOWN:
        if (g_state.scroll_speed > 1) g_state.scroll_speed--;
        g_state.reader_char_delay_ms = 800 - g_state.scroll_speed * 70;
        break;

    case CMD_INTENSITY_UP:
        if (g_state.drive_intensity < 100) g_state.drive_intensity += 10;
        actuator_set_voltage((g_state.drive_intensity * HV_TARGET_V) / 100);
        break;
    case CMD_INTENSITY_DOWN:
        if (g_state.drive_intensity >= 10) g_state.drive_intensity -= 10;
        actuator_set_voltage((g_state.drive_intensity * HV_TARGET_V) / 100);
        break;

    case CMD_BRAILLE_GRADE1: g_state.braille_grade = 1; break;
    case CMD_BRAILLE_GRADE2: g_state.braille_grade = 2; break;

    case CMD_PAUSE:  g_state.text_playing = false; break;
    case CMD_RESUME:  g_state.text_playing = true;  break;
    case CMD_CLEAR:
        g_state.text_head = 0;
        g_state.text_tail = 0;
        memset((void *)g_prepare_frame, 0, ACT_COUNT);
        swap_frames();
        break;

    default:
        handle_error("Unknown BLE command");
        break;
    }
}

/* ----------------------------------------------------------------
 * Gesture handler
 * ---------------------------------------------------------------- */
static void handle_gesture(int gesture)
{
    switch (gesture) {
    case IMU_GESTURE_TAP:
        if (g_state.mode == MODE_READER) {
            /* Advance to next char immediately */
            mode_reader_tick();
        } else if (g_state.mode == MODE_TUTOR) {
            /* Next letter */
            mode_tutor_tick();
        }
        break;

    case IMU_GESTURE_DOUBLE_TAP:
        /* Toggle play/pause */
        g_state.text_playing = !g_state.text_playing;
        if (g_state.text_playing) {
            /* Brief vibration to confirm resume */
            power_lra_pulse(100);
        }
        break;

    case IMU_GESTURE_SWIPE_LEFT:
        /* Previous mode */
        if (g_state.mode > MODE_READER && g_state.mode <= MODE_TEXTURE)
            mode_enter(g_state.mode - 1);
        break;

    case IMU_GESTURE_SWIPE_RIGHT:
        /* Next mode */
        if (g_state.mode >= MODE_READER && g_state.mode < MODE_TEXTURE)
            mode_enter(g_state.mode + 1);
        break;

    case IMU_GESTURE_LONG_PRESS:
        /* Enter sleep mode */
        enter_sleep();
        break;

    default:
        break;
    }

    /* Notify app of gesture via BLE */
    ble_notify_gesture(gesture);
}

/* ----------------------------------------------------------------
 * Status display update
 * ---------------------------------------------------------------- */
static void update_status_display(void)
{
    if (g_state.mode == MODE_SLEEP)
        return; /* OLED is off in sleep */

    oled_clear();

    /* Line 0: mode name */
    const char *mode_name = "?";
    switch (g_state.mode) {
    case MODE_READER:     mode_name = "READ"; break;
    case MODE_NAVIGATION: mode_name = "NAV";  break;
    case MODE_MUSIC:      mode_name = "MUSIC"; break;
    case MODE_TUTOR:      mode_name = "TUTR"; break;
    case MODE_TEXTURE:    mode_name = "TXT";  break;
    case MODE_SLEEP:      mode_name = "SLEEP"; break;
    default: break;
    }
    oled_draw_string(0, 0, mode_name);

    /* Line 1: battery + charging indicator */
    char bat_str[16];
    const char *chg = g_state.charging ? "+" : " ";
    /* Simple integer formatting (no snprintf to keep code portable) */
    bat_str[0] = 'B'; bat_str[1] = ':';
    bat_str[2] = '0' + (g_state.battery_pct / 100) % 10;
    bat_str[3] = '0' + (g_state.battery_pct / 10) % 10;
    bat_str[4] = '0' + g_state.battery_pct % 10;
    bat_str[5] = '%';
    bat_str[6] = chg[0];
    bat_str[7] = g_state.ble_connected ? 'C' : '-';
    bat_str[8] = '\0';
    oled_draw_string(0, 1, bat_str);

    /* Line 2: braille grade + speed */
    char info_str[16];
    info_str[0] = 'G'; info_str[1] = '0' + g_state.braille_grade;
    info_str[2] = ' '; info_str[3] = 'S'; info_str[4] = '0' + g_state.scroll_speed;
    info_str[5] = ' '; info_str[6] = 'I';
    info_str[7] = '0' + (g_state.drive_intensity / 10) % 10;
    info_str[8] = '0' + g_state.drive_intensity % 10;
    info_str[9] = '\0';
    oled_draw_string(0, 2, info_str);

    /* Line 3: uptime */
    char up_str[16];
    uint32_t t = g_state.uptime_s;
    uint32_t h = t / 3600;
    uint32_t m = (t / 60) % 60;
    uint32_t s = t % 60;
    up_str[0] = '0' + (h / 10) % 10;
    up_str[1] = '0' + h % 10;
    up_str[2] = ':';
    up_str[3] = '0' + (m / 10) % 10;
    up_str[4] = '0' + m % 10;
    up_str[5] = ':';
    up_str[6] = '0' + (s / 10) % 10;
    up_str[7] = '0' + s % 10;
    up_str[8] = '\0';
    oled_draw_string(0, 3, up_str);

    oled_display_flush();
}

/* ----------------------------------------------------------------
 * Frame swap (double-buffered, lock-free)
 * ---------------------------------------------------------------- */
static void swap_frames(void)
{
    __asm volatile ("cpsid i"); /* disable interrupts */
    volatile uint8_t *tmp = g_render_frame;
    g_render_frame = g_prepare_frame;
    g_prepare_frame = tmp;
    __asm volatile ("cpsie i"); /* re-enable interrupts */
}

/* ----------------------------------------------------------------
 * Battery check
 * ---------------------------------------------------------------- */
static void check_battery(void)
{
    g_state.battery_pct = power_read_battery_pct();
    g_state.charging = power_is_charging();

    if (g_state.battery_pct < 10 && !g_state.charging) {
        /* Low battery: pulse LRA as warning */
        power_lra_pulse(500);
    }

    /* If critical and not charging, force sleep */
    if (g_state.battery_pct < 3 && !g_state.charging) {
        handle_error("Battery critical — entering sleep");
        enter_sleep();
    }
}

/* ----------------------------------------------------------------
 * Sleep mode
 * ---------------------------------------------------------------- */
static void enter_sleep(void)
{
    mode_enter(MODE_SLEEP);
    /* The main loop will WFE until an interrupt wakes us.
     * IMU tap interrupt or BLE connect will wake. */
}

/* ----------------------------------------------------------------
 * Error handler
 * ---------------------------------------------------------------- */
static void handle_error(const char *msg)
{
    (void)msg;
    g_state.error_flag = true;
    /* Toggle red LED */
    GPIO_OUTSET(NRF_GPIO_P1_BASE) = (1u << 10);
    power_delay_ms(100);
    GPIO_OUTCLR(NRF_GPIO_P1_BASE) = (1u << 10);
}

/* ----------------------------------------------------------------
 * Interrupt service routines
 * ---------------------------------------------------------------- */

/* TIMER0 ISR — 200 Hz actuator refresh */
void TIMER0_IRQHandler(void)
{
    if (TIMER_EVENTS_COMPARE(NRF_TIMER0_BASE, 0)) {
        TIMER_EVENTS_COMPARE(NRF_TIMER0_BASE, 0) = 0;

        /* Clear timer */
        TIMER_TASKS_CLEAR(NRF_TIMER0_BASE) = 1;

        /* Read the current render frame and drive the actuator array */
        actuator_refresh((const uint8_t *)g_render_frame);
    }
}

/* RTC0 ISR — 1 kHz system tick */
void RTC0_IRQHandler(void)
{
    static uint32_t tick_count = 0;

    if (RTC_EVENTS_TICK(NRF_RTC0_BASE)) {
        RTC_EVENTS_TICK(NRF_RTC0_BASE) = 0;
        tick_count++;

        /* Update uptime every 1024 ticks (~1 second at 1.024 kHz) */
        if (tick_count >= 1024) {
            tick_count = 0;
            g_state.uptime_s++;
        }
    }
}

/* ----------------------------------------------------------------
 * Callbacks from BLE service (weak, called by ble_svc.c)
 * ---------------------------------------------------------------- */
void ble_on_text_received(const uint8_t *data, uint16_t len)
{
    /* Append received text to the ring buffer */
    for (uint16_t i = 0; i < len; i++) {
        uint16_t next = (g_state.text_head + 1) % sizeof(g_state.text_ring);
        if (next == g_state.text_tail)
            break; /* buffer full */
        g_state.text_ring[g_state.text_head] = data[i];
        g_state.text_head = next;
    }
}

void ble_on_command_received(uint8_t cmd)
{
    handle_ble_command(cmd);
}

void ble_on_texture_received(const uint8_t *data, uint16_t len)
{
    if (len >= ACT_COUNT) {
        memcpy((void *)g_prepare_frame, data, ACT_COUNT);
        swap_frames();
    }
}

void ble_on_nav_received(uint8_t direction)
{
    g_state.nav_direction = direction;
}

void ble_on_connected(void)
{
    g_state.ble_connected = true;
    GPIO_OUTSET(NRF_GPIO_P1_BASE) = (1u << 11); /* green LED on */
}

void ble_on_disconnected(void)
{
    g_state.ble_connected = false;
    GPIO_OUTCLR(NRF_GPIO_P1_BASE) = (1u << 11); /* green LED off */
}

/* ----------------------------------------------------------------
 * End of main.c
 * Author: jayis1
 * ---------------------------------------------------------------- */