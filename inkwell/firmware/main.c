/*
 * main.c — Inkwell smart fountain pen firmware entry point and scheduler
 *
 * This file implements the boot sequence, the cooperative 1 kHz main loop,
 * the power/sleep finite state machine, and the top-level orchestration of
 * the IMU/AHRS/dead-reckoning/stroke pipeline. There is no RTOS: a single
 * 1 kHz ISR driven by the BMI270 data-ready line feeds a ring buffer that
 * the main loop drains; the main loop then runs AHRS, double-integration,
 * stroke segmentation, flash journaling, and BLE notification.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "board.h"
#include "registers.h"
#include "drivers/imu.h"
#include "drivers/pressure.h"
#include "drivers/optflow.h"
#include "drivers/ahrs.h"
#include "drivers/dead_reckon.h"
#include "drivers/stroke.h"
#include "drivers/flashio.h"
#include "drivers/ble_pen.h"
#include "drivers/power.h"
#include "drivers/usb_shell.h"

/* ---- nRF GPIO shim (minimal, defined in board support) ---- */
void nrf_gpio_pin_clear(uint32_t pin)  { (void)pin; }
void nrf_gpio_pin_set(uint32_t pin)    { (void)pin; }
void nrf_gpio_pin_toggle(uint32_t pin) { (void)pin; }
uint32_t nrf_gpio_pin_read(uint32_t pin) { (void)pin; return 1; }
void nrf_gpio_cfg_output(uint32_t pin, uint32_t pull) { (void)pin; (void)pull; }
void nrf_gpio_cfg_input(uint32_t pin, uint32_t pull)  { (void)pin; (void)pull; }

/* ---- Global state ---- */
static volatile uint32_t g_tick_ms;          /* 1 kHz tick counter */
static volatile bool     g_imu_data_ready;    /* set by ISR, cleared by loop */
static power_state_t     g_power_state = PWR_STATE_OFF;
static uint32_t          g_last_motion_ms;    /* for idle timeout */

/* Calibrated pen-lift thresholds (set by calibration routine) */
static uint16_t g_pen_down_mN = DEFAULT_PEN_DOWN_MN;
static uint16_t g_pen_up_mN   = (DEFAULT_PEN_DOWN_MN * DEFAULT_PEN_UP_RATIO) / 100;
static float    g_ahrs_beta   = MADGWICK_BETA_DEFAULT;

/* Per-sample buffers (single-producer single-consumer ring) */
#define IMU_RING_LEN 64
static imu_sample_t  g_imu_ring[IMU_RING_LEN];
static volatile uint32_t g_imu_ring_head;
static volatile uint32_t g_imu_ring_tail;

/* Pressure samples (drained each main-loop tick) */
#define PRES_RING_LEN 16
static pressure_sample_t g_pres_ring[PRES_RING_LEN];
static volatile uint32_t g_pres_head;
static volatile uint32_t g_pres_tail;

/* ---- 1 kHz ISR (BMI270 INT1 rising edge) ----
 * In a full SDK build this is wired to GPIOTE_IN[0]; here it is exposed as
 * a regular function the BSP/port layer calls from the ISR context.
 */
void bmi270_drdy_isr(void)
{
    int32_t n = imu_fifo_drain(&g_imu_ring[g_imu_ring_head],
                               IMU_RING_LEN - g_imu_ring_head);
    if (n > 0) {
        g_imu_ring_head = (g_imu_ring_head + (uint32_t)n) % IMU_RING_LEN;
    }
    g_imu_data_ready = true;
    g_tick_ms++;
}

/* Pressure ADC completion callback (500 Hz) */
void pressure_sample_ready(const pressure_sample_t *s)
{
    g_pres_ring[g_pres_head] = *s;
    g_pres_head = (g_pres_head + 1) % PRES_RING_LEN;
}

/* ---- Public setters used by the USB calibration shell ---- */
void inkwell_set_pen_thresholds(uint16_t down_mN, uint16_t up_mN)
{
    g_pen_down_mN = down_mN;
    g_pen_up_mN   = up_mN;
}

void inkwell_set_ahrs_beta(float beta)
{
    g_ahrs_beta = beta;
    ahrs_set_beta(beta);
}

power_state_t inkwell_get_power_state(void) { return g_power_state; }

/* ---- Idle / sleep transition ---- */
static void enter_sleep(void)
{
    /* In a real build: __WFE(); here we just mark state for the simulator. */
    switch (g_power_state) {
    case PWR_STATE_WRITING:
        /* Stay awake while connected & writing. */
        break;
    case PWR_STATE_CONNECTED_IDLE:
    case PWR_STATE_ADVERTISING:
        /* WFE between ticks keeps avg current low. */
        break;
    default:
        break;
    }
}

/* ---- Cap detection (magnetometer field anomaly) ---- */
static bool pen_is_capped(void)
{
    /* If |B| differs by > 80 µT from baseline, the cap magnet is near. */
    float bx, by, bz;
    imu_read_mag(&bx, &by, &bz);
    float mag = bx*bx + by*by + bz*bz;
    /* Baseline ~ (25 µT)^2 in Earth field; cap magnet dominates. */
    return mag > (100e-6f * 100e-6f); /* 100 µT squared */
}

/* ---- Main pipeline: process one IMU sample through AHRS + integration ---- */
static void process_imu_sample(const imu_sample_t *s)
{
    ahrs_update(s->gyro_radps[0], s->gyro_radps[1], s->gyro_radps[2],
                s->accel_g[0],     s->accel_g[1],     s->accel_g[2],
                s->mag_ut[0],      s->mag_ut[1],      s->mag_ut[2]);

    float q[4];
    ahrs_get_quaternion(q);

    float a_lin[3];
    dead_reckon_update(s->accel_g, q, s->dt_s, a_lin);
}

/* ---- Drain pending pressure samples; update pen-lift FSM ---- */
static void drain_pressure(void)
{
    while (g_pres_tail != g_pres_head) {
        pressure_sample_t p = g_pres_ring[g_pres_tail];
        g_pres_tail = (g_pres_tail + 1) % PRES_RING_LEN;
        pressure_update(p.force_mN, p.ts_ms);
    }
}

/* ---- Periodic 20 ms segment flush to BLE + flash ---- */
static void flush_segments(uint32_t now_ms)
{
    static uint32_t last_flush_ms;
    if ((now_ms - last_flush_ms) < BLE_SEGMENT_PERIOD_MS) return;
    last_flush_ms = now_ms;

    stroke_segment_t seg;
    while (stroke_pop_segment(&seg)) {
        /* 1) persist to flash journal first (source of truth) */
        flashio_append(&seg, sizeof(seg));
        /* 2) notify BLE observers */
        ble_pen_notify_segment(&seg);
    }

    /* Update BLE status characteristic every second */
    static uint32_t last_status_ms;
    if ((now_ms - last_status_ms) >= 1000) {
        last_status_ms = now_ms;
        ble_pen_notify_status(power_get_battery_pct(),
                              g_power_state,
                              flashio_fill_pct());
    }
}

/* ---- Optical flow poll (every 10 ms) ---- */
static void poll_optflow(uint32_t now_ms)
{
    static uint32_t last_of_ms;
    if ((now_ms - last_of_ms) < 10) return;
    last_of_ms = now_ms;

    optflow_sample_t of;
    if (optflow_read(&of)) {
        if (of.squal >= PMW3360_SQUAL_THRESHOLD) {
            dead_reckon_fuse_optflow(of.dx_counts, of.dy_counts, of.squal);
        }
    }
}

/* ---- Main ---- */
int main(void)
{
    /* ---- Boot: bring up peripherals in dependency order ---- */
    nrf_gpio_cfg_output(LED_PIN, 0);
    nrf_gpio_cfg_input(BUTTON_PIN, 1);   /* pull-up */
    LED_ON();  /* solid during boot */

    imu_init();
    pressure_init(pressure_sample_ready);
    optflow_init();
    ahrs_init(g_ahrs_beta, AHRS_SAMPLE_HZ);
    dead_reckon_init(SAMPLE_RATE_HZ);
    stroke_init(g_pen_down_mN, g_pen_up_mN, PEN_LIFT_DEBOUNCE);
    flashio_init();
    ble_pen_init();
    power_init();
    usb_shell_init();

    /* Arm the BMI270 data-ready interrupt (wakes from sleep on motion) */
    imu_enable_drdy_irq(bmi270_drdy_isr);

    g_power_state = PWR_STATE_ADVERTISING;
    g_last_motion_ms = g_tick_ms;
    LED_OFF();

    /* ---- Main loop ---- */
    while (1) {
        uint32_t now_ms = g_tick_ms;

        /* ---- Pen-lift detection drives the whole FSM ---- */
        drain_pressure();
        bool pen_down = pressure_is_pen_down();

        /* ---- Drain IMU ring into the pipeline ---- */
        while (g_imu_ring_tail != g_imu_ring_head) {
            imu_sample_t s = g_imu_ring[g_imu_ring_tail];
            g_imu_ring_tail = (g_imu_ring_tail + 1) % IMU_RING_LEN;

            /* If pen is lifted, feed zero-velocity update to dead-reckoner */
            if (!pen_down) {
                dead_reckon_zupt();
            }

            process_imu_sample(&s);

            /* Feed stroke segmenter with integrated deltas + pressure */
            float dx_um, dy_um;
            dead_reckon_get_delta(&dx_um, &dy_um);
            uint16_t p_mN = pressure_get_force_mN();
            stroke_feed(now_ms, (int32_t)dx_um, (int32_t)dy_um,
                        p_mN, pen_down);
            dead_reckon_clear_delta();
        }

        /* ---- Periodic tasks ---- */
        poll_optflow(now_ms);
        flush_segments(now_ms);

        /* ---- Power state machine ---- */
        bool motion_recent = (now_ms - g_last_motion_ms) < 5000;
        if (pen_down || motion_recent) {
            if (pen_down) g_power_state = PWR_STATE_WRITING;
            else if (ble_pen_is_connected())
                g_power_state = PWR_STATE_CONNECTED_IDLE;
            else
                g_power_state = PWR_STATE_ADVERTISING;
            g_last_motion_ms = now_ms;
        } else if (IS_CHARGING()) {
            g_power_state = PWR_STATE_CHARGING;
        } else if (pen_is_capped()) {
            g_power_state = PWR_STATE_OFF;
        } else {
            g_power_state = PWR_STATE_ADVERTISING;
        }

        /* ---- Sleep until next interrupt ---- */
        if (g_power_state != PWR_STATE_WRITING) {
            enter_sleep();
        }
    }

    /* unreachable */
    return 0;
}