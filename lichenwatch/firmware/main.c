/*
 * main.c — Lichenwatch firmware entry point and FreeRTOS task graph.
 *
 * Author: jayis1
 * License: MIT
 *
 * Architecture
 * -------------
 * The firmware is a single-application FreeRTOS image.  The measurement
 * loop is driven by a high-priority "burst" task that runs every
 * MEASURE_INTERVAL_MIN minutes.  A lower-priority "radio" task handles
 * LoRa uplinks and BLE walk-by sessions.  An "inference" task runs the
 * CMSIS-NN state classifier on the latest feature vector.
 *
 * Power profile
 * -------------
 * Between bursts the MCU is in Stop 2 mode (~1.7 µA) and the TPL5111
 * gates the regulator entirely during the long sleep.  A burst takes
 * roughly 3 s at ~12 mA, giving an average of ~180 µA at a 15 min cadence.
 */

#include <stdint.h>
#include <string.h>

/* FreeRTOS — minimal shim so this file compiles standalone in CI.        */
/* In a real build these come from the FreeRTOS Cortex-M port.             */
#include "FreeRTOS_shim.h"

#include "board.h"
#include "registers.h"

#include "drivers/pam_drv.h"
#include "drivers/spectral_drv.h"
#include "drivers/env_drv.h"
#include "drivers/wetness_drv.h"
#include "drivers/audio_drv.h"
#include "drivers/flash_drv.h"
#include "drivers/lora_drv.h"
#include "drivers/ble_svc.h"
#include "drivers/power.h"
#include "drivers/inference.h"

/* ------------------------------------------------------------------------ */
/* Global state                                                              */
/* ------------------------------------------------------------------------ */

typedef struct {
    /* Latest measurement (filled by burst task) */
    float fv_fm;            /* PSII max quantum yield             */
    float lndvi;             /* lichen NDVI                         */
    float chl_index;        /* chlorophyll proxy                   */
    float melanin_proxy;   /* UV-screening pigment proxy          */
    float wetness;          /* 0..1 normalized thallus conductance  */
    int16_t co2_ppm;        /* ambient CO₂                         */
    int8_t  temp_c;         /* surface temperature                  */
    uint8_t rh_pct;         /* relative humidity                   */
    uint8_t uv_index_x10;  /* UV index × 10                       */
    uint16_t acoustic_events; /* desiccation clicks                */
    uint16_t battery_mv;
    uint8_t  classifier_state;  /* INFERENCE_STATE_*               */
    uint8_t  classifier_conf;  /* 0..100                          */
    uint16_t seq;           /* uplink sequence number              */
    uint32_t uptime_s;
    uint8_t  flags;
} lichenwatch_state_t;

static lichenwatch_state_t g_state;

/* Runtime configuration (mutable via downlink / BLE) */
typedef struct {
    uint8_t  measure_interval_min;
    uint8_t  uplink_interval_min;
    uint8_t  classifier_sensitivity;  /* 0..3 */
    uint8_t  lora_sf;
    char     node_id[12];
} lichenwatch_config_t;

static lichenwatch_config_t g_config = {
    .measure_interval_min = DEFAULT_MEASURE_INTERVAL_MIN,
    .uplink_interval_min  = DEFAULT_UPLINK_INTERVAL_MIN,
    .classifier_sensitivity = 1,
    .lora_sf              = LORA_SF,
    .node_id              = BOARD_NODE_ID_DEFAULT,
};

/* Queue handles */
static QueueHandle_t s_burst_q;   /* burst task -> inference task */
static QueueHandle_t s_radio_q;  /* inference task -> radio task  */

/* ------------------------------------------------------------------------ */
/* Forward declarations                                                      */
/* ------------------------------------------------------------------------ */
static void task_burst(void *arg);
static void task_inference(void *arg);
static void task_radio(void *arg);
static void task_ble(void *arg);
static void board_init(void);
static void do_measurement_burst(lichenwatch_state_t *st);
void handle_downlink(const uint8_t *buf, uint8_t len);

/* ------------------------------------------------------------------------ */
/* main                                                                       */
/* ------------------------------------------------------------------------ */
int main(void)
{
    /* Bare-metal board bring-up before the scheduler starts. */
    board_init();

    memset(&g_state, 0, sizeof(g_state));
    g_state.seq = 0;
    g_state.uptime_s = 0;

    /* Create communication queues. */
    s_burst_q  = xQueueCreate(2, sizeof(lichenwatch_state_t));
    s_radio_q  = xQueueCreate(4, sizeof(uint8_t)); /* state class to uplink */

    /* Tasks */
    xTaskCreate(task_burst,      "burst",     2048, NULL, 4, NULL);
    xTaskCreate(task_inference,  "infer",     1024, NULL, 3, NULL);
    xTaskCreate(task_radio,      "radio",     1536, NULL, 2, NULL);
    xTaskCreate(task_ble,        "ble",       1024, NULL, 1, NULL);

    vTaskStartScheduler();

    /* Should never reach here. */
    for (;;) {
        /* fall back to WFI if scheduler fails */
#ifdef ARM_CM4
        __asm volatile ("wfi");
#else
        break;
#endif
    }
}

/* ------------------------------------------------------------------------ */
/* Board initialization                                                       */
/* ------------------------------------------------------------------------ */
static void board_init(void)
{
    /* 1. Enable HSI as initial system clock (already default out of reset). */
    RCC->CR |= RCC_CR_HSION;
    while (!(RCC->CR & RCC_CR_HSIRDY)) { }

    /* 2. Enable HSE (16 MHz TCXO). */
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY)) { }

    /* 3. Configure PLL: HSE / PLLM(1) × PLLN(20) / PLLR(2) = 160 MHz
     *    We want 80 MHz system clock, so we also set AHB div by 2 via CFGR HPRE.
     *    For simplicity here we run SYSCLK = 80 MHz directly from PLLR.
     */
    RCC->PLLCFGR = RCC_PLLCFGR_PLLSRC_HSE
                 | (0 << 0)    /* PLLM = 1 (value 0 means div-by-1 in this field) */
                 | (20U << 8)  /* PLLN = 20 */
                 | (0U << 25)  /* PLLR div by 2 -> encoded 0 */
                 | RCC_PLLCFGR_PLLREN;

    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY)) { }

    /* 4. Switch SYSCLK to PLL. */
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW_MSK) | RCC_CFGR_SW_PLL;
    while (((RCC->CFGR & RCC_CFGR_SWS_MSK) >> 3) != RCC_CFGR_SW_PLL) { }

    /* 5. Enable peripheral clocks. */
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN
                  | RCC_AHB2ENR_GPIOCEN | RCC_AHB2ENR_GPIOHEN
                  | RCC_AHB2ENR_ADCEN;
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN | RCC_AHB1ENR_DMA2EN;
    RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN | RCC_APB1ENR1_TIM3EN
                   | RCC_APB1ENR1_I2C1EN | RCC_APB1ENR1_SPI2EN
                   | RCC_APB1ENR1_USART3EN | RCC_APB1ENR1_RTCAPBEN;
    RCC->APB2ENR  |= RCC_APB2ENR_SPI1EN | RCC_APB2ENR_USART1EN
                   | RCC_APB2ENR_SYSCFGEN;

    /* 6. SysTick — 1 kHz tick at 80 MHz. */
    SYSTICK->LOAD = (BOARD_SYSCLK_HZ / SYSTICK_HZ) - 1;
    SYSTICK->VAL  = 0;
    SYSTICK->CSR  = SYSTICK_CSR_CLKSRC | SYSTICK_CSR_TICKINT | SYSTICK_CSR_ENABLE;

    /* 7. Initialize driver subsystems (each configures its own GPIO/AF). */
    power_init();
    pam_init();
    spectral_init();
    env_init();
    wetness_init();
    audio_init();
    flash_init();
    lora_init(LORA_FREQ_HZ, g_config.lora_sf, LORA_BW_HZ, LORA_TX_POWER_DBM);
    ble_init();

    /* 8. Status LED blink — indicates successful boot. */
    {
        volatile uint32_t *pc = (volatile uint32_t *)GPIOC;
        /* set PC13 as output */
        pc[0] = (pc[0] & ~(0x3U << 26)) | (GPIO_MODE_OUTPUT << 26);
        for (int i = 0; i < 3; i++) {
            pc[5] = (1U << 13);   /* BSRR set   */
            vTaskDelay(100);
            pc[6] = (1U << 13);   /* BRR reset  */
            vTaskDelay(100);
        }
    }
}

/* ------------------------------------------------------------------------ */
/* Burst task — wakes every MEASURE_INTERVAL_MIN and runs a full measurement */
/* ------------------------------------------------------------------------ */
static void task_burst(void *arg)
{
    (void)arg;
    TickType_t last = xTaskGetTickCount();

    for (;;) {
        /* Run the measurement burst. */
        do_measurement_burst(&g_state);

        /* Push a copy to the inference task. */
        xQueueOverwrite(s_burst_q, &g_state);

        /* Sleep until the next interval. */
        uint16_t delay_ms = (uint16_t)g_config.measure_interval_min * 60U * 1000U;
        vTaskDelayUntil(&last, pdMS_TO_TICKS(delay_ms));
    }
}

/* ------------------------------------------------------------------------ */
/* do_measurement_burst — the heart of the instrument                         */
/* ------------------------------------------------------------------------ */
static void do_measurement_burst(lichenwatch_state_t *st)
{
    /* --- Phase 1: Dark adapt the thallus --------------------------------- */
    /* Close the optical shroud (or just turn all LEDs off and wait).       */
    pam_shroud_close();
    vTaskDelay(pdMS_TO_TICKS(PAM_DARK_ADAPT_MS));

    /* --- Phase 2: PAM fluorometry ---------------------------------------- */
    pam_result_t pam;
    if (pam_measure(&pam) == 0) {
        st->fv_fm = pam.fv_fm;
    } else {
        /* Shroud failure or photodiode saturated — mark invalid. */
        st->fv_fm = -1.0f;
        st->flags |= 0x01;  /* FLAG_PAM_FAIL */
    }

    /* --- Phase 3: Spectral reflectance ----------------------------------- */
    spectral_result_t spec;
    if (spectral_read(&spec) == 0) {
        st->lndvi          = spec.lndvi;
        st->chl_index      = spec.chl_index;
        st->melanin_proxy  = spec.melanin_proxy;
    } else {
        st->flags |= 0x02;  /* FLAG_SPECTRAL_FAIL */
    }

    /* --- Phase 4: Environmental sensors ----------------------------------- */
    env_result_t env;
    if (env_read(&env) == 0) {
        st->co2_ppm     = env.co2_ppm;
        st->temp_c      = env.temp_c;
        st->rh_pct      = env.rh_pct;
        st->uv_index_x10 = env.uv_index_x10;
    } else {
        st->flags |= 0x04;  /* FLAG_ENV_FAIL */
    }

    /* --- Phase 5: Surface wetness ---------------------------------------- */
    st->wetness = wetness_read();

    /* --- Phase 6: Contact acoustic -------------------------------------- */
    audio_result_t audio;
    audio_capture(&audio);
    st->acoustic_events = audio.click_count;

    /* --- Phase 7: Power / battery --------------------------------------- */
    st->battery_mv = power_read_battery_mv();
    st->uptime_s   = power_uptime_s();

    /* --- Phase 8: Persist raw burst to SPI flash ------------------------ */
    flash_record_t rec;
    memset(&rec, 0, sizeof(rec));
    rec.magic   = STORAGE_MAGIC;
    rec.seq     = st->seq;
    rec.fv_fm   = (int16_t)(st->fv_fm * 1000.0f);
    rec.lndvi   = (int16_t)(st->lndvi * 1000.0f);
    rec.chl_idx = (uint16_t)(st->chl_index * 100.0f);
    rec.wetness = (uint16_t)(st->wetness * 1000.0f);
    rec.co2     = st->co2_ppm;
    rec.temp    = st->temp_c;
    rec.rh      = st->rh_pct;
    rec.uv      = st->uv_index_x10;
    rec.aclicks = st->acoustic_events;
    rec.bat_mv  = st->battery_mv;
    rec.flags   = st->flags;
    rec.class_state = st->classifier_state;
    flash_append(&rec);

    /* Open the shroud so the thallus isn't kept in the dark between bursts. */
    pam_shroud_open();
}

/* ------------------------------------------------------------------------ */
/* Inference task — runs the CMSIS-NN classifier on each new feature vector   */
/* ------------------------------------------------------------------------ */
static void task_inference(void *arg)
{
    (void)arg;
    lichenwatch_state_t snap;

    for (;;) {
        if (xQueueReceive(s_burst_q, &snap, portMAX_DELAY) == pdPASS) {
            /* Build the feature vector for the classifier. */
            inference_features_t f;
            f.fv_fm         = snap.fv_fm;
            f.lndvi         = snap.lndvi;
            f.chl_index     = snap.chl_index;
            f.wetness       = snap.wetness;
            f.acoustic_events = (float)snap.acoustic_events;
            f.uv_index      = (float)snap.uv_index_x10 / 10.0f;

            uint8_t conf = 0;
            uint8_t cls = inference_run(&f, &conf);

            /* Apply sensitivity bias from config. */
            if (g_config.classifier_sensitivity >= 2 && cls == INFERENCE_STATE_HEALTHY) {
                /* At high sensitivity, downgrade borderline-healthy to stressed. */
                if (snap.fv_fm < 0.68f) cls = INFERENCE_STATE_STRESSED;
            }

            g_state.classifier_state = cls;
            g_state.classifier_conf  = conf;

            /* Enqueue for the radio task if it's an uplink window. */
            static uint32_t last_uplink_ticks = 0;
            uint32_t now = xTaskGetTickCount();
            uint32_t uplink_ms = (uint32_t)g_config.uplink_interval_min * 60U * 1000U;
            if ((now - last_uplink_ticks) >= pdMS_TO_TICKS(uplink_ms)) {
                xQueueSendToBack(s_radio_q, &cls, 0);
                last_uplink_ticks = now;
            }

            /* Alert: if bleaching or acoustic storm, uplink immediately. */
            if (cls == INFERENCE_STATE_BLEACHING ||
                snap.acoustic_events > 20) {
                uint8_t alert = 0xFE;  /* alert class */
                xQueueOverwrite(s_radio_q, &alert);
            }
        }
    }
}

/* ------------------------------------------------------------------------ */
/* Radio task — handles LoRa uplinks and downlink config changes              */
/* ------------------------------------------------------------------------ */
static void task_radio(void *arg)
{
    (void)arg;
    uint8_t class_to_send;

    for (;;) {
        if (xQueueReceive(s_radio_q, &class_to_send, portMAX_DELAY) == pdPASS) {
            /* Build the 19-byte LoRa uplink. */
            uint8_t payload[19];
            payload[0]  = (class_to_send == 0xFE) ? g_state.classifier_state : class_to_send;
            payload[1]  = (uint8_t)(g_state.seq >> 8);
            payload[2]  = (uint8_t)(g_state.seq & 0xFF);
            /* Fv/Fm × 1000 */
            int16_t fv = (int16_t)(g_state.fv_fm * 1000.0f);
            payload[3]  = (uint8_t)(fv >> 8);
            payload[4]  = (uint8_t)(fv & 0xFF);
            /* LNDVI × 1000 */
            int16_t lv = (int16_t)(g_state.lndvi * 1000.0f);
            payload[5]  = (uint8_t)(lv >> 8);
            payload[6]  = (uint8_t)(lv & 0xFF);
            /* ChlIndex × 100 */
            payload[7]  = (uint8_t)((uint16_t)(g_state.chl_index * 100.0f) >> 8);
            payload[8]  = (uint8_t)((uint16_t)(g_state.chl_index * 100.0f) & 0xFF);
            /* wetness × 100 */
            payload[9]  = (uint8_t)((uint16_t)(g_state.wetness * 100.0f) >> 8);
            payload[10] = (uint8_t)((uint16_t)(g_state.wetness * 100.0f) & 0xFF);
            /* CO₂ */
            payload[11] = (uint8_t)(g_state.co2_ppm >> 8);
            payload[12] = (uint8_t)(g_state.co2_ppm & 0xFF);
            /* T, RH */
            payload[13] = (uint8_t)g_state.temp_c;
            payload[14] = g_state.rh_pct;
            /* UV × 10 */
            payload[15] = g_state.uv_index_x10;
            /* acoustic events */
            payload[16] = (uint8_t)(g_state.acoustic_events & 0xFF);
            /* battery mV (high byte only) */
            payload[17] = (uint8_t)(g_state.battery_mv >> 8);
            /* flags */
            payload[18] = g_state.flags;

            lora_send(payload, sizeof(payload));
            g_state.seq++;

            /* Check for downlink. */
            uint8_t downlink[16];
            int dl_len = lora_receive(downlink, sizeof(downlink), 2000);
            if (dl_len > 0) {
                handle_downlink(downlink, (uint8_t)dl_len);
            }

            /* Update battery state-of-charge for next report. */
            power_update_soc();
        }
    }
}

/* ------------------------------------------------------------------------ */
/* Handle a LoRa downlink command                                              */
/* ------------------------------------------------------------------------ */
void handle_downlink(const uint8_t *buf, uint8_t len)
{
    if (len < 2) return;
    uint8_t port = buf[0];
    switch (port) {
    case 10:  /* set measurement interval */
        if (buf[1] >= MIN_MEASURE_INTERVAL_MIN &&
            buf[1] <= MAX_MEASURE_INTERVAL_MIN) {
            g_config.measure_interval_min = buf[1];
        }
        break;
    case 11:  /* set uplink interval */
        if (buf[1] > 0) g_config.uplink_interval_min = buf[1];
        break;
    case 12:  /* force immediate burst */
        {
            lichenwatch_state_t snap = g_state;
            do_measurement_burst(&snap);
            g_state = snap;
        }
        break;
    case 20:  /* retune classifier sensitivity */
        if (buf[1] <= 3) g_config.classifier_sensitivity = buf[1];
        break;
    default:
        break;
    }
}

/* ------------------------------------------------------------------------ */
/* BLE task — services walk-by bulk download requests                          */
/* ------------------------------------------------------------------------ */
static void task_ble(void *arg)
{
    (void)arg;
    ble_advertise_start(g_config.node_id);

    for (;;) {
        ble_event_t evt;
        if (ble_poll_event(&evt, portMAX_DELAY) == 0) {
            switch (evt.type) {
            case BLE_EVENT_CONNECT:
                ble_stop_advertising();
                break;
            case BLE_EVENT_DISCONNECT:
                ble_advertise_start(g_config.node_id);
                break;
            case BLE_EVENT_READ_STATUS: {
                /* Pack the current state vector. */
                ble_status_t bs;
                bs.fv_fm       = g_state.fv_fm;
                bs.lndvi       = g_state.lndvi;
                bs.chl_index   = g_state.chl_index;
                bs.wetness     = g_state.wetness;
                bs.class_state = g_state.classifier_state;
                bs.conf        = g_state.classifier_conf;
                bs.battery_mv  = g_state.battery_mv;
                bs.seq         = g_state.seq;
                bs.uptime_s    = g_state.uptime_s;
                ble_send_status(&bs);
                break;
            }
            case BLE_EVENT_BULK_REQ: {
                /* Stream SPI flash records over BLE. */
                uint32_t count = flash_record_count();
                for (uint32_t i = 0; i < count; i++) {
                    flash_record_t rec;
                    if (flash_read_record(i, &rec) == 0) {
                        if (ble_send_bulk_record(&rec) != 0) break;
                    }
                }
                ble_send_bulk_done();
                break;
            }
            case BLE_EVENT_CONFIG:
                if (evt.len >= 2) {
                    /* Same encoding as LoRa downlink on port 10. */
                    if (evt.data[0] >= MIN_MEASURE_INTERVAL_MIN &&
                        evt.data[0] <= MAX_MEASURE_INTERVAL_MIN) {
                        g_config.measure_interval_min = evt.data[0];
                    }
                    if (evt.len >= 2 && evt.data[1] > 0) {
                        g_config.uplink_interval_min = evt.data[1];
                    }
                }
                break;
            default:
                break;
            }
        }
    }
}

/* ------------------------------------------------------------------------ */
/* Hard fault / fault handlers                                                */
/* ------------------------------------------------------------------------ */
void HardFault_Handler(void)
{
    /* On a hard fault, blink the status LED rapidly and hang — visible field
     * indication that something went wrong. A watchdog will eventually reset. */
    volatile uint32_t *pc = (volatile uint32_t *)GPIOC;
    for (;;) {
        pc[5] = (1U << 13);
        for (volatile int i = 0; i < 200000; i++) { }
        pc[6] = (1U << 13);
        for (volatile int i = 0; i < 200000; i++) { }
    }
}

/* ------------------------------------------------------------------------ */
/* Author / build identification                                              */
/* ------------------------------------------------------------------------ */
const char fw_author_str[]   = "jayis1";
const char fw_version_str[]   = "1.0.0";
const char fw_build_date_str[] = "2026-07-08";