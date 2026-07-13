/*
 * main.c — System initialization, scheduler, and super-loop for HivePulse
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#include "board.h"
#include "audio.h"
#include "dsp.h"
#include "ml_model.h"
#include "sensors.h"
#include "bee_counter.h"
#include "lora.h"
#include "ble.h"
#include "power.h"

/* ---- Task Scheduler ---- */
typedef enum {
    TASK_AUDIO_CAPTURE    = 0,
    TASK_DSP_PROCESS      = 1,
    TASK_ML_INFERENCE     = 2,
    TASK_SENSOR_READ      = 3,
    TASK_BEE_COUNTER_POLL = 4,
    TASK_LORA_UPLINK      = 5,
    TASK_BLE_SERVICE      = 6,
    TASK_POWER_MANAGE     = 7,
    TASK_WATCHDOG_REFRESH = 8,
    TASK_COUNT
} task_id_t;

typedef struct {
    uint32_t period_ms;     /* Task period in milliseconds (0 = event-driven) */
    uint32_t last_run_ms;   /* Last execution timestamp */
    uint8_t  priority;      /* 0=highest, 255=lowest */
    bool     enabled;
    bool     pending;       /* Event flag for event-driven tasks */
} task_t;

static task_t tasks[TASK_COUNT];

/* ---- System State ---- */
typedef struct {
    uint32_t boot_count;
    uint32_t uptime_s;
    uint32_t last_lora_tx_s;
    uint32_t last_listen_s;
    uint32_t listen_windows_today;
    bool     winter_mode;
    bool     listening_active;
    bool     alert_swarm_pending;
    bool     alert_queenless_pending;
    uint8_t  consecutive_swarm_detections;
    uint8_t  consecutive_queenless_detections;
    float    battery_mv;
    float    solar_mv;
    float    charge_current_ma;
} sys_state_t;

static sys_state_t sys_state;

/* ---- Global Statistics ---- */
static volatile uint32_t systick_ms = 0;
static colony_state_t latest_colony_state = COLONY_QUEENRIGHT_HEALTHY;
static float latest_confidence = 0.0f;
static feature_vector_t latest_features;
static sensor_data_t latest_sensor_data;
static bee_traffic_t latest_bee_traffic;
static audio_snapshot_t latest_audio_snapshot;

/* ---- Forward Declarations ---- */
static void scheduler_init(void);
static void scheduler_run(void);
static void tasks_init(void);
static void sys_state_init(void);
static void check_alerts(void);
static void handle_lora_downlink(const lora_downlink_t *dl);
static void enter_low_power(void);
static void log_debug(const char *msg);

/* ---- SysTick Handler (1 ms) ---- */
void SysTick_Handler(void)
{
    systick_ms++;
}

uint32_t millis(void)
{
    return systick_ms;
}

uint32_t seconds(void)
{
    return systick_ms / 1000;
}

/* ---- Main Entry Point ---- */
int main(void)
{
    /* Step 1: Initialize hardware clocks and GPIO */
    board_init();

    /* Step 2: Initialize watchdog (10-second window) */
    power_watchdog_init(10000);

    /* Step 3: Initialize RTC and restore state from backup registers */
    power_rtc_init();
    sys_state_init();

    /* Step 4: Initialize all peripherals and sensors */
    sensors_init();
    bee_counter_init();
    audio_init();
    dsp_init();
    ml_model_init();
    lora_init();
    ble_init();
    power_management_init();

    /* Step 5: Join LoRaWAN network (OTAA) */
    log_debug("HivePulse boot: joining LoRaWAN...");
    if (lora_join()) {
        log_debug("LoRaWAN joined successfully");
        /* Set Class B mode for scheduled receive windows */
        lora_set_class_b();
    } else {
        log_debug("LoRaWAN join failed, will retry");
    }

    /* Step 6: Perform initial sensor sweep */
    sensors_read_all(&latest_sensor_data);
    power_read_battery(&sys_state.battery_mv,
                       &sys_state.solar_mv,
                       &sys_state.charge_current_ma);

    /* Step 7: Detect winter mode based on ambient temperature */
    if (latest_sensor_data.temps[TEMP_EXT_AMBIENT] < 10.0f) {
        sys_state.winter_mode = true;
        log_debug("Winter mode enabled (ambient < 10C)");
    }

    /* Step 8: Initialize and start scheduler */
    tasks_init();
    scheduler_init();

    log_debug("HivePulse system ready, entering main loop");

    /* Step 9: Main super-loop */
    while (1) {
        scheduler_run();
    }

    return 0; /* Never reached */
}

/* ---- Task Definitions ---- */
static void tasks_init(void)
{
    /* Audio capture: event-driven (triggered by DMA half/full transfer) */
    tasks[TASK_AUDIO_CAPTURE].period_ms = 0;
    tasks[TASK_AUDIO_CAPTURE].priority = 0;
    tasks[TASK_AUDIO_CAPTURE].enabled = true;
    tasks[TASK_AUDIO_CAPTURE].pending = false;

    /* DSP processing: event-driven (triggered when audio buffer ready) */
    tasks[TASK_DSP_PROCESS].period_ms = 0;
    tasks[TASK_DSP_PROCESS].priority = 1;
    tasks[TASK_DSP_PROCESS].enabled = true;
    tasks[TASK_DSP_PROCESS].pending = false;

    /* ML inference: event-driven (triggered when features extracted) */
    tasks[TASK_ML_INFERENCE].period_ms = 0;
    tasks[TASK_ML_INFERENCE].priority = 2;
    tasks[TASK_ML_INFERENCE].enabled = true;
    tasks[TASK_ML_INFERENCE].pending = false;

    /* Sensor read: every 60 seconds */
    tasks[TASK_SENSOR_READ].period_ms = 60000;
    tasks[TASK_SENSOR_READ].priority = 5;
    tasks[TASK_SENSOR_READ].enabled = true;
    tasks[TASK_SENSOR_READ].last_run_ms = 0;

    /* Bee counter poll: every 100 ms (aggregates counts) */
    tasks[TASK_BEE_COUNTER_POLL].period_ms = 100;
    tasks[TASK_BEE_COUNTER_POLL].priority = 4;
    tasks[TASK_BEE_COUNTER_POLL].enabled = true;
    tasks[TASK_BEE_COUNTER_POLL].last_run_ms = 0;

    /* LoRa uplink: every 4 hours (14400 s = 14,400,000 ms) */
    tasks[TASK_LORA_UPLINK].period_ms = LORA_TX_INTERVAL_S * 1000;
    tasks[TASK_LORA_UPLINK].priority = 6;
    tasks[TASK_LORA_UPLINK].enabled = true;
    tasks[TASK_LORA_UPLINK].last_run_ms = 0;

    /* BLE service: every 50 ms */
    tasks[TASK_BLE_SERVICE].period_ms = 50;
    tasks[TASK_BLE_SERVICE].priority = 3;
    tasks[TASK_BLE_SERVICE].enabled = true;
    tasks[TASK_BLE_SERVICE].last_run_ms = 0;

    /* Power management: every 10 seconds */
    tasks[TASK_POWER_MANAGE].period_ms = 10000;
    tasks[TASK_POWER_MANAGE].priority = 7;
    tasks[TASK_POWER_MANAGE].enabled = true;
    tasks[TASK_POWER_MANAGE].last_run_ms = 0;

    /* Watchdog refresh: every 5 seconds */
    tasks[TASK_WATCHDOG_REFRESH].period_ms = 5000;
    tasks[TASK_WATCHDOG_REFRESH].priority = 0;
    tasks[TASK_WATCHDOG_REFRESH].enabled = true;
    tasks[TASK_WATCHDOG_REFRESH].last_run_ms = 0;
}

static void scheduler_init(void)
{
    /* Configure SysTick for 1 ms tick at 280 MHz */
    SysTick->LOAD = (SYSCLK_FREQ / SYSTICK_HZ) - 1;
    SysTick->VAL = 0;
    SysTick->CTRL = SysTick_CTRL_ENABLE | SysTick_CTRL_TICKINT | SysTick_CTRL_CLKSRC;

    /* Set NVIC priority group 4 (all preemption, no sub-priority) */
    *(volatile uint32_t *)(SCB_BASE + 0x0C) = 0x05FA0500; /* AIRCR PRIGROUP=4 */
}

static void scheduler_run(void)
{
    uint32_t now = millis();
    int i;

    /* Scan tasks in priority order */
    for (i = 0; i < TASK_COUNT; i++) {
        if (!tasks[i].enabled)
            continue;

        bool should_run = false;

        if (tasks[i].period_ms > 0) {
            /* Periodic task */
            if ((now - tasks[i].last_run_ms) >= tasks[i].period_ms) {
                should_run = true;
                tasks[i].last_run_ms = now;
            }
        } else {
            /* Event-driven task */
            if (tasks[i].pending) {
                should_run = true;
                tasks[i].pending = false;
            }
        }

        if (should_run) {
            switch ((task_id_t)i) {
            case TASK_AUDIO_CAPTURE:
                /* Audio is DMA-driven; this handles starting a listening window */
                if (!sys_state.listening_active) {
                    uint32_t interval = sys_state.winter_mode ?
                        WINTER_LISTEN_INT_S : LISTEN_INTERVAL_S;
                    if ((seconds() - sys_state.last_listen_s) >= interval) {
                        sys_state.listening_active = true;
                        sys_state.last_listen_s = seconds();
                        audio_start_capture();
                        power_set_mode(POWER_MODE_ACTIVE);
                    }
                } else {
                    /* Check if listening window is over */
                    if ((seconds() - sys_state.last_listen_s) >= LISTEN_DURATION_S) {
                        sys_state.listening_active = false;
                        audio_stop_capture();
                        power_set_mode(POWER_MODE_LOWPOWER);
                    }
                }
                break;

            case TASK_DSP_PROCESS:
                if (audio_get_snapshot(&latest_audio_snapshot)) {
                    dsp_extract_features(&latest_audio_snapshot,
                                        &latest_features);
                    /* Signal ML task that features are ready */
                    tasks[TASK_ML_INFERENCE].pending = true;
                }
                break;

            case TASK_ML_INFERENCE:
                latest_colony_state = ml_predict(&latest_features,
                                                &latest_confidence);
                sys_state.listen_windows_today++;
                check_alerts();
                break;

            case TASK_SENSOR_READ:
                sensors_read_all(&latest_sensor_data);

                /* Check for winter mode transition */
                float ambient = latest_sensor_data.temps[TEMP_EXT_AMBIENT];
                if (!sys_state.winter_mode && ambient < 8.0f) {
                    sys_state.winter_mode = true;
                    log_debug("Switching to winter mode");
                } else if (sys_state.winter_mode && ambient > 12.0f) {
                    sys_state.winter_mode = false;
                    log_debug("Switching to active season mode");
                }
                break;

            case TASK_BEE_COUNTER_POLL:
                bee_counter_poll(&latest_bee_traffic);
                break;

            case TASK_LORA_UPLINK: {
                /* Build and transmit uplink packet */
                lora_uplink_t uplink;
                uplink.timestamp = seconds();
                uplink.colony_state = latest_colony_state;
                uplink.confidence = latest_confidence;
                uplink.battery_mv = sys_state.battery_mv;
                uplink.solar_mv = sys_state.solar_mv;
                uplink.weight_kg = latest_sensor_data.weight_kg;
                uplink.ambient_temp = latest_sensor_data.temps[TEMP_EXT_AMBIENT];
                uplink.brood_temp = latest_sensor_data.temps[TEMP_BROOD_TOP];
                uplink.humidity = latest_sensor_data.humidity;
                uplink.co2_ppm = latest_sensor_data.co2_ppm;
                uplink.bees_in = latest_bee_traffic.bees_in;
                uplink.bees_out = latest_bee_traffic.bees_out;
                uplink.swarm_alert = sys_state.alert_swarm_pending;
                uplink.queenless_alert = sys_state.alert_queenless_pending;
                uplink.winter_mode = sys_state.winter_mode;

                lora_send_uplink(&uplink);
                sys_state.last_lora_tx_s = seconds();

                /* Clear alerts after uplink (they're now in the cloud) */
                sys_state.alert_swarm_pending = false;
                sys_state.alert_queenless_pending = false;
                break;
            }

            case TASK_BLE_SERVICE:
                ble_service();
                /* Check for downlink commands from BLE */
                ble_command_t cmd;
                if (ble_get_command(&cmd)) {
                    switch (cmd.type) {
                    case BLE_CMD_REQUEST_AUDIO:
                        audio_start_capture();
                        break;
                    case BLE_CMD_REQUEST_SENSORS:
                        ble_send_sensor_data(&latest_sensor_data);
                        break;
                    case BLE_CMD_TARE_WEIGHT:
                        sensors_tare_load_cells();
                        break;
                    case BLE_CMD_SET_SCHEDULE:
                        /* Adjust sampling schedule */
                        break;
                    case BLE_CMD_OTA_UPDATE:
                        ble_enter_dfu();
                        break;
                    default:
                        break;
                    }
                }
                break;

            case TASK_POWER_MANAGE:
                power_read_battery(&sys_state.battery_mv,
                                  &sys_state.solar_mv,
                                  &sys_state.charge_current_ma);

                /* Low battery protection */
                if (sys_state.battery_mv < BATTERY_CRIT_MV) {
                    log_debug("CRITICAL battery, entering deep sleep");
                    power_set_mode(POWER_MODE_SHUTDOWN);
                    enter_low_power();
                } else if (sys_state.battery_mv < BATTERY_LOW_MV) {
                    /* Reduce sampling rate to conserve power */
                    if (!sys_state.winter_mode) {
                        sys_state.winter_mode = true;
                        log_debug("Low battery, reducing sampling");
                    }
                }
                break;

            case TASK_WATCHDOG_REFRESH:
                power_watchdog_refresh();
                break;

            default:
                break;
            }
        }
    }

    /* Check for LoRa downlinks (Class B ping slots) */
    lora_downlink_t dl;
    if (lora_check_downlink(&dl)) {
        handle_lora_downlink(&dl);
    }

    /* Enter sleep mode if nothing is pending and not listening */
    if (!sys_state.listening_active) {
        __WFI();
    }
}

/* ---- Alert Detection Logic ---- */
static void check_alerts(void)
{
    /* Swarm prediction: requires 6 consecutive pre-swarm detections
     * (6 × 5 min = 30 minutes of consistent pre-swarm signal) */
    if (latest_colony_state == COLONY_PREPARING_SWARM) {
        sys_state.consecutive_swarm_detections++;
        if (sys_state.consecutive_swarm_detections >= 6) {
            sys_state.alert_swarm_pending = true;
            log_debug("SWARM ALERT triggered");
            /* Trigger immediate LoRa uplink rather than waiting */
            tasks[TASK_LORA_UPLINK].pending = true;
            tasks[TASK_LORA_UPLINK].last_run_ms = 0;
        }
    } else {
        sys_state.consecutive_swarm_detections = 0;
    }

    /* Queen loss detection: requires 4 consecutive queenless detections
     * (4 × 5 min = 20 minutes, filters out momentary classification noise) */
    if (latest_colony_state == COLONY_QUEENLESS) {
        sys_state.consecutive_queenless_detections++;
        if (sys_state.consecutive_queenless_detections >= 4) {
            sys_state.alert_queenless_pending = true;
            log_debug("QUEENLESS ALERT triggered");
            tasks[TASK_LORA_UPLINK].pending = true;
            tasks[TASK_LORA_UPLINK].last_run_ms = 0;
        }
    } else {
        sys_state.consecutive_queenless_detections = 0;
    }

    /* Dead colony detection: trigger alert immediately */
    if (latest_colony_state == COLONY_DEAD_INACTIVE &&
        latest_confidence > 0.90f) {
        sys_state.alert_queenless_pending = true; /* Reuse alert channel */
    }
}

/* ---- LoRa Downlink Handler ---- */
static void handle_lora_downlink(const lora_downlink_t *dl)
{
    switch (dl->command) {
    case LORA_CMD_INCREASE_SAMPLING:
        /* Override sampling to every 60 seconds for the next hour */
        sys_state.last_listen_s = seconds() - LISTEN_INTERVAL_S + 60;
        log_debug("Downlink: increase sampling rate");
        break;

    case LORA_CMD_REQUEST_AUDIO_SNAPSHOT:
        /* Capture and send a raw audio window via LoRa (compressed) */
        audio_start_capture();
        log_debug("Downlink: audio snapshot requested");
        break;

    case LORA_CMD_SET_WINTER_MODE:
        sys_state.winter_mode = (dl->param != 0);
        log_debug("Downlink: winter mode set");
        break;

    case LORA_CMD_RESTART:
        log_debug("Downlink: restart command");
        /* Save state to RTC backup registers before restart */
        power_save_state_to_rtc(&sys_state, sizeof(sys_state));
        __ISB();
        SCB_AIRCR = 0x05FA0004; /* System reset */
        break;

    case LORA_CMD_CALIBRATE_WEIGHT:
        sensors_tare_load_cells();
        log_debug("Downlink: weight calibration (tare)");
        break;

    default:
        log_debug("Downlink: unknown command");
        break;
    }
}

/* ---- System State Initialization ---- */
static void sys_state_init(void)
{
    /* Try to restore state from RTC backup registers */
    if (power_restore_state_from_rtc(&sys_state, sizeof(sys_state))) {
        sys_state.boot_count++;
        log_debug("State restored from RTC, boot count incremented");
    } else {
        sys_state.boot_count = 1;
        sys_state.uptime_s = 0;
        sys_state.last_lora_tx_s = 0;
        sys_state.last_listen_s = 0;
        sys_state.listen_windows_today = 0;
        sys_state.winter_mode = false;
        sys_state.listening_active = false;
        sys_state.alert_swarm_pending = false;
        sys_state.alert_queenless_pending = false;
        sys_state.consecutive_swarm_detections = 0;
        sys_state.consecutive_queenless_detections = 0;
        sys_state.battery_mv = 3300.0f;
        sys_state.solar_mv = 0.0f;
        sys_state.charge_current_ma = 0.0f;
        log_debug("Fresh system state initialized");
    }
}

/* ---- Low Power Entry ---- */
static void enter_low_power(void)
{
    /* Stop audio and sensors to minimize power */
    audio_stop_capture();
    sensors_sleep();

    /* Configure RTC alarm to wake us up in 1 hour */
    power_set_rtc_wakeup(3600);

    /* Enter Stop2 mode (lowest power with RTC running) */
    __disable_irq();
    SCB_SCR |= SCB_SCR_SLEEPDEEP;
    __DSB();
    __WFI();
    /* Returns here after RTC wakeup */
    SCB_SCR &= ~SCB_SCR_SLEEPDEEP;
    __enable_irq();

    /* Re-initialize clocks after wake from Stop mode */
    board_clock_config();
    sensors_wake();
    log_debug("Woke from low-power mode");
}

/* ---- Debug Logging ---- */
static void log_debug(const char *msg)
{
#ifdef DEBUG_ENABLE
    /* Output via LPUART1 at 115200 baud */
    while (*msg) {
        while (!(LPUART1->ISR & USART_ISR_TXE));
        LPUART1->TDR = (uint8_t)*msg++;
    }
    while (!(LPUART1->ISR & USART_ISR_TC));
    /* Newline */
    while (!(LPUART1->ISR & USART_ISR_TXE));
    LPUART1->TDR = '\r';
    while (!(LPUART1->ISR & USART_ISR_TXE));
    LPUART1->TDR = '\n';
#else
    (void)msg;
#endif
}