/* SPDX-License-Identifier: MIT
 * QuadraAlign application state machine
 * Author: jayis1
 */
#include "board.h"
#include "protocol.h"
#include "sensors.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

typedef enum { MODE_BOOT, MODE_IDLE, MODE_MEASURE, MODE_TARE, MODE_FAULT, MODE_SLEEP } app_mode_t;
typedef struct {
    app_mode_t mode; qa_config_t config; qa_measurement_t latest;
    uint32_t last_sample_ms; uint32_t last_report_ms; uint32_t button_down_ms;
    bool button_previous; bool capture_pending; uint16_t sensor_failures;
} app_t;

static app_t app;

static void set_status_led(void) {
    if (app.mode == MODE_FAULT || app.latest.overloaded) { board_led_set(LED_RED, 180U); return; }
    if (board_battery_mv() < QA_BATTERY_LOW_MV) { board_led_set(LED_PURPLE, 100U); return; }
    if (app.mode == MODE_TARE) { board_led_set(LED_BLUE, 160U); return; }
    if (app.mode == MODE_MEASURE && app.latest.stable) {
        bool aligned = app.latest.imbalance_percent < app.config.warn_imbalance_percent &&
            fabsf(app.latest.pitch_deg) < app.config.warn_tilt_deg && fabsf(app.latest.roll_deg) < app.config.warn_tilt_deg;
        board_led_set(aligned ? LED_GREEN : LED_AMBER, 140U);
        return;
    }
    board_led_set(LED_BLUE, 40U);
}

static void initialize(void) {
    memset(&app, 0, sizeof app);
    app.mode = MODE_BOOT;
    board_init();
    protocol_init();
    estimator_init();
    if (!config_load(&app.config)) {
        config_defaults(&app.config);
        (void)config_save(&app.config);
    }
    if (!sensors_init()) {
        app.mode = MODE_FAULT;
        (void)protocol_send_event(1U, "IMU initialization failed");
        return;
    }
    app.mode = MODE_MEASURE;
    (void)protocol_send_event(0U, "QuadraAlign ready; author jayis1");
}

static void perform_tare(void) {
    app.mode = MODE_TARE;
    set_status_led();
    (void)protocol_send_event(10U, "Tare started; unload all pucks");
    sensors_tare(&app.config, 32U);
    if (config_save(&app.config)) (void)protocol_send_event(11U, "Tare complete");
    else (void)protocol_send_event(12U, "Tare flash save failed");
    estimator_init();
    app.mode = MODE_MEASURE;
}

static void handle_command(const qa_command_t *command) {
    switch (command->kind) {
        case EVENT_TARE: perform_tare(); break;
        case EVENT_CAPTURE:
            app.capture_pending = true;
            (void)protocol_send_event(20U, app.latest.stable ? "Stable capture marked" : "Unstable capture marked");
            break;
        case EVENT_SET_LIMITS:
            if (command->value_a >= 1.0f && command->value_a <= 50.0f && command->value_b >= 0.05f && command->value_b <= 5.0f) {
                app.config.warn_imbalance_percent = command->value_a;
                app.config.warn_tilt_deg = command->value_b;
                (void)config_save(&app.config);
                (void)protocol_send_event(21U, "Limits updated");
            } else (void)protocol_send_event(22U, "Limits rejected");
            break;
        case EVENT_CALIBRATE:
            if (command->channel < QA_NUM_FEET && command->value_a >= 1.0f) {
                load_frame_t load;
                if (sensors_read_load(&load, &app.config)) {
                    int32_t zero = (int32_t)app.config.cal[command->channel].offset_counts;
                    sensors_calibrate_channel(&app.config, command->channel, zero, load.raw[command->channel], command->value_a);
                    (void)config_save(&app.config);
                    (void)protocol_send_event(23U, "Channel calibrated");
                }
            }
            break;
        case EVENT_SLEEP: app.mode = MODE_SLEEP; (void)protocol_send_event(30U, "Sleeping"); break;
        case EVENT_NONE: default: break;
    }
}

static void poll_button(void) {
    bool pressed = !board_gpio_read(PIN_BUTTON);
    uint32_t now = board_millis();
    if (pressed && !app.button_previous) app.button_down_ms = now;
    if (!pressed && app.button_previous) {
        uint32_t duration = now - app.button_down_ms;
        if (duration >= QA_LONG_PRESS_MS) perform_tare();
        else if (duration >= QA_BUTTON_DEBOUNCE_MS) app.capture_pending = true;
    }
    app.button_previous = pressed;
}

static bool acquire(void) {
    load_frame_t load;
    imu_frame_t imu;
    if (!sensors_read_load(&load, &app.config) || !sensors_read_imu(&imu)) {
        if (++app.sensor_failures > 20U) {
            app.mode = MODE_FAULT;
            (void)protocol_send_event(2U, "Repeated sensor acquisition failure");
        }
        return false;
    }
    app.sensor_failures = 0U;
    return estimator_update(&load, &imu, &app.config, &app.latest);
}

static void app_iteration(void) {
    protocol_poll();
    qa_command_t command;
    while (protocol_next_command(&command)) handle_command(&command);
    poll_button();
    uint32_t now = board_millis();
    if (app.mode == MODE_SLEEP) { set_status_led(); board_power_off(); return; }
    if (app.mode == MODE_FAULT) { set_status_led(); board_watchdog_kick(); return; }
    uint32_t sample_period = 1000U / QA_SAMPLE_HZ;
    if ((now - app.last_sample_ms) >= sample_period) {
        app.last_sample_ms = now;
        (void)acquire();
    }
    uint32_t report_period = 1000U / QA_REPORT_HZ;
    if ((now - app.last_report_ms) >= report_period || app.capture_pending) {
        app.last_report_ms = now;
        if (protocol_send_measurement(&app.latest, board_battery_mv())) app.capture_pending = false;
    }
    set_status_led();
    board_watchdog_kick();
}

static int self_test(void) {
    initialize();
    if (app.mode == MODE_FAULT) return 1;
    qa_config_t saved = app.config;
    if (!config_save(&saved)) return 2;
    qa_config_t loaded;
    if (!config_load(&loaded) || loaded.crc32 != saved.crc32) return 3;
    if (protocol_crc16((const uint8_t *)"123456789", 9U) != 0x29B1U) return 4;
    for (uint16_t i = 0; i < 80U; ++i) app_iteration();
    if (app.latest.sequence == 0U || !isfinite(app.latest.total_kg)) return 5;
    printf("SELF-TEST PASS author=%s sequence=%lu total=%.2fkg battery=%umV\n",
           QA_AUTHOR, (unsigned long)app.latest.sequence, app.latest.total_kg, board_battery_mv());
    return 0;
}

int main(int argc, char **argv) {
    if (argc > 1 && strcmp(argv[1], "--self-test") == 0) return self_test();
    initialize();
#ifdef QUADRALIGN_EMBEDDED
    for (;;) app_iteration();
#else
    for (uint16_t i = 0; i < 500U; ++i) app_iteration();
    printf("QuadraAlign simulation complete: %lu frames\n", (unsigned long)app.latest.sequence);
#endif
    return app.mode == MODE_FAULT ? 1 : 0;
}
