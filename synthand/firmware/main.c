/*
 * main.c — Synthand top-level firmware.
 *
 * Boot, initialization, main loop, state machine, power management, and the
 * sensor sampling orchestration that ties together IMU acquisition, EMG
 * acquisition, signal processing, gesture inference, MIDI/OSC output, and
 * haptic feedback.
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
#include "drivers/imu.h"
#include "drivers/emg.h"
#include "drivers/signal.h"
#include "drivers/gesture.h"
#include "drivers/tcn_model.h"
#include "drivers/haptic.h"
#include "drivers/ble_midi.h"
#include "drivers/osc.h"
#include "drivers/power.h"
#include "drivers/storage.h"
#include "drivers/usb.h"

/* -------------------------------------------------------------------------
 * Global state
 * Author: jayis1
 * ------------------------------------------------------------------------- */

static volatile system_state_t g_state = STATE_BOOT;
static volatile uint32_t g_tick_500hz = 0;    /* incremented every 2 ms */
static volatile uint32_t g_time_ms = 0;       /* system time in ms */
static volatile uint8_t  g_sample_ready = 0;  /* flag set by TIMER0 IRQ */

/* Sensor data buffers (filled by DMA / IRQ handlers) */
static imu_sample_t   g_imu_data[NUM_IMUS];
static emg_sample_t   g_emg_data;

/* Processed feature vector (output of signal.c) */
static feature_vector_t g_features;

/* Gesture result (output of gesture.c) */
static gesture_result_t g_gesture;

/* Calibration and mapping (loaded from flash) */
static calibration_t g_calib;
static mapping_t     g_mapping;

/* MIDI ring buffer for BLE-MIDI */
static midi_event_t  g_midi_ring[MIDI_RING_SIZE];
static volatile uint16_t g_midi_head = 0;
static volatile uint16_t g_midi_tail = 0;

/* Haptic pending queue */
static volatile uint8_t g_haptic_pending[NUM_HAPTIC] = {0};

/* -------------------------------------------------------------------------
 * Forward declarations
 * ------------------------------------------------------------------------- */
static void clock_init(void);
static void gpio_init(void);
static void timer0_init(void);
static void system_halt(void);
static void load_config(void);
static void enter_state(system_state_t new_state);
static void process_sample(void);
static void emit_midi(const gesture_result_t *gest);
static void emit_osc(const feature_vector_t *feat,
                     const gesture_result_t *gest);
static void handle_button(void);
static void update_led(void);

/* -------------------------------------------------------------------------
 * Default mapping table (used if flash is empty)
 * Author: jayis1
 * ------------------------------------------------------------------------- */
static const mapping_t default_mapping = {
    .magic = MAPPING_MAGIC,
    .midi_channel = DEFAULT_MIDI_CHANNEL,
    .notes = { DEFAULT_NOTE_FINGER0, DEFAULT_NOTE_FINGER1,
               DEFAULT_NOTE_FINGER2, DEFAULT_NOTE_FINGER3,
               DEFAULT_NOTE_FINGER4 },
    .cc_emg = { 20, 21, 22, 23, 24 },
    .cc_curl = { 30, 31, 32, 33, 34 },
    .cc_vibrato = 35,
    .cc_mod = 1,
    .haptic_waveform = {
        HAPTIC_WAVEFORM_CLICK,      /* 0: tap */
        HAPTIC_WAVEFORM_BUMP,       /* 1: press */
        0,                          /* 2: release — no haptic */
        HAPTIC_WAVEFORM_SOFT_BUZZ,  /* 3: pluck */
        HAPTIC_WAVEFORM_CLICK,      /* 4: strum down */
        HAPTIC_WAVEFORM_CLICK,      /* 5: strum up */
        0,                          /* 6: vibrato — no haptic */
        HAPTIC_WAVEFORM_CLICK,      /* 7: tremolo */
        0,                          /* 8: glide — no haptic */
        HAPTIC_WAVEFORM_RAMP,       /* 9: fist close */
        0,                          /* 10: open hand — no haptic */
        HAPTIC_WAVEFORM_STRONG,     /* 11: snap */
    },
    .emg_threshold = 0x4000,        /* Q15 ≈ 0.25 */
    .tap_accel_threshold = 200,     /* ~12.5 m/s² (Q8) */
    .vibrato_sensitivity = 5,
    .reserved = {0},
    .crc32 = 0,
};

/* -------------------------------------------------------------------------
 * Clock initialization
 * Start HFXO (32 MHz) and LFXO (32.768 kHz) for BLE timing
 * Author: jayis1
 * ------------------------------------------------------------------------- */
static void clock_init(void)
{
    /* Start HFXO (32 MHz external crystal) */
    CLOCK->TASKS_HFCLKSTART = 1;
    while ((CLOCK->HFCLKSTAT & CLOCK_HFCLKSTAT_STATE_Msk) == 0)
        ;

    /* Start LFXO (32.768 kHz for RTC / BLE sleep timing) */
    CLOCK->LFCLKSRC = CLOCK_LFCLKSRC_LFXO;
    CLOCK->TASKS_LFCLKSTART = 1;
    while (CLOCK->EVENTS_LFCLKSTARTED == 0)
        ;
    CLOCK->EVENTS_LFCLKSTARTED = 0;
}

/* -------------------------------------------------------------------------
 * GPIO initialization
 * Configure all SPI CS, I²C, LED, button, and power pins
 * Author: jayis1
 * ------------------------------------------------------------------------- */
static void gpio_init(void)
{
    /* Configure SPI0 pins (IMU bus) — SCK, MOSI, MISO are set by SPIM driver */
    P0->PIN_CNF[PIN_SPIM0_SCK]  = GPIO_CNF_DIR_INPUT | GPIO_CNF_INPUT_DISCONNECT;
    P0->PIN_CNF[PIN_SPIM0_MOSI] = GPIO_CNF_DIR_INPUT | GPIO_CNF_INPUT_DISCONNECT;
    P0->PIN_CNF[PIN_SPIM0_MISO] = GPIO_CNF_DIR_INPUT | GPIO_CNF_INPUT_DISCONNECT;

    /* IMU chip selects — output, default high (deselected) */
    for (int i = 0; i < NUM_IMUS; i++) {
        uint32_t pin = PIN_IMU_CS0 + i;
        P0->PIN_CNF[pin] = GPIO_CNF_DIR_OUTPUT | GPIO_CNF_S0S1;
        P0->OUTSET = (1U << pin);
    }

    /* SPI1 pins (EMG bus) */
    P0->PIN_CNF[PIN_SPIM1_SCK]  = GPIO_CNF_DIR_INPUT | GPIO_CNF_INPUT_DISCONNECT;
    P0->PIN_CNF[PIN_SPIM1_MOSI] = GPIO_CNF_DIR_INPUT | GPIO_CNF_INPUT_DISCONNECT;
    P0->PIN_CNF[PIN_SPIM1_MISO] = GPIO_CNF_DIR_INPUT | GPIO_CNF_INPUT_DISCONNECT;

    /* EMG chip selects — output, default high */
    for (int i = 0; i < 3; i++) {
        uint32_t pin = PIN_EMG_CS0 + i;
        P0->PIN_CNF[pin] = GPIO_CNF_DIR_OUTPUT | GPIO_CNF_S0S1;
        P0->OUTSET = (1U << pin);
    }

    /* EMG DRDY — input with pull-up */
    P0->PIN_CNF[PIN_EMG_DRDY] = GPIO_CNF_DIR_INPUT | GPIO_CNF_PULL_UP;

    /* I²C pins — set by TWIM driver, but configure as disconnected here */
    P0->PIN_CNF[PIN_TWIM0_SCL] = GPIO_CNF_DIR_INPUT | GPIO_CNF_PULL_UP;
    P0->PIN_CNF[PIN_TWIM0_SDA] = GPIO_CNF_DIR_INPUT | GPIO_CNF_PULL_UP;

    /* Haptic enable — output, default low (LRAs off) */
    P0->PIN_CNF[PIN_HAPTIC_EN] = GPIO_CNF_DIR_OUTPUT | GPIO_CNF_S0S1;
    P0->OUTCLR = (1U << PIN_HAPTIC_EN);

    /* LED — output, default off */
    P0->PIN_CNF[PIN_LED_STATUS] = GPIO_CNF_DIR_OUTPUT | GPIO_CNF_S0S1;
    P0->OUTCLR = (1U << PIN_LED_STATUS);

    /* Button — input with pull-up, sense for wake-up */
    P0->PIN_CNF[PIN_BUTTON_MODE] = GPIO_CNF_DIR_INPUT |
                                    GPIO_CNF_PULL_UP |
                                    GPIO_CNF_SENSE_LOW;

    /* Charge status — input with pull-up */
    P1->PIN_CNF[PIN_CHG_STAT] = GPIO_CNF_DIR_INPUT | GPIO_CNF_PULL_UP;

    /* Ship mode FET — output, default low (normal operation) */
    P1->PIN_CNF[PIN_SHIP_MODE] = GPIO_CNF_DIR_OUTPUT | GPIO_CNF_S0S1;
    P1->OUTCLR = (1U << PIN_SHIP_MODE);
}

/* -------------------------------------------------------------------------
 * TIMER0 — 500 Hz sampling tick (2 ms period)
 * Generates the master sample clock for the entire sensor pipeline.
 * Author: jayis1
 * ------------------------------------------------------------------------- */
static void timer0_init(void)
{
    TIMER0->TASKS_STOP = 1;
    TIMER0->TASKS_CLEAR = 1;
    TIMER0->MODE = TIMER_MODE_TIMER;
    TIMER0->BITMODE = TIMER_BITMODE_32BIT;
    /* Prescaler: 32 MHz / 2^PRESCALER. PRESCALER=0 → 32 MHz tick.
       CC[0] = 64000 for 2 ms period (32 MHz × 2 ms = 64000). */
    TIMER0->PRESCALER = 0;
    TIMER0->CC[0] = HFXO_FREQ_HZ / SAMPLE_RATE_HZ;  /* 64000 */
    TIMER0->INTENSET = TIMER_INTENSET_CC0;
    TIMER0->SHORTS = (1U << 0);  /* AUTO-clear on CC[0] compare */
    NVIC_ICPR0 = (1U << IRQ_TIMER0);
    NVIC_ISER0 = (1U << IRQ_TIMER0);
    TIMER0->TASKS_START = 1;
}

/* -------------------------------------------------------------------------
 * TIMER0 interrupt — 500 Hz sample tick
 * Author: jayis1
 * ------------------------------------------------------------------------- */
void TIMER0_IRQHandler(void)
{
    if (TIMER0->EVENTS_COMPARE[0]) {
        TIMER0->EVENTS_COMPARE[0] = 0;
        g_tick_500hz++;
        g_time_ms += SAMPLE_PERIOD_MS;
        g_sample_ready = 1;
    }
}

/* -------------------------------------------------------------------------
 * Load calibration and mapping from flash
 * Author: jayis1
 * ------------------------------------------------------------------------- */
static void load_config(void)
{
    /* Load calibration */
    if (storage_load_calibration(&g_calib) != 0) {
        /* No calibration — use neutral defaults */
        memset(&g_calib, 0, sizeof(g_calib));
        g_calib.magic = CALIBRATION_MAGIC;
        for (int i = 0; i < NUM_EMG_CHANNELS; i++) {
            g_calib.emg_baseline[i] = 0;
            g_calib.emg_mvc[i] = 0x7FFF;
        }
    }

    /* Load mapping */
    if (storage_load_mapping(&g_mapping) != 0) {
        memcpy(&g_mapping, &default_mapping, sizeof(mapping_t));
    }
}

/* -------------------------------------------------------------------------
 * State machine transition
 * Author: jayis1
 * ------------------------------------------------------------------------- */
static void enter_state(system_state_t new_state)
{
    system_state_t old = g_state;
    (void)old;
    g_state = new_state;

    switch (new_state) {
    case STATE_BOOT:
        break;
    case STATE_CALIBRATING:
        /* Calibration mode — sensors on, BLE advertising for app */
        imu_enable(true);
        emg_enable(true);
        ble_midi_advertise(true);
        break;
    case STATE_IDLE:
        /* Connected but not playing — sensors on at reduced rate */
        imu_enable(true);
        emg_enable(true);
        break;
    case STATE_PLAYING:
        /* Full-rate sampling + gesture + MIDI output */
        imu_enable(true);
        emg_enable(true);
        haptic_enable(true);
        break;
    case STATE_CHARGING:
        /* Charging — sensors off, BLE on for status */
        imu_enable(false);
        emg_enable(false);
        haptic_enable(false);
        break;
    case STATE_SHUTDOWN:
        /* Graceful shutdown — save state, disconnect BLE */
        ble_midi_disconnect();
        imu_enable(false);
        emg_enable(false);
        haptic_enable(false);
        storage_save_calibration(&g_calib);
        storage_save_mapping(&g_mapping);
        break;
    case STATE_SHIP:
        /* Ship mode — cut battery via FET */
        P1->OUTSET = (1U << PIN_SHIP_MODE);
        break;
    }
}

/* -------------------------------------------------------------------------
 * Process one 500 Hz sample: read sensors → extract features → infer gesture
 * Author: jayis1
 * ------------------------------------------------------------------------- */
static void process_sample(void)
{
    int ret;

    /* Read all 6 IMUs via SPI round-robin */
    ret = imu_read_all(g_imu_data);
    if (ret != 0) return;

    /* Read 5-channel EMG via SPI */
    ret = emg_read(&g_emg_data);
    if (ret != 0) return;

    /* Extract features from raw sensor data */
    signal_extract_features(&g_imu_data[0], &g_emg_data, &g_calib,
                            &g_features);

    /* Run TCN inference every 10th sample (20 ms / 50 Hz) */
    if ((g_tick_500hz % TCN_INFERENCE_DIV) == 0) {
        gesture_infer(&g_features, &g_gesture);

        /* If a gesture was detected, emit MIDI and trigger haptic */
        if (g_gesture.event_type != GESTURE_EVENT_NONE) {
            emit_midi(&g_gesture);

            /* Trigger haptic feedback for this gesture */
            uint8_t wf = g_mapping.haptic_waveform[g_gesture.gesture_id];
            if (wf != HAPTIC_WAVEFORM_NONE && g_gesture.finger < NUM_HAPTIC) {
                g_haptic_pending[g_gesture.finger] = wf;
            }
        }

        /* Always emit OSC feature stream (for visualization / OSC apps) */
        emit_osc(&g_features, &g_gesture);
    }

    /* Process pending haptic commands */
    for (int i = 0; i < NUM_HAPTIC; i++) {
        if (g_haptic_pending[i] != 0) {
            haptic_trigger(i, g_haptic_pending[i]);
            g_haptic_pending[i] = 0;
        }
    }
}

/* -------------------------------------------------------------------------
 * Emit MIDI events based on gesture result
 * Author: jayis1
 * ------------------------------------------------------------------------- */
static void emit_midi(const gesture_result_t *gest)
{
    midi_event_t evt;
    evt.channel = g_mapping.midi_channel;
    evt.timestamp = g_time_ms;

    switch (gest->event_type) {
    case GESTURE_EVENT_NOTE_ON:
        evt.status = MIDI_STATUS_NOTE_ON;
        evt.data1 = g_mapping.notes[gest->finger % NUM_FINGERS];
        evt.data2 = gest->velocity;  /* 1-127, from TCN regression */
        midi_ring_push(&g_midi_ring[0], &g_midi_head, &g_midi_tail,
                       &evt, MIDI_RING_SIZE);
        break;

    case GESTURE_EVENT_NOTE_OFF:
        evt.status = MIDI_STATUS_NOTE_OFF;
        evt.data1 = g_mapping.notes[gest->finger % NUM_FINGERS];
        evt.data2 = 64;  /* release velocity */
        midi_ring_push(&g_midi_ring[0], &g_midi_head, &g_midi_tail,
                       &evt, MIDI_RING_SIZE);
        break;

    case GESTURE_EVENT_CC:
        evt.status = MIDI_STATUS_CC;
        /* CC number depends on the gesture subtype */
        if (gest->gesture_id == GESTURE_VIBRATO) {
            evt.data1 = g_mapping.cc_vibrato;
        } else if (gest->gesture_id == GESTURE_TREMOLO) {
            evt.data1 = g_mapping.cc_vibrato;  /* reuse or custom */
        } else {
            /* Per-finger curl CC */
            evt.data1 = g_mapping.cc_curl[gest->finger % NUM_FINGERS];
        }
        evt.data2 = gest->pressure;  /* 0-127 */
        midi_ring_push(&g_midi_ring[0], &g_midi_head, &g_midi_tail,
                       &evt, MIDI_RING_SIZE);
        break;

    case GESTURE_EVENT_PITCH_BEND:
        evt.status = MIDI_STATUS_PITCH_BEND;
        /* Pitch bend from wrist tilt — 14-bit value */
        evt.data1 = (uint8_t)(gest->pitch_bend & 0x7F);
        evt.data2 = (uint8_t)((gest->pitch_bend >> 7) & 0x7F);
        midi_ring_push(&g_midi_ring[0], &g_midi_head, &g_midi_tail,
                       &evt, MIDI_RING_SIZE);
        break;

    case GESTURE_EVENT_PROGRAM_CHANGE:
        evt.status = MIDI_STATUS_PROGRAM_CHANGE;
        evt.data1 = gest->program;
        evt.data2 = 0;
        midi_ring_push(&g_midi_ring[0], &g_midi_head, &g_midi_tail,
                       &evt, MIDI_RING_SIZE);
        break;

    case GESTURE_EVENT_CHANNEL_PRESSURE:
        evt.status = MIDI_STATUS_CHANNEL_PRESSURE;
        evt.data1 = gest->pressure;
        evt.data2 = 0;
        midi_ring_push(&g_midi_ring[0], &g_midi_head, &g_midi_tail,
                       &evt, MIDI_RING_SIZE);
        break;

    default:
        break;
    }

    /* Flush MIDI ring buffer to BLE-MIDI */
    ble_midi_flush_ring(g_midi_ring, &g_midi_head, &g_midi_tail,
                        MIDI_RING_SIZE, g_time_ms);

    /* Also send via USB-MIDI if connected */
    if (usb_is_connected()) {
        usb_midi_send(&evt);
    }
}

/* -------------------------------------------------------------------------
 * Emit OSC data over BLE GATT
 * Author: jayis1
 * ------------------------------------------------------------------------- */
static void emit_osc(const feature_vector_t *feat,
                     const gesture_result_t *gest)
{
    osc_bundle_t bundle;
    osc_bundle_init(&bundle, g_time_ms);

    /* Per-finger curl and velocity */
    for (int i = 0; i < NUM_FINGERS; i++) {
        osc_bundle_add_float(&bundle, "/synthand/finger/curl", i,
                             feat->finger_curl[i]);
        osc_bundle_add_float(&bundle, "/synthand/finger/velocity", i,
                             feat->finger_velocity[i]);
    }

    /* EMG envelopes */
    for (int i = 0; i < NUM_EMG_CHANNELS; i++) {
        osc_bundle_add_float(&bundle, "/synthand/emg", i,
                             feat->emg_envelope[i]);
    }

    /* Wrist quaternion */
    osc_bundle_add_quat(&bundle, "/synthand/wrist/quaternion",
                        feat->wrist_quat);

    /* Gesture event (if any) */
    if (gest->event_type != GESTURE_EVENT_NONE) {
        osc_bundle_add_gesture(&bundle, "/synthand/gesture",
                               gest->gesture_id, gest->confidence);
    }

    osc_bundle_send(&bundle);
}

/* -------------------------------------------------------------------------
 * Button handler — cycle modes or trigger calibration
 * Author: jayis1
 * ------------------------------------------------------------------------- */
static void handle_button(void)
{
    static uint32_t last_press_ms = 0;
    static uint8_t  press_count = 0;

    /* Read button (active low) */
    if ((P0->IN & (1U << PIN_BUTTON_MODE)) == 0) {
        /* Debounce */
        if ((g_time_ms - last_press_ms) > 200) {
            last_press_ms = g_time_ms;
            press_count++;

            if (press_count == 1) {
                /* Single press: toggle play/idle */
                if (g_state == STATE_IDLE) {
                    enter_state(STATE_PLAYING);
                } else if (g_state == STATE_PLAYING) {
                    enter_state(STATE_IDLE);
                }
            } else if (press_count >= 3) {
                /* Triple press: enter calibration mode */
                enter_state(STATE_CALIBRATING);
                press_count = 0;
            }
        }
    } else {
        /* Button released — reset counter after timeout */
        if ((g_time_ms - last_press_ms) > 1000) {
            press_count = 0;
        }
    }
}

/* -------------------------------------------------------------------------
 * LED status indicator
 * Author: jayis1
 * ------------------------------------------------------------------------- */
static void update_led(void)
{
    static uint32_t last_toggle = 0;
    static uint8_t  led_on = 0;

    switch (g_state) {
    case STATE_BOOT:
        /* Solid on during boot */
        P0->OUTSET = (1U << PIN_LED_STATUS);
        break;
    case STATE_CALIBRATING:
        /* Fast blink (100 ms) during calibration */
        if ((g_time_ms - last_toggle) > 100) {
            led_on = !led_on;
            last_toggle = g_time_ms;
            if (led_on) P0->OUTSET = (1U << PIN_LED_STATUS);
            else        P0->OUTCLR = (1U << PIN_LED_STATUS);
        }
        break;
    case STATE_IDLE:
        /* Slow blink (2 s) when idle */
        if ((g_time_ms - last_toggle) > 2000) {
            led_on = !led_on;
            last_toggle = g_time_ms;
            if (led_on) P0->OUTSET = (1U << PIN_LED_STATUS);
            else        P0->OUTCLR = (1U << PIN_LED_STATUS);
        }
        break;
    case STATE_PLAYING:
        /* Solid on when playing */
        P0->OUTSET = (1U << PIN_LED_STATUS);
        break;
    case STATE_CHARGING:
        /* Pulsing (500 ms) when charging */
        if ((g_time_ms - last_toggle) > 500) {
            led_on = !led_on;
            last_toggle = g_time_ms;
            if (led_on) P0->OUTSET = (1U << PIN_LED_STATUS);
            else        P0->OUTCLR = (1U << PIN_LED_STATUS);
        }
        break;
    case STATE_SHUTDOWN:
    case STATE_SHIP:
        P0->OUTCLR = (1U << PIN_LED_STATUS);
        break;
    }
}

/* -------------------------------------------------------------------------
 * System halt — unrecoverable error
 * Author: jayis1
 * ------------------------------------------------------------------------- */
static void system_halt(void)
{
    /* Blink LED rapidly to indicate fault */
    while (1) {
        P0->OUTSET = (1U << PIN_LED_STATUS);
        for (volatile int i = 0; i < 100000; i++);
        P0->OUTCLR = (1U << PIN_LED_STATUS);
        for (volatile int i = 0; i < 100000; i++);
    }
}

/* -------------------------------------------------------------------------
 * Main entry point
 * Author: jayis1
 * ------------------------------------------------------------------------- */
int main(void)
{
    /* Initialize clocks */
    clock_init();

    /* Configure SysTick for 1 ms tick (used by BLE stack timing) */
    SysTick->LOAD = (HFXO_FREQ_HZ / 1000) - 1;
    SysTick->VAL = 0;
    SysTick->CTRL = SysTick_CTRL_ENABLE | SysTick_CTRL_TICKINT |
                    SysTick_CTRL_CLKSOURCE;

    /* Initialize GPIO */
    gpio_init();

    /* Initialize storage (flash config) */
    storage_init();
    load_config();

    /* Initialize power management */
    power_init();
    uint32_t bat_mv = power_read_battery_mv();
    if (bat_mv < BAT_CRIT_MV) {
        /* Battery critically low on boot — go to ship mode */
        enter_state(STATE_SHIP);
        system_halt();  /* should not return */
    }

    /* Initialize sensors */
    if (imu_init() != 0) {
        system_halt();
    }
    if (emg_init() != 0) {
        system_halt();
    }

    /* Initialize haptic drivers */
    if (haptic_init() != 0) {
        system_halt();
    }

    /* Initialize signal processing */
    signal_init(&g_calib);

    /* Initialize gesture recognition (load TCN model) */
    gesture_init();
    tcn_model_init();

    /* Initialize BLE-MIDI */
    ble_midi_init();
    ble_midi_advertise(true);

    /* Initialize USB */
    usb_init();

    /* Initialize OSC */
    osc_init();

    /* Start 500 Hz sampling timer */
    timer0_init();

    /* Enter idle state (will transition to PLAYING on button press or
       BLE command from app) */
    enter_state(STATE_IDLE);

    /* Enable SEVONPEND so we wake from WFE on any interrupt */
    SCB_SCR |= SCB_SCR_SEVONPEND;

    /* -----------------------------------------------------------------
     * Main loop — WFE sleep, process samples when ready
     * Author: jayis1
     * --------------------------------------------------------------- */
    uint32_t last_battery_check = 0;
    uint32_t last_temp_check = 0;

    while (1) {
        /* Wait for event (interrupt wakes us) */
        __asm volatile ("wfe");

        /* Process sensor sample if ready */
        if (g_sample_ready) {
            g_sample_ready = 0;
            if (g_state == STATE_PLAYING || g_state == STATE_CALIBRATING) {
                process_sample();
            }
        }

        /* Handle button presses */
        handle_button();

        /* Update LED status */
        update_led();

        /* Battery check every 10 seconds */
        if ((g_time_ms - last_battery_check) > 10000) {
            last_battery_check = g_time_ms;
            bat_mv = power_read_battery_mv();
            if (bat_mv < BAT_LOW_MV && g_state != STATE_CHARGING) {
                /* Low battery haptic alert */
                g_haptic_pending[0] = HAPTIC_WAVEFORM_DOUBLE;
                g_haptic_pending[1] = HAPTIC_WAVEFORM_DOUBLE;
            }
            if (bat_mv < BAT_CRIT_MV) {
                enter_state(STATE_SHUTDOWN);
                /* After saving, go to ship mode */
                for (volatile int i = 0; i < 1000000; i++);
                enter_state(STATE_SHIP);
            }

            /* Check charging status */
            if ((P1->IN & (1U << PIN_CHG_STAT)) == 0) {
                /* Charging — not charging */
                if (g_state == STATE_PLAYING || g_state == STATE_IDLE) {
                    enter_state(STATE_CHARGING);
                }
            } else {
                /* Not charging — return to idle if was charging */
                if (g_state == STATE_CHARGING) {
                    enter_state(STATE_IDLE);
                }
            }
        }

        /* Temperature check every 5 seconds */
        if ((g_time_ms - last_temp_check) > 5000) {
            last_temp_check = g_time_ms;
            int32_t temp_mc = power_read_temp_mc();
            if (temp_mc > TEMP_ALERT_MC) {
                /* Thermal alert — haptic double-click on all fingers */
                for (int i = 0; i < NUM_HAPTIC; i++) {
                    g_haptic_pending[i] = HAPTIC_WAVEFORM_DOUBLE;
                }
            }
            if (temp_mc > TEMP_SHUTDOWN_MC) {
                enter_state(STATE_SHUTDOWN);
                enter_state(STATE_SHIP);
            }
        }

        /* Process haptic queue */
        for (int i = 0; i < NUM_HAPTIC; i++) {
            if (g_haptic_pending[i] != 0) {
                haptic_trigger(i, g_haptic_pending[i]);
                g_haptic_pending[i] = 0;
            }
        }
    }

    return 0;  /* never reached */
}

/* -------------------------------------------------------------------------
 * SysTick handler — 1 ms system tick
 * Author: jayis1
 * ------------------------------------------------------------------------- */
void SysTick_Handler(void)
{
    /* SysTick is used by BLE stack timing; the main timekeeping is done
       by the 500 Hz TIMER0 IRQ. SysTick just provides a 1 ms reference
       for the SoftDevice. */
}

/*
 * Author: jayis1
 * End of main.c
 */