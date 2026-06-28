/*
 * main.c — Cryo-Sentinel application-core main loop.
 *
 * Author: jayis1
 * License: MIT
 *
 * This file ties together the sensor scheduler, the alarm state machine,
 * the FRAM audit log, the power subsystem, and the radio dispatch. It is
 * structured so the 5-second sensor tick is the heartbeat of the whole
 * device: every other activity (logging, heartbeats, alarm sends) is
 * derived from tick counts.
 */
#include <string.h>
#include <stdbool.h>
#include "board.h"
#include "registers.h"
#include "cs_sensor.h"
#include "cs_alarm.h"
#include "cs_radio.h"
#include "cs_log.h"
#include "cs_power.h"
#include "cs_calibration.h"

/* ---- monotonic time -------------------------------------------------- *
 * We keep a minute-resolution epoch clock. On boot it starts at 0; once
 * the cellular link comes up the modem's NITZ (Network Identity and Time
 * Zone) gives us an absolute epoch, which we latch and offset all subsequent
 * timestamps from. This avoids needing an RTC crystal. */
static uint32_t g_epoch_min = 0;
static uint32_t g_tick_count = 0;
static bool     g_time_synced = false;

static uint32_t now_min(void) { return g_epoch_min + (g_tick_count * 5) / 60; }

/* Rolling level-rate window (last 15 minutes of calibrated level). */
#define LR_WINDOW 12   /* 12 ticks * 5 s = 60 s; we use 15-min heartbeat */
static float    g_level_hist[LR_WINDOW];
static int      g_level_hist_idx = 0;
static bool     g_level_hist_full = false;

static void level_hist_push(float pct)
{
    g_level_hist[g_level_hist_idx] = pct;
    g_level_hist_idx = (g_level_hist_idx + 1) % LR_WINDOW;
    if (g_level_hist_idx == 0) g_level_hist_full = true;
}

static float level_rate_per_hour(void)
{
    /* Slope of the oldest vs newest valid sample, scaled to per-hour. */
    int n = g_level_hist_full ? LR_WINDOW : g_level_hist_idx;
    if (n < 2) return 0.0f;
    int oldest = g_level_hist_full ? g_level_hist_idx : 0;
    float a = g_level_hist[oldest];
    float b = g_level_hist[(g_level_hist_idx - 1 + LR_WINDOW) % LR_WINDOW];
    if (a < 0 || b < 0) return 0.0f;   /* uncalibrated */
    float dt_h = (float)(n * 5) / 3600.0f;
    if (dt_h <= 0) return 0.0f;
    return (a - b) / dt_h;   /* +ve = dropping */
}

/* ---- main ------------------------------------------------------------ */
int main(void)
{
    /* 1. Bring up subsystems. */
    cs_power_init();
    int sensor_faults = cs_sensor_init();
    cs_alarm_t alarm;
    cs_alarm_init(&alarm);
    cs_log_init();
    cs_radio_init();

    /* Log the boot, including any sensor init faults. */
    cs_log_append(CS_LOG_BOOT, (int32_t)sensor_faults, now_min());
    if (sensor_faults) {
        cs_log_append(CS_LOG_FAULT, (int32_t)sensor_faults, now_min());
    }

    /* Load calibration if present. */
    cs_calibration_t cal;
    bool have_cal = (cs_calibration_load(&cal) == 0);

    /* 2. Main loop: 5 s tick. */
    cs_reading_t r;
    cs_power_t   pwr;

    for (;;) {
        int rc = cs_sensor_sample(&r);
        (void)rc;
        cs_power_read(&pwr);

        /* Feed the level-rate window. */
        if (r.level_ok && r.level_pct >= 0.0f)
            level_hist_push(r.level_pct);
        else
            level_hist_push(-1.0f);
        r.level_rate_ph = level_rate_per_hour();

        /* Evaluate the alarm state machine. */
        cs_reason_t reason = CS_REASON_NONE;
        cs_alarm_state_t prev = alarm.state;
        cs_alarm_state_t next = cs_alarm_eval(&alarm, &r, pwr.batt_pct,
                                              now_min(), &reason);

        /* Log transitions and GPIO edges. */
        if (next != prev) {
            cs_log_append(CS_LOG_STATE, (int32_t)((next << 16) | reason),
                          now_min());
        }
        if (r.lid_open && alarm.lid_open_since_min == now_min()) {
            cs_log_append(CS_LOG_LID_OPEN, 0, now_min());
        } else if (!r.lid_open && prev >= CS_STATE_WARN &&
                   alarm.lid_open_since_min == 0) {
            cs_log_append(CS_LOG_LID_CLOSE, 0, now_min());
        }
        if (r.enclosure_open && alarm.enclosure_open_since_min == now_min()) {
            cs_log_append(CS_LOG_ENC_OPEN, 0, now_min());
        } else if (!r.enclosure_open && alarm.enclosure_open_since_min == 0) {
            cs_log_append(CS_LOG_ENC_CLOSE, 0, now_min());
        }
        if (!r.mains_present && alarm.mains_lost_since_min == now_min()) {
            cs_log_append(CS_LOG_MAINS_LOST, 0, now_min());
        } else if (r.mains_present && alarm.mains_lost_since_min == 0) {
            cs_log_append(CS_LOG_MAINS_BACK, 0, now_min());
        }

        /* Drive the local LEDs and buzzer. */
        /* (In the real build these are GPIO writes; stubbed here.) */
        extern void mock_gpio_set(uint8_t, uint32_t);
        mock_gpio_set(CS_GPIO_LED_WARN,
                      (next >= CS_STATE_WATCH) ? 1 : 0);
        mock_gpio_set(CS_GPIO_LED_CRIT,
                      (next >= CS_STATE_CRITICAL) ? 1 : 0);
        mock_gpio_set(CS_GPIO_BUZZER,
                      (next == CS_STATE_CRITICAL &&
                       (g_tick_count % 12) == 0) ? 1 : 0);

        /* Check for an incoming ack from the cloud/BLE bridge. */
        uint8_t in_op = 0; uint8_t in_payload[72]; size_t in_len = 0;
        if (cs_radio_poll_incoming(&in_op, in_payload, &in_len)) {
            if (in_op == CS_IPC_OP_ALARM_ACK && in_len >= 8) {
                uint32_t ack_seq = 0, ack_tech = 0;
                memcpy(&ack_seq, in_payload + 0, 4);
                memcpy(&ack_tech, in_payload + 4, 4);
                cs_alarm_ack(&alarm, ack_tech);
                cs_log_append(CS_LOG_ALARM_ACK, (int32_t)ack_tech, now_min());
            }
        }

        /* Alarm send logic: on transition into WARN/CRIT, and repeat if
         * unacked and the repeat interval has elapsed. */
        if (next >= CS_STATE_WARN) {
            bool should_send = (next != prev) || cs_alarm_repeat_due(&alarm, now_min());
            if (should_send) {
                cs_alarm_msg_t msg;
                memset(&msg, 0, sizeof(msg));
                msg.seq       = cs_log_seq();
                msg.epoch_min = now_min();
                msg.tier      = (next == CS_STATE_CRITICAL) ? 2 : 1;
                msg.state     = next;
                msg.reason    = reason != CS_REASON_NONE ? reason : alarm.reason;
                msg.level_pct = r.level_pct;
                switch (msg.reason) {
                case CS_REASON_TILT_WARN:
                case CS_REASON_TILT_CRIT:    msg.aux = r.tilt_deg;       break;
                case CS_REASON_LEVEL_RATE:   msg.aux = r.level_rate_ph;  break;
                case CS_REASON_AMBIENT_HOT:  msg.aux = r.ambient_degC;   break;
                case CS_REASON_BATT_CRIT:    msg.aux = pwr.batt_pct;     break;
                default:                     msg.aux = 0.0f;             break;
                }
                if (have_cal)
                    memcpy(msg.dewar_serial, cal.dewar_label,
                           sizeof(msg.dewar_serial));
                if (cs_radio_send_alarm(&msg) == 0) {
                    alarm.last_alarm_sent_min = now_min();
                    cs_log_append(CS_LOG_ALARM_SEND,
                                  (int32_t)((msg.tier << 16) | msg.reason),
                                  now_min());
                }
            }
        }

        /* Heartbeat every 15 minutes (180 ticks). */
        if ((g_tick_count % (CS_HEARTBEAT_MIN * 60 / 5)) == 0) {
            cs_heartbeat_t hb;
            memset(&hb, 0, sizeof(hb));
            hb.seq            = cs_log_seq();
            hb.epoch_min      = now_min();
            hb.state          = alarm.state;
            hb.reason         = alarm.reason;
            hb.level_pct      = r.level_pct;
            hb.level_rate_ph  = r.level_rate_ph;
            hb.rtd_top_degC   = r.rtd_degC[CS_RTD_TOP];
            hb.rtd_mid_degC   = r.rtd_degC[CS_RTD_MID];
            hb.rtd_bot_degC   = r.rtd_degC[CS_RTD_BOT];
            hb.gradient_slope = r.gradient_slope;
            hb.tilt_deg       = r.tilt_deg;
            hb.vib_rms_g      = r.vib_rms_g;
            hb.ambient_degC   = r.ambient_degC;
            hb.ambient_rh     = r.ambient_rh;
            hb.batt_pct       = pwr.batt_pct;
            hb.mains_present  = r.mains_present;
            hb.lid_open       = r.lid_open;
            hb.enclosure_open = r.enclosure_open;
            if (cs_radio_send_heartbeat(&hb) == 0) {
                cs_log_append(CS_LOG_HEARTBEAT, 0, now_min());
                /* After a successful heartbeat, try to sync time from the
                 * modem's NITZ. In the real build this is an AT+CCLK? parse. */
                if (!g_time_synced) {
                    g_epoch_min = 0;  /* placeholder until NITZ parsed */
                    /* g_time_synced = true; once parsed */
                }
            }
        }

        g_tick_count++;
        /* In the real build: k_sleep(K_MSEC(CS_TICK_MS)). On desktop we just
         * loop; the mock_delay_ms calls inside sensor sampling provide the
         * wall-clock pacing for testing. */
        extern void mock_delay_ms(uint32_t);
        mock_delay_ms(CS_TICK_MS);
    }
    /* not reached */
    return 0;
}

/* ---- weak mock symbols for desktop linking --------------------------- */
__attribute__((weak)) int  mock_i2c_read(uint8_t a, uint8_t r, uint8_t *b, int n)
{ (void)a;(void)r;(void)b;(void)n; return 0; }
__attribute__((weak)) int  mock_i2c_write(uint8_t a, uint8_t r, const uint8_t *b, int n)
{ (void)a;(void)r;(void)b;(void)n; return 0; }
__attribute__((weak)) int  mock_spi_xfer(uint8_t c, const uint8_t *t, uint8_t *r, int n)
{ (void)c;(void)t;(void)r;(void)n; return 0; }
__attribute__((weak)) uint32_t mock_gpio_get(uint8_t p) { (void)p; return 0; }
__attribute__((weak)) void mock_gpio_set(uint8_t p, uint32_t v) { (void)p;(void)v; }
__attribute__((weak)) void mock_delay_ms(uint32_t ms) { (void)ms; }