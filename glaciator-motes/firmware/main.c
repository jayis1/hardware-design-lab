/*
 * main.c — Glaciator-Motes application core (Cortex-M4)
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 *
 * The M4 core handles the seismic acquisition chain, event detection,
 * SD logging, GPS discipline, and the power FSM. Sealed event packets are
 * handed to the M0+ radio core via a shared-SRAM mailbox for LoRa TX.
 *
 * Build: make (uses arm-none-eabi-gcc)
 * Target: STM32WL55JC
 */

#include "board.h"
#include "registers.h"
#include "adc_driver.h"
#include "mems_driver.h"
#include "seismology.h"
#include "lora_mesh.h"
#include "gps_discipline.h"
#include "power.h"
#include "sdlog.h"
#include "lz4.h"
#include <string.h>

/* ---- RTC 1 Hz flag (set by ISR) ---------------------------------------- */
static volatile bool g_rtc_flag = false;
static volatile uint32_t g_systick_ms = 0;

/* ---- Outgoing event queue (mailbox to M0+ radio core) ------------------ */
#define EVENT_QUEUE_LEN 4
static uint8_t  g_event_queue[EVENT_QUEUE_LEN][8192];
static uint32_t g_event_queue_len[EVENT_QUEUE_LEN];
static uint8_t  g_event_head = 0;
static uint8_t  g_event_tail = 0;

/* ---- Compressed event buffer ------------------------------------------- */
static uint8_t  g_compressed[8192];

/* ---- Initialization helpers -------------------------------------------- */
static void board_init_clocks(void)
{
    /* In real build: switch SYSCLK to 48 MHz PLL sourced from HSE (TCXO 26 MHz),
       enable SUBGHZSPI clock for the M0+ core, enable all needed peripheral
       clocks (SPI1, SPI2, I2C1, USART1, EXTI, RTC, DMA). */
}

static void board_init_gpio(void)
{
    /* Configure all pins per board.h: SPI CS, EXTI inputs, LED, heater,
       GPS_EN, TCXO_EN as outputs; ADS1256/MEMS DRDY as falling-edge EXTI. */
}

static void board_init_rtc(void)
{
    /* LSE (32.768 kHz TCXO) → RTC calendar + 1 Hz wakeup interrupt. */
}

static void ipc_notify_m0plus(void)
{
    /* Trigger C2_IT1 interrupt to wake the M0+ core and tell it there's
       a new event packet in the shared mailbox. */
}

/* ---- Rx callback (called from mesh M0+ via IPC) ------------------------ */
static void on_mesh_rx(const mesh_hdr_t *hdr, const uint8_t *payload, uint16_t len)
{
    (void)hdr; (void)payload; (void)len;
    /* In a relay/gateway node, we might log the received event or re-broadcast.
       For a leaf node this is mostly beacons & ACKs which are handled in the
       mesh layer. */
}

/* ---- GPS duty cycle (5 min / hour) ------------------------------------- */
static void gps_duty_cycle(uint32_t uptime_s)
{
    uint32_t phase = uptime_s % 3600;
    if (phase < 300) {
        if (!gps_is_fixed()) gps_power_on();
    } else {
        gps_power_off();
    }
}

/* ---- Main -------------------------------------------------------------- */
int main(void)
{
    board_init_clocks();
    board_init_gpio();
    board_init_rtc();

    /* Bring up peripherals */
    power_init();
    adc_init();
    mems_init();
    seismo_init();
    sdlog_init();
    gps_init();

    /* Initialize LoRa mesh as a node (gateway role set via provisioning) */
    mesh_init(MESH_ROLE_NODE, MESH_MY_NODE_ID, 0);
    mesh_on_rx(on_mesh_rx);

    /* Enable interrupts */
    /* NVIC_EnableIRQ(ADS1256_DRDY_IRQn); */
    /* NVIC_EnableIRQ(ICM42688_DRDY_IRQn); */
    /* NVIC_EnableIRQ(GPS_PPS_IRQn); */
    /* NVIC_EnableIRQ(RTC_IRQn); */

    /* Start continuous seismic acquisition */
    adc_start_continuous();

    uint32_t last_health_s = 0;
    uint8_t  event_buf[8192];
    uint32_t event_len = 0;

    /* ---- Super-loop --------------------------------------------------- */
    while (1) {
        /* 1 Hz RTC tick */
        if (g_rtc_flag) {
            g_rtc_flag = false;
            uint32_t uptime = mesh_uptime_s();
            power_tick_1hz();
            gps_tick_1hz();
            gps_duty_cycle(uptime);
            mesh_tick();

            /* Send health packet once per minute */
            if (uptime - last_health_s >= 60) {
                last_health_s = uptime;
                mesh_send_health(power_vbat_mv(), power_temp_c(),
                                  power_solar_pct(), uptime);
            }

            /* Adjust acquisition based on power state */
            power_state_t ps = power_state();
            if (ps == PWR_STATE_SURVIVAL || ps == PWR_STATE_CHARGE_ONLY) {
                adc_stop();
                /* In survival: just beacon + listen for gateway poll */
            } else if (ps == PWR_STATE_DEGRADED) {
                adc_set_drate(ADS1256_DRATE_60SPS);
                adc_start_continuous();
            } else {
                adc_set_drate(ADS1256_DRATE_1000SPS);
                adc_start_continuous();
            }
        }

        /* Seismic frame processing */
        adc_frame_t *f = adc_take_frame();
        if (f) {
            bool triggered = seismo_process_frame(f);

            /* Always write to continuous SD ring (for post-season analysis) */
            if (power_state() == PWR_STATE_NORMAL) {
                uint8_t flat[ADC_FRAME_LEN * ADC_CHANNELS * 2];
                uint16_t i;
                uint32_t off = 0;
                for (i = 0; i < f->len; i++) {
                    int16_t v;
                    v = (int16_t)(f->samples[i].volts[0] * 10000.0f);
                    memcpy(&flat[off], &v, 2); off += 2;
                    v = (int16_t)(f->samples[i].volts[1] * 10000.0f);
                    memcpy(&flat[off], &v, 2); off += 2;
                    v = (int16_t)(f->samples[i].volts[2] * 10000.0f);
                    memcpy(&flat[off], &v, 2); off += 2;
                }
                sdlog_write_continuous(flat, off);
            }
            adc_release_frame(f);

            /* If triggered, seal event + compress + queue for TX */
            if (triggered) {
                seismo_seal_event(event_buf, &event_len, sizeof(event_buf));

                /* One-bit classification */
                int8_t sign_z[ADC_FRAME_LEN], sign_ns[ADC_FRAME_LEN], sign_ew[ADC_FRAME_LEN];
                seismo_frame_to_signbits(f, sign_z, sign_ns, sign_ew);
                uint8_t tpl_id = seismo_classify(sign_z, ADC_FRAME_LEN);

                /* Log to SD as an event record */
                event_meta_t *meta = seismo_last_event();
                if (meta) {
                    meta->type        = (event_type_t)tpl_id;
                    meta->template_id = tpl_id;
                    sdlog_write_event(meta, event_buf, event_len);
                }

                /* LZ4-compress for LoRa transmission */
                int32_t clen = lz4_compress(event_buf, g_compressed,
                                             event_len, sizeof(g_compressed));
                if (clen > 0) {
                    /* Enqueue for M0+ radio core */
                    memcpy(g_event_queue[g_event_head], g_compressed, (uint32_t)clen);
                    g_event_queue_len[g_event_head] = (uint32_t)clen;
                    g_event_head = (g_event_head + 1) % EVENT_QUEUE_LEN;
                    ipc_notify_m0plus();
                }
            }
        }

        /* MEMS high-rate frame processing (crevasse-snap detection) */
        mems_frame_t *mf = mems_take_frame();
        if (mf) {
            /* Simple threshold detector for high-frequency snaps */
            for (uint16_t i = 0; i < mf->len; i++) {
                int16_t mag = mf->s[i].x < 0 ? -mf->s[i].x : mf->s[i].x;
                if (mag > 16000) {   /* ~8 g threshold */
                    /* High-frequency crevasse snap detected.
                       Build a small event packet from the MEMS window. */
                    break;
                }
            }
            mems_release_frame(mf);
        }

        /* Drain event queue → mesh_send_event */
        if (g_event_head != g_event_tail) {
            mesh_send_event(g_event_queue[g_event_tail],
                             g_event_queue_len[g_event_tail]);
            g_event_tail = (g_event_tail + 1) % EVENT_QUEUE_LEN;
        }

        /* Idle: enter WFI (Wait For Interrupt) to save power */
        /* __WFI(); */
    }
}

/* ---- ISR stubs --------------------------------------------------------- */
void RTC_IRQHandler(void)
{
    g_rtc_flag = true;
    /* Clear RTC wakeup flag in real build */
}

void SysTick_Handler(void)
{
    g_systick_ms++;
}

/* ---- stubs for libc ---------------------------------------------------- */
/* (real build links newlib-nano) */