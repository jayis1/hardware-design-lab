/*
 * main.c — SoundLoom firmware entry point
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 *
 * SoundLoom: Bioacoustic Soil Health Monitor
 * Target: STM32H733VIH6 (Cortex-M7, 480 MHz)
 *
 * The main loop implements a duty-cycled state machine:
 *   SLEEP → WAKE_ENV → WAKE_ACQ → LISTEN → PROCESS → TELEMETRY → SLEEP
 *
 * During LISTEN (30 seconds at default cadence), the 12-channel acoustic
 * acquisition chain streams data through DSP, event detection, edge AI
 * classification, and beamforming. At the end of each cycle, the SVI is
 * computed and a LoRaWAN uplink is sent with the 51-byte payload.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. MIT License.
 */

#include "board.h"
#include "registers.h"
#include "acq.h"
#include "dsp.h"
#include "classifier.h"
#include "beamformer.h"
#include "env_sensors.h"
#include "lorawan.h"
#include "svi.h"
#include <string.h>
#include <math.h>

/* ---- HAL stubs (in production these come from STM32Cube HAL) ---- */

void HAL_Delay(uint32_t ms) { (void)ms; }
void HAL_DelayMicroseconds(uint32_t us) { (void)us; }
uint32_t HAL_GetTick(void) { static uint32_t t = 0; return t++; }
void HAL_GPIO_WritePin(void *p, uint16_t pin, uint8_t v) { (void)p; (void)pin; (void)v; }
uint8_t HAL_GPIO_ReadPin(void *p, uint16_t pin) { (void)p; (void)pin; return 0; }
void HAL_GPIO_Init(void *p, void *gp) { (void)p; (void)gp; }
void HAL_SPI_Init(void *h) { (void)h; }
HAL_StatusTypeDef HAL_SPI_TransmitReceive(void *h, uint8_t *tx, uint8_t *rx, uint16_t n, uint32_t t)
    { (void)h; (void)tx; (void)rx; (void)n; (void)t; return 0; }
HAL_StatusTypeDef HAL_SPI_TransmitReceive_DMA(void *h, uint8_t *tx, uint8_t *rx, uint16_t n)
    { (void)h; (void)tx; (void)rx; (void)n; return 0; }
HAL_StatusTypeDef HAL_SPI_DMAStop(void *h) { (void)h; return 0; }
void HAL_DMA_Init(void *h) { (void)h; }
void HAL_DMA_IRQHandler(void *h) { (void)h; }
void HAL_UART_Init(void *h) { (void)h; }
HAL_StatusTypeDef HAL_UART_Transmit(void *h, uint8_t *d, uint16_t n, uint32_t t)
    { (void)h; (void)d; (void)n; (void)t; return 0; }
HAL_StatusTypeDef HAL_UART_Receive(void *h, uint8_t *d, uint16_t n, uint32_t t)
    { (void)h; (void)d; (void)n; (void)t; return 0; }
HAL_StatusTypeDef HAL_I2C_Master_Transmit(void *h, uint16_t a, uint8_t *d, uint16_t n, uint32_t t)
    { (void)h; (void)a; (void)d; (void)n; (void)t; return 0; }
HAL_StatusTypeDef HAL_I2C_Master_Receive(void *h, uint16_t a, uint8_t *d, uint16_t n, uint32_t t)
    { (void)h; (void)a; (void)d; (void)n; (void)t; return 0; }
HAL_StatusTypeDef HAL_I2C_Init(void *h) { (void)h; return 0; }
void HAL_ADC_Init(void *h) { (void)h; }
HAL_StatusTypeDef HAL_ADC_ConfigChannel(void *h, void *c) { (void)h; (void)c; return 0; }
HAL_StatusTypeDef HAL_ADC_Start(void *h) { (void)h; return 0; }
HAL_StatusTypeDef HAL_ADC_PollForConversion(void *h, uint32_t t) { (void)h; (void)t; return 0; }
uint32_t HAL_ADC_GetValue(void *h) { (void)h; return 0; }
HAL_StatusTypeDef HAL_ADC_Stop(void *h) { (void)h; return 0; }

/* Stub UART/I2C/SPI/ADC handles */
UART_HandleTypeDef huart_sdi12;
I2C_HandleTypeDef  hi2c1;
SPI_HandleTypeDef  hspi_lora;
SPI_HandleTypeDef  hspi3;
ADC_HandleTypeDef  hadc1;

/* ---- State machine ---- */

typedef enum {
    STATE_BOOT = 0,
    STATE_SLEEP,
    STATE_WAKE_ENV,
    STATE_WAKE_ACQ,
    STATE_LISTEN,
    STATE_PROCESS,
    STATE_TELEMETRY,
    STATE_ERROR
} system_state_t;

static system_state_t sys_state = STATE_BOOT;
static uint32_t state_entry_tick = 0;
static uint32_t cycle_count = 0;
static uint32_t listen_window_ms = LISTEN_WINDOW_MS;
static uint32_t cadence_ms = DEFAULT_CADENCE_MS;

/* Event log: stores recent classified events for SD card and telemetry */
typedef struct {
    uint32_t timestamp;
    uint8_t  class_id;
    uint8_t  channel;
    float    confidence;
    float    pos_x, pos_y, pos_z;  /* beamformer localisation */
    float    residual;
} event_log_entry_t;

static event_log_entry_t event_log[LOG_BUF_EVENTS];
static uint32_t event_log_idx = 0;

/* Acoustic sample buffer for DSP */
static int32_t sample_buf[ADC_USED_CHS];

/* Multi-channel block buffer for beamformer (filled during listen window) */
static float bf_channels[BF_NUM_RECEIVERS][BF_BLOCK_LEN];
static uint32_t bf_sample_count = 0;

/* ---- Error handler ---- */

static void error_handler(void)
{
    sys_state = STATE_ERROR;
    while (1) {
        /* In production: blink fault LED, log to SD, attempt recovery */
        HAL_Delay(1000);
    }
}

/* ---- System initialisation ---- */

static void system_init(void)
{
    /* HAL init, clock tree, cache enable (production code omitted for brevity) */

    /* Initialise subsystems */
    int rc;

    rc = acq_init();
    if (rc != ACQ_OK) error_handler();

    dsp_init();
    cls_init();
    bf_init();
    env_init();

    rc = lora_init();
    if (rc != LORA_OK) {
        /* LoRa is not fatal; continue without telemetry */
    }

    svi_init();

    sys_state = STATE_SLEEP;
    state_entry_tick = HAL_GetTick();
}

/* ---- Listen window: process acoustic data ---- */

static void listen_process(void)
{
    /* Poll acquisition for new samples */
    int n = acq_poll(sample_buf);
    if (n > 0) {
        /* Feed DSP pipeline */
        dsp_process(sample_buf, ADC_USED_CHS);

        /* Accumulate samples into beamformer channel buffers */
        if (bf_sample_count < BF_BLOCK_LEN) {
            for (int c = 0; c < BF_NUM_RECEIVERS && c < ADC_USED_CHS; c++) {
                bf_channels[c][bf_sample_count] = (float)sample_buf[c] / (float)(1 << 23);
            }
            bf_sample_count++;
        }

        /* Check for detected events */
        dsp_event_t ev;
        while (dsp_get_event(&ev)) {
            /* Classify the event */
            float confidence = 0.0f;
            int cls = cls_classify_event(ev.features, CLS_FRAMES * CLS_MEL_BINS, &confidence);

            if (cls >= 0 && cls < CLS_NUM_CLASSES) {
                /* Record in event log */
                if (event_log_idx < LOG_BUF_EVENTS) {
                    event_log_entry_t *entry = &event_log[event_log_idx++];
                    entry->timestamp = HAL_GetTick();
                    entry->class_id = (uint8_t)cls;
                    entry->channel = ev.channel;
                    entry->confidence = confidence;
                    entry->pos_x = 0.0f;
                    entry->pos_y = 0.0f;
                    entry->pos_z = 0.0f;
                    entry->residual = 0.0f;
                }

                /* Update DSP class counters for SVI */
                dsp_record_class(cls);

                /* Attempt beamformer localisation if we have enough samples */
                if (bf_sample_count >= BF_BLOCK_LEN) {
                    bf_pos_t pos;
                    float residual;
                    int bf_rc = bf_localise((const float (*)[BF_BLOCK_LEN])bf_channels,
                                           BF_NUM_RECEIVERS, BF_BLOCK_LEN,
                                           (float)ADC_SAMPLE_RATE, &pos, &residual);
                    if (bf_rc == 0 && event_log_idx > 0) {
                        event_log_entry_t *last = &event_log[event_log_idx - 1];
                        last->pos_x = pos.x;
                        last->pos_y = pos.y;
                        last->pos_z = pos.z;
                        last->residual = residual;
                    }
                    bf_sample_count = 0;  /* reset buffer after localisation attempt */
                }
            }
        }
    }
}

/* ---- Process cycle: compute SVI and build uplink ---- */

static void process_cycle(lora_uplink_t *uplink)
{
    /* Read environmental sensors */
    env_data_t env;
    env_read_all(&env);

    /* Get event rates per class */
    float rates[CLS_NUM_CLASSES];
    dsp_get_event_rates(rates, 15);  /* 15-minute window */

    /* Compute spectral diversity (averaged across channels) */
    float diversity = 0.0f;
    for (int c = 0; c < ADC_USED_CHS; c++) {
        diversity += dsp_spectral_entropy((uint8_t)c);
    }
    diversity /= (float)ADC_USED_CHS;

    /* Compute SVI */
    uint8_t svi = svi_compute(rates, diversity, rates[9], &env);

    /* Get alert flags */
    uint16_t flags = svi_get_flags(rates, env.battery_mv);

    /* Fill uplink struct */
    memset(uplink, 0, sizeof(*uplink));
    uplink->svi = svi;
    for (int i = 0; i < CLS_NUM_CLASSES; i++) {
        uplink->event_counts[i] = (uint16_t)(rates[i] * 1000.0f);  /* fixed-point */
    }
    uplink->moisture[0] = env.moisture[0];
    uplink->moisture[1] = env.moisture[1];
    uplink->ec[0] = env.ec[0];
    uplink->ec[1] = env.ec[1];
    for (int i = 0; i < 4; i++) uplink->depth_temp[i] = env.depth_temp[i];
    uplink->co2_ppm = env.co2_ppm;
    uplink->battery_mv = env.battery_mv;
    uplink->flags = flags;

    /* Reset DSP counters for next cycle */
    dsp_reset_counters();
}

/* ---- Log events to SD card (stub) ---- */

static void log_events_to_sd(void)
{
    /* In production: write event_log[0..event_log_idx] to FatFS on microSD.
     * Format: CSV with timestamp, class_id, channel, confidence, x, y, z, residual.
     * Here we just reset the index.
     */
    event_log_idx = 0;
}

/* ---- Main loop ---- */

int main(void)
{
    system_init();

    lora_uplink_t uplink;

    while (1) {
        uint32_t now = HAL_GetTick();
        uint32_t elapsed = now - state_entry_tick;

        switch (sys_state) {

        case STATE_BOOT:
            /* Should not reach here after system_init() */
            sys_state = STATE_SLEEP;
            state_entry_tick = now;
            break;

        case STATE_SLEEP:
            /* Low-power sleep until next cadence interval.
             * In production: enter STOP2 mode, wake via RTC alarm.
             * Here we just simulate the delay.
             */
            if (elapsed >= cadence_ms) {
                sys_state = STATE_WAKE_ENV;
                state_entry_tick = now;
            }
            break;

        case STATE_WAKE_ENV:
            /* Power on sensor rail, read environmental sensors.
             * This state runs for ~10 seconds before acoustic listen.
             */
            env_read_all(NULL);  /* pre-read to stabilise sensors */
            sys_state = STATE_WAKE_ACQ;
            state_entry_tick = now;
            break;

        case STATE_WAKE_ACQ:
            /* Power on ADC front-end, start acquisition */
            acq_start();
            bf_sample_count = 0;
            sys_state = STATE_LISTEN;
            state_entry_tick = now;
            break;

        case STATE_LISTEN:
            /* Active listening window: process acoustic data continuously */
            listen_process();
            if (elapsed >= listen_window_ms) {
                acq_stop();
                sys_state = STATE_PROCESS;
                state_entry_tick = now;
            }
            break;

        case STATE_PROCESS:
            /* Compute SVI, classify events, build uplink payload */
            process_cycle(&uplink);
            log_events_to_sd();
            sys_state = STATE_TELEMETRY;
            state_entry_tick = now;
            break;

        case STATE_TELEMETRY:
            /* Send LoRaWAN uplink */
            lora_send_uplink(&uplink);
            cycle_count++;
            /* Return to sleep */
            sys_state = STATE_SLEEP;
            state_entry_tick = now;
            break;

        case STATE_ERROR:
        default:
            error_handler();
            break;
        }
    }
}

/* ---- Interrupt handlers (production: STM32 HAL handles these) ---- */

void HardFault_Handler(void)
{
    while (1) { /* fault: blink LED in production */ }
}

void MemManage_Handler(void)
{
    while (1) {}
}

void BusFault_Handler(void)
{
    while (1) {}
}

void UsageFault_Handler(void)
{
    while (1) {}
}

void SVC_Handler(void)      {}
void PendSV_Handler(void)   {}
void SysTick_Handler(void)  {}

/* ---- System configuration helpers ---- */

void system_set_cadence(uint32_t ms)
{
    if (ms >= 60000) cadence_ms = ms;  /* minimum 1 minute */
}

void system_set_listen_window(uint32_t ms)
{
    if (ms >= 5000 && ms <= 120000) listen_window_ms = ms;
}

uint32_t system_get_cycle_count(void)
{
    return cycle_count;
}

uint32_t system_get_uptime_s(void)
{
    return HAL_GetTick() / 1000u;
}

/* ---- Version info ---- */

const char* system_get_version(void)
{
    return FW_NAME " v" 
           "1.0.0";  /* FW_VERSION_MAJOR.MINOR.PATCH */
}

const char* system_get_author(void)
{
    return FW_AUTHOR;
}

/* ---- Statistics for BLE/app ---- */

typedef struct {
    uint32_t cycles;
    uint32_t uptime_s;
    uint32_t total_events;
    uint8_t  svi;
    uint8_t  battery_pct;
    int8_t   lora_rssi;
    uint16_t flags;
} system_stats_t;

system_stats_t system_get_stats(void)
{
    system_stats_t s;
    s.cycles = cycle_count;
    s.uptime_s = system_get_uptime_s();
    s.total_events = dsp_get_total_events();
    s.battery_pct = env_battery_percent();
    s.lora_rssi = lora_get_last_rssi();
    /* SVI and flags would come from last uplink */
    s.svi = 0;
    s.flags = 0;
    return s;
}