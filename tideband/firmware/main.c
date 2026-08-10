/**
 * @file    main.c
 * @brief   TideBand — Main firmware entry point and super-loop state machine.
 *          Manages dive detection, sample scheduling, sensor orchestration,
 *          data logging, BLE communication, haptic feedback, and display.
 * @author  jayis1
 * @copyright © 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 *
 * Hardware: STM32H733VGT6 (280 MHz Cortex-M7)
 *
 * State machine:
 *   SURFACE → (depth > 0.5m) → DIVING → (depth < 0.5m for 5min) → SURFACE
 *
 * Sample timing:
 *   - Attitude: 100 Hz (background, interrupt-driven from IMU DRDY)
 *   - Doppler + Depth: 1-4 Hz (user-configurable, default 2 Hz)
 *   - Display: 4 Hz (every 250 ms)
 *   - BLE: event-driven + status at 1 Hz
 *   - Haptic: 100 Hz (pattern state machine)
 *
 * The super-loop runs continuously, checking timers for each subsystem
 * and dispatching work. ISRs handle time-critical DMA completion and
 * UART RX buffering.
 */

#include <string.h>
#include <math.h>
#include "board.h"
#include "registers.h"
#include "drivers/doppler.h"
#include "drivers/attitude.h"
#include "drivers/depth.h"
#include "drivers/storage.h"
#include "drivers/display.h"
#include "drivers/ble_link.h"
#include "drivers/haptic.h"
#include "drivers/power.h"

/* ---- System state machine ---- */
typedef enum {
    STATE_SURFACE = 0,
    STATE_DIVING,
    STATE_SLEEP,
} system_state_t;

/* ---- Global state ---- */
static system_state_t sys_state = STATE_SURFACE;
static doppler_result_t last_doppler;
static depth_data_t    last_depth;
static attitude_t      last_attitude;
static uint16_t        sample_rate_hz = 2;
static uint32_t        dive_start_time = 0;
static uint32_t        dive_id = 0;
static float           max_depth_this_dive = 0.0f;
static float           current_speed_sum = 0.0f;
static uint16_t        current_speed_count = 0;
static uint32_t        surface_timer_s = 0;
static uint8_t         display_update_counter = 0;

/* ---- SysTick — 1 ms timer ---- */
static volatile uint32_t sys_ticks = 0;
static volatile uint32_t sys_seconds = 0;

/* Simple non-RTOS delay */
static void delay_ms(uint32_t ms)
{
    uint32_t start = sys_ticks;
    while (sys_ticks - start < ms) { }
}

/* Get seconds since boot */
static uint32_t get_seconds(void)
{
    return sys_seconds;
}

/* ---- Clock configuration ---- */
static void clock_init(void)
{
    /* Enable HSE (8 MHz external crystal) */
    RCC_CR |= RCC_CR_HSEON;
    while ((RCC_CR & RCC_CR_HSERDY) == 0) { }

    /* Configure PLL1: HSE / M(1) × N(70) / P(2) = 280 MHz
     *                Q(5) = 56 MHz for USB
     *                R(2) = 280 MHz for SYSCLK */
    RCC_PLLCFGR = (1u << 0) |          /* M = 1 */
                  (70u << 8) |         /* N = 70 */
                  (0u << 16) |         /* P = 2 (00 → /2) */
                  (1u << 24) |         /* PLL1SRC = HSE */
                  (5u << 21);          /* Q = 5 */

    /* Enable PLL1 */
    RCC_CR |= RCC_CR_PLL1ON;
    while ((RCC_CR & RCC_CR_PLL1RDY) == 0) { }

    /* Set voltage scaling to VOS3 (maximum) for 280 MHz */
    PWR_CR1 = (PWR_CR1 & ~PWR_CR1_VOS_MASK) | (PWR_CR1_VOS_3 << PWR_CR1_VOS_SHIFT);

    /* Set flash latency for 280 MHz (WS = 4 for VOS3) */
    *(volatile uint32_t *)(0x52002000u + 0x00u) = 4u << 0;  /* FLASH_ACR */

    /* Switch SYSCLK to PLL1 */
    RCC_CFGR = (RCC_CFGR & ~(3u << 0)) | (2u << 0);  /* SW = PLL1 */
    while (((RCC_CFGR >> 2) & 3u) != 2u) { }  /* Wait for switch */

    /* Set AHB = SYSCLK / 1, APB1 = SYSCLK / 4, APB2 = SYSCLK / 2 */
    RCC_CFGR = (RCC_CFGR & ~0xFFu) | (0u << 4) | (5u << 8) | (4u << 0);
    /* HPRE=0 (/1), PPRE1=5 (/4), PPRE2=4 (/2) */
}

/* ---- SysTick initialization ---- */
static void systick_init(void)
{
    /* SysTick: 24-bit down-counter, wraps at reload value */
    *(volatile uint32_t *)(0xE000E014u) = SYSTICK_RELOAD;  /* LOAD */
    *(volatile uint32_t *)(0xE000E018u) = 0;                 /* VAL = 0 */
    *(volatile uint32_t *)(0xE000E010u) = 0x7u;              /* CTRL: enable, IRQ, clk */
}

/* ---- SysTick interrupt handler ---- */
void SysTick_Handler(void)
{
    sys_ticks++;
    if (sys_ticks % 1000 == 0) {
        sys_seconds++;
    }
}

/* ---- BLE command callback ---- */
static void ble_cmd_handler(uint8_t opcode, const uint8_t *payload, uint8_t len)
{
    switch (opcode) {
        case BLE_OP_SET_RATE:
            if (len >= 1 && payload[0] >= 1 && payload[0] <= 4) {
                sample_rate_hz = payload[0];
            }
            break;
        case BLE_OP_SET_THRESHOLD:
            if (len >= 4) {
                float threshold;
                memcpy(&threshold, payload, 4);
                haptic_set_threshold(threshold);
            }
            break;
        case BLE_OP_ERASE_DIVES:
            storage_erase_all();
            break;
        case BLE_OP_CAL_SET:
            /* Calibration data — would update Doppler calibration */
            break;
        default:
            break;
    }
}

/* ---- Dive management ---- */
static void check_dive_transition(void)
{
    float depth = last_depth.depth_m;

    if (sys_state == STATE_SURFACE) {
        /* Check for dive start */
        if (depth > DIVE_IMMERSION_DEPTH_M && last_depth.valid) {
            sys_state = STATE_DIVING;
            dive_start_time = get_seconds();
            surface_timer_s = 0;

            /* Set surface pressure reference */
            depth_set_surface(last_depth.pressure_mbar);

            /* Start dive log */
            dive_id = storage_start_dive(dive_start_time, sample_rate_hz);

            /* Notify app */
            ble_link_send_dive_event(1, dive_start_time, dive_id);

            /* Haptic: double pulse to confirm dive start */
            haptic_trigger(HAPTIC_DOUBLE);

            /* Switch display to dive mode */
            display_set_mode(DISP_MODE_DIVE);

            max_depth_this_dive = 0.0f;
            current_speed_sum = 0.0f;
            current_speed_count = 0;
        }
    } else if (sys_state == STATE_DIVING) {
        /* Track max depth */
        if (depth > max_depth_this_dive) {
            max_depth_this_dive = depth;
        }

        /* Check for dive end (back at surface) */
        if (depth <= DIVE_IMMERSION_DEPTH_M && last_depth.valid) {
            surface_timer_s++;
            if (surface_timer_s > DIVE_SURFACE_TIMEOUT_S) {
                /* End dive */
                uint32_t end_time = get_seconds();
                storage_end_dive(end_time);
                ble_link_send_dive_event(0, end_time, dive_id);

                sys_state = STATE_SURFACE;
                display_set_mode(DISP_MODE_SURFACE);
                surface_timer_s = 0;

                /* Haptic: long pulse to confirm dive end */
                haptic_trigger(HAPTIC_LONG);
            }
        } else {
            surface_timer_s = 0;  /* Reset surface timer if back underwater */
        }
    }
}

/* ---- Haptic direction encoding ---- */
static void update_haptic_for_current(float speed_ms, float heading_deg)
{
    /* Only trigger if speed exceeds threshold and no pattern is active */
    if (speed_ms < 0.5f || haptic_is_active()) {
        return;
    }

    /* Encode direction as haptic pattern:
     * N (315-45°):   LONG pulse
     * E (45-135°):   DOUBLE pulse
     * S (135-225°):  SHORT pulse
     * W (225-315°):  TRIPLE pulse
     * (Heading is compass degrees: 0=N, 90=E, 180=S, 270=W) */
    if (heading_deg < 0) heading_deg += 360.0f;
    if (heading_deg < 45.0f || heading_deg >= 315.0f) {
        haptic_trigger(HAPTIC_LONG);
    } else if (heading_deg < 135.0f) {
        haptic_trigger(HAPTIC_DOUBLE);
    } else if (heading_deg < 225.0f) {
        haptic_trigger(HAPTIC_SHORT);
    } else {
        haptic_trigger(HAPTIC_TRIPLE);
    }
}

/* ---- Main function ---- */
int main(void)
{
    /* ---- Boot sequence ---- */

    /* 1. Initialize clock system to 280 MHz */
    clock_init();

    /* 2. Initialize SysTick for 1 ms timing */
    systick_init();

    /* 3. Enable global interrupts */
    __asm volatile ("cpsie i");

    /* 4. Initialize all peripheral drivers */
    display_init();
    attitude_init();
    depth_init();
    doppler_init();
    storage_init();
    haptic_init();
    power_init();
    ble_link_init();

    /* 5. Calibrate gyro (device should be still during boot) */
    attitude_calibrate_gyro();

    /* 6. Set BLE callback */
    ble_link_set_callback(ble_cmd_handler);

    /* 7. Show surface display */
    display_set_mode(DISP_MODE_SURFACE);
    display_render_surface(100.0f, 0, 20.0f);

    /* 8. Wait a moment for sensors to settle */
    delay_ms(500);

    /* ---- Main super-loop ---- */

    uint32_t last_sample_ticks = 0;
    uint32_t last_attitude_ticks = 0;
    uint32_t last_display_ticks = 0;
    uint32_t last_ble_status_ticks = 0;
    uint32_t last_power_ticks = 0;
    uint32_t last_com_inversion_ticks = 0;

    const uint32_t attitude_period_ms = 10;     /* 100 Hz */
    const uint32_t display_period_ms = 250;     /* 4 Hz */
    const uint32_t ble_status_period_ms = 1000; /* 1 Hz */
    const uint32_t power_period_ms = 5000;      /* 0.2 Hz */
    const uint32_t com_inversion_period_ms = 10000; /* 0.1 Hz */

    while (1) {
        uint32_t now = sys_ticks;

        /* --- Attitude update (100 Hz) --- */
        if (now - last_attitude_ticks >= attitude_period_ms) {
            last_attitude_ticks = now;
            attitude_update(&last_attitude);
        }

        /* --- Doppler + Depth sampling (1-4 Hz) --- */
        uint32_t sample_period_ms = 1000 / sample_rate_hz;
        if (now - last_sample_ticks >= sample_period_ms) {
            last_sample_ticks = now;

            /* Read depth */
            depth_read(&last_depth);

            /* Trigger Doppler measurement */
            doppler_trigger();

            /* Wait for data (blocking — DMA ISR sets flag) */
            uint32_t timeout = now + 100;  /* 100 ms timeout */
            while (!doppler_data_ready() && sys_ticks < timeout) {
                /* Process BLE while waiting */
                ble_link_process();
            }

            if (doppler_data_ready()) {
                doppler_process(&last_doppler);

                /* Rotate body-frame velocity to NED using attitude */
                float vel_body[3] = {
                    last_doppler.vx,
                    last_doppler.vy,
                    last_doppler.vz
                };
                float vel_ned[3];
                attitude_rotate_to_ned(&last_attitude, vel_body, vel_ned);

                /* Check dive state transition */
                check_dive_transition();

                /* Log record if diving */
                if (sys_state == STATE_DIVING && last_doppler.valid) {
                    profile_record_t rec;
                    memset(&rec, 0, sizeof(rec));
                    rec.timestamp = get_seconds();
                    rec.depth_m = last_depth.depth_m;
                    rec.temp_c = last_depth.temp_c;
                    rec.vn_ms = vel_ned[0];
                    rec.ve_ms = vel_ned[1];
                    rec.vu_ms = vel_ned[2];
                    rec.speed_ms = last_doppler.speed;
                    rec.heading_deg = atan2f(vel_ned[1], vel_ned[0]) *
                                      57.2958f;
                    rec.roll_deg = (int16_t)(last_attitude.roll *
                                              57.2958f);
                    rec.pitch_deg = (int16_t)(last_attitude.pitch *
                                               57.2958f);
                    rec.quality = last_doppler.quality;

                    /* Compute CRC (simplified — use the storage's CRC) */
                    storage_write_record(&rec);

                    /* Update running average */
                    current_speed_sum += last_doppler.speed;
                    current_speed_count++;

                    /* Haptic feedback based on current direction */
                    float heading = atan2f(vel_ned[1], vel_ned[0]) *
                                    57.2958f;
                    if (heading < 0) heading += 360.0f;
                    update_haptic_for_current(last_doppler.speed, heading);
                }

                /* Send BLE profile data if connected */
                if (ble_link_is_connected()) {
                    ble_link_send_profile(&last_doppler, &last_depth,
                                          &last_attitude, get_seconds());
                }
            }
        }

        /* --- Display update (4 Hz) --- */
        if (now - last_display_ticks >= display_period_ms) {
            last_display_ticks = now;
            display_update_counter++;

            if (sys_state == STATE_DIVING) {
                uint32_t dive_time = get_seconds() - dive_start_time;
                display_render_dive(&last_doppler, &last_depth,
                                    &last_attitude,
                                    power_get_battery_pct(),
                                    dive_time);
            } else {
                display_render_surface(power_get_battery_pct(),
                                       storage_get_dive_count(),
                                       last_depth.temp_c);
            }
        }

        /* --- BLE status update (1 Hz) --- */
        if (now - last_ble_status_ticks >= ble_status_period_ms) {
            last_ble_status_ticks = now;

            if (ble_link_is_connected()) {
                float heading = 0.0f;
                if (last_doppler.valid) {
                    heading = atan2f(last_doppler.vy, last_doppler.vx) *
                              57.2958f;
                    if (heading < 0) heading += 360.0f;
                }

                ble_link_send_status(
                    (uint8_t)power_get_battery_pct(),
                    (sys_state == STATE_DIVING) ? 1 : 0,
                    storage_get_dive_count(),
                    last_depth.depth_m,
                    last_depth.temp_c,
                    last_doppler.speed,
                    heading,
                    last_doppler.quality
                );
            }
        }

        /* --- Power management update (0.2 Hz) --- */
        if (now - last_power_ticks >= power_period_ms) {
            last_power_ticks = now;
            power_update();

            /* Critical battery handling */
            if (power_is_critical() && sys_state != STATE_DIVING) {
                /* Low battery on surface — could enter sleep mode */
                display_render_error("LOW BAT");
            }
        }

        /* --- COM inversion for Sharp LCD (every 10 s) --- */
        if (now - last_com_inversion_ticks >= com_inversion_period_ms) {
            last_com_inversion_ticks = now;
            /* Toggle EXTMODE pin for LCD COM inversion */
            uint32_t extmode_val = gpio_read(LCD_EXTMODE_GPIO, LCD_EXTMODE_PIN);
            if (extmode_val) {
                gpio_clear(LCD_EXTMODE_GPIO, LCD_EXTMODE_PIN);
            } else {
                gpio_set(LCD_EXTMODE_GPIO, LCD_EXTMODE_PIN);
            }
        }

        /* --- Process BLE RX --- */
        ble_link_process();

        /* --- Update haptic state machine (every loop iteration) --- */
        haptic_update();

        /* --- Sleep mode (when on surface and idle) --- */
        if (sys_state == STATE_SURFACE) {
            /* Could enter Stop mode between samples to save power.
             * For now, just spin to keep things simple. In production,
             * we'd WFI here and wake on SysTick. */
            __asm volatile ("wfi");
        }
    }

    /* Should never reach here */
    return 0;
}