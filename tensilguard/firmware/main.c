/*
 * main.c — TensilGuard firmware entry point and scheduler
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 *
 * The firmware is bare-metal C with no RTOS. main() calls board_init() then
 * scheduler_run(), which never returns. The scheduler is driven by the RTC
 * alarm waking the MCU out of STOP2 each measurement interval, or by the
 * COMP1 wake on an AE (wire-break) event.
 *
 * Each cycle:
 *   1. run the magnetoelastic sweep (T_mag)
 *   2. read the accelerometer window + FFT (T_vib)
 *   3. compute disagreement ΔT and AE count
 *   4. log to flash
 *   5. send LoRa uplink (urgent if ΔT threshold or AE events)
 *   6. return to STOP2
 */
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include "tensilguard.h"
#include "board.h"
#include "registers.h"
#include "proto.h"
#include "hal.h"

/* Global state — referenced from other modules via tensilguard.h */
volatile uint32_t g_seq = 0;
measurement_t     g_last;
cal_page_t        g_cal;
uint8_t           g_interval_s = TG_DEFAULT_INTERVAL_S / 10;
uint8_t           g_urgent_remaining = 0;

/* Static sample buffers and telemetry buffer (no dynamic allocation) */
static uint8_t s_txbuf[TG_UPLINK_MAX_BYTES];
static uint8_t s_node_id[6] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 };
static uint32_t s_reset_cause = 0;

/* Rolling baselines (EMA) for anomaly detection on ΔT */
static float s_ema_dt = 0.0f;
static float s_var_dt = 1.0f;
static float s_dt_threshold = 5.0f;    /* kN, auto-learned */
static uint8_t s_warmup_cycles = 0;
static const uint8_t WARMUP_N = 48;   /* first day at 30-min cycles */

/* ----------------------------- Prototypes --------------------------------- */
static void board_init(void);
static void scheduler_run(void);
static void do_measurement_cycle(void);
static void build_uplink(const measurement_t *m, uint32_t seq, uint8_t *out, uint8_t *len);
static void features_to_proto(const measurement_t *m, ul_packet_t *p);
static void update_anomaly(const measurement_t *m);

/* stubs provided by HAL modules (declared extern so we can link) */
extern void lora_on_receive(const uint8_t *pkt, uint8_t len, int8_t snr);

/* ----------------------------- Entry -------------------------------------- */

int main(void)
{
    board_init();
    /* read calibration from flash; if missing, cal coefficients are zero
     * and the magnetoelastic channel returns raw mu until calibrated. */
    if (!flash_read_cal(&g_cal)) {
        memset(&g_cal, 0, sizeof(g_cal));
        g_cal.cable_len_m   = CABLE_LEN_M;
        g_cal.cable_lin_mass = CABLE_LIN_MASS;
        g_cal.cable_diam_mm = CABLE_DIAM_MM;
        g_cal.sag_m         = CABLE_SAG_M;
        g_cal.n_wires       = CABLE_NWIRES;
    }
    scheduler_run();
    return 0;   /* never reached */
}

/* ----------------------------- Board init --------------------------------- */

static void board_init(void)
{
    /* 1. clocks: HSI → PLL → 170 MHz system clock */
    /* (real firmware configures RCC_PLLCFGR, sets latency, switches RCC_CFGR) */
    /* 2. enable GPIO + peripheral clocks */
    RCC_AHB2ENR |= BIT(0) | BIT(1) | BIT(2);   /* GPIOA/B/C */
    RCC_APB1ENR1 |= BIT(0) | BIT(14) | BIT(21) | BIT(22) | BIT(23); /* TIM2/6, I2C1, SPI2/3 */
    RCC_APB2ENR  |= BIT(11) | BIT(12) | BIT(14) | BIT(16) | BIT(0); /* SYSCFG, COMP, TIM1, ADC */

    /* 3. peripherals init */
    (void)temp_init();
    (void)accel_init();
    (void)power_init();
    ae_init();
    lora_init(s_node_id);
    (void)flash_init();

    /* 4. RTC + first alarm */
    rtc_set_alarm(g_interval_s * 10);
    s_reset_cause = read_reset_cause();
}

/* ----------------------------- Scheduler ---------------------------------- */

static void scheduler_run(void)
{
    uint32_t last_wake_unix = 0;
    while (1) {
        /* enter STOP2; woken by RTC alarm or COMP1 (AE) */
        power_enter_stop2();

        /* why did we wake? check RTC alarm flag and AE comparator flag */
        uint32_t now = rtc_get_unix();
        bool rtc_alarm = (RTC_ISR & BIT(8)) != 0;   /* ALRAF */
        bool ae_wake = comp1_triggered();

        if (ae_wake) {
            ae_isr();                 /* capture, classify, log, maybe urgent */
            continue;
        }
        if (rtc_alarm) {
            RTC_ISR &= ~BIT(8);        /* clear ALRAF */
            if (now != last_wake_unix) {
                do_measurement_cycle();
                last_wake_unix = now;
            }
            /* adapt interval based on battery state and sunlight */
            uint32_t adapted = power_adapt_interval(g_interval_s * 10);
            rtc_set_alarm(adapted);
        }
    }
}

/* ----------------------------- Measurement cycle -------------------------- */

static void do_measurement_cycle(void)
{
    measurement_t m;
    memset(&m, 0, sizeof(m));
    uint32_t seq = ++g_seq;        /* atomic-ish in this single-thread model */

    /* temperature first — needed by both modalities */
    m.temp_c = temp_read_c();

    /* 1. magnetoelastic sweep */
    float mu_eff_raw, t_mag;
    if (mag_sweep(&mu_eff_raw, &t_mag)) {
        m.mu_eff = temp_compensate_mu_eff(mu_eff_raw, m.temp_c, &g_cal);
        m.t_mag_kn = t_mag;
    } else {
        m.flags |= TG_FLAG_CAL_LOST;
        m.mu_eff = 0.0f;
        m.t_mag_kn = 0.0f;
    }

    /* 2. accelerometer-based tension */
    float f1, t_vib;
    if (accel_measure_tension(&f1, &t_vib)) {
        m.f1_hz = temp_compensate_f1(f1, m.temp_c, g_cal.cable_len_m);
        m.t_vib_kn = t_vib;
    } else {
        m.f1_hz = 0.0f;
        m.t_vib_kn = 0.0f;
    }

    /* 3. disagreement */
    m.dt_kn = fabsf(m.t_mag_kn - m.t_vib_kn);

    /* 4. battery / panel */
    power_poll();
    m.battery_pct = power_battery_pct();
    m.battery_mv   = power_battery_mv();
    m.panel_mv     = power_panel_mv();
    m.accel_rms_mg = accel_rms_mg();

    /* 5. AE count since last uplink */
    m.ae_count = ae_count_drain();

    /* 6. anomaly / alarm logic */
    update_anomaly(&m);
    if (m.flags & (TG_FLAG_DT_ALARM | TG_FLAG_AE_ALARM)) {
        m.flags |= TG_FLAG_URGENT;
    }
    if (m.battery_pct < TG_BATTERY_LOW_PCT) {
        m.flags |= TG_FLAG_LOW_BATT;
    }

    /* 7. persist + uplink */
    g_last = m;
    flash_log_measurement(&m, seq, rtc_get_unix());

    uint8_t len;
    build_uplink(&m, seq, s_txbuf, &len);
    bool relay = false;   /* root node never relays its own packet */
    (void)lora_send(s_txbuf, len, relay, 0);

    /* re-arm urgent window */
    if (g_urgent_remaining > 0) {
        g_urgent_remaining--;
        m.flags |= TG_FLAG_URGENT;
    }

    /* return to RX listen so downlinks/mesh relays are handled */
    lora_listen();
}

/* ----------------------------- Uplink builder ----------------------------- */

static void build_uplink(const measurement_t *m, uint32_t seq, uint8_t *out, uint8_t *len)
{
    ul_header_t *h = (ul_header_t *)out;
    h->magic       = UL_HDR_MAGIC;
    h->ver          = UL_VER;
    memcpy(h->node_id, s_node_id, 6);
    h->hops         = 0;
    h->rssi_db      = 0;
    h->seq          = seq;
    h->unix_time    = rtc_get_unix();

    /* payload: ul_packet_t */
    ul_packet_t *p = (ul_packet_t *)(out + sizeof(ul_header_t));
    features_to_proto(m, p);

    /* AE summary record if any new events */
    uint8_t off = sizeof(ul_header_t) + sizeof(ul_packet_t);
    if (m->ae_count > 0) {
        const ae_event_t *ev = ae_last_event();
        ae_record_t *ar = (ae_record_t *)(out + off);
        ar->unix_time       = ev->unix_time;
        ar->peak_uv_q1     = (uint16_t)(ev->peak_uv * 10.0f);
        ar->rise_us         = (uint16_t)ev->rise_us;
        ar->dur_us          = (uint16_t)ev->dur_us;
        ar->centroid_khz   = (uint16_t)ev->centroid_khz;
        ar->hi_lo_ratio_q4 = (uint8_t)(ev->hi_lo_ratio * 16.0f);
        ar->score           = ev->score;
        ar->classification = ev->classification;
        memcpy(ar->waveform, ev->waveform, sizeof(ar->waveform));
        off += sizeof(ae_record_t);
    }

    /* CRC over everything after the header's crc16 field */
    uint16_t crc = crc16_ccitt(out + offsetof(ul_header_t, seq),
                                off - offsetof(ul_header_t, seq));
    h->crc16 = crc;

    *len = off;
}

static void features_to_proto(const measurement_t *m, ul_packet_t *p)
{
    /* scale floats into packed Q representations */
    p->t_mag_q4 = (uint16_t)(clampf(m->t_mag_kn, 0.0f, 4000.0f) * 16.0f);
    p->t_vib_q4 = (uint16_t)(clampf(m->t_vib_kn, 0.0f, 4000.0f) * 16.0f);
    p->dt_q4    = (uint16_t)(clampf(m->dt_kn,    0.0f, 4000.0f) * 16.0f);
    p->f1_mHz   = (uint16_t)(clampf(m->f1_hz,    0.0f, 65.0f)  * 1000.0f);
    p->temp_c10 = (int16_t)(m->temp_c * 10.0f);
    p->mu_eff_q12 = (uint16_t)(clampf(m->mu_eff, 0.0f, 16.0f) * 4096.0f);
    p->battery_pct = (uint8_t)m->battery_pct;
    p->soc_q4      = (uint16_t)(clampf(m->battery_pct, 0.0f, 100.0f) * 16.0f);
    p->battery_mv  = (uint16_t)m->battery_mv;
    p->panel_mv    = (uint16_t)m->panel_mv;
    p->accel_rms_mg = m->accel_rms_mg;
    p->interval_s   = g_interval_s;
    p->reset_cause  = (uint8_t)(s_reset_cause & 0xFF);
    p->urgent       = (m->flags & TG_FLAG_URGENT) ? 1 : 0;
    p->ae_count     = m->ae_count;
    p->flags        = m->flags;
}

/* ----------------------------- Anomaly learning --------------------------- */

static void update_anomaly(const measurement_t *m)
{
    /* EMA of ΔT and its variance; alarm when current ΔT > mean + k*sigma */
    float dt = m->dt_kn;
    if (s_warmup_cycles < WARMUP_N) {
        /* warm-up: accumulate EMA, don't alarm */
        if (s_warmup_cycles == 0) {
            s_ema_dt = dt;
            s_var_dt = 1.0f;
        } else {
            float e = dt - s_ema_dt;
            s_ema_dt += 0.1f * e;
            s_var_dt += 0.1f * (e * e - s_var_dt);
        }
        s_warmup_cycles++;
        if (s_warmup_cycles == WARMUP_N) {
            s_dt_threshold = s_ema_dt + 4.0f * sqrtf(s_var_dt);
            if (s_dt_threshold < 1.0f) s_dt_threshold = 1.0f;
        }
        return;
    }
    /* steady state: update EMA slowly, alarm on excursions */
    float e = dt - s_ema_dt;
    s_ema_dt += 0.02f * e;
    s_var_dt += 0.02f * (e * e - s_var_dt);
    s_dt_threshold = s_ema_dt + 4.0f * sqrtf(s_var_dt);
    if (s_dt_threshold < 1.0f) s_dt_threshold = 1.0f;
    if (dt > s_dt_threshold) {
        ((measurement_t *)m)->flags |= TG_FLAG_DT_ALARM;
    }
}

/* ----------------------------- Downlink handler --------------------------- */

void lora_on_receive(const uint8_t *pkt, uint8_t len, int8_t snr)
{
    (void)snr;
    if (len < sizeof(dl_command_t)) return;
    const dl_command_t *cmd = (const dl_command_t *)pkt;
    switch (cmd->cmd) {
    case DL_CMD_SET_INTERVAL: {
        uint32_t new_s = ((uint32_t)cmd->arg[0] << 16) |
                         ((uint32_t)cmd->arg[1] << 8)  |
                          (uint32_t)cmd->arg[2];
        if (new_s >= TG_MIN_INTERVAL_S && new_s <= TG_MAX_INTERVAL_S) {
            g_interval_s = (uint8_t)(new_s / 10);
            rtc_set_alarm(new_s);
        }
        break;
    }
    case DL_CMD_REQUEST_SWEEP:
        do_measurement_cycle();
        break;
    case DL_CMD_CAL_WRITE: {
        cal_page_t *new_cal = (cal_page_t *)cmd->arg;
        (void)new_cal;   /* actual arg is longer than 7 bytes; impl extends */
        flash_write_cal(&g_cal);
        break;
    }
    case DL_CMD_CAL_READ: {
        /* reply with current cal page */
        static uint8_t buf[sizeof(ul_header_t) + sizeof(cal_page_t)];
        ul_header_t *h = (ul_header_t *)buf;
        h->magic = UL_HDR_MAGIC; h->ver = UL_VER;
        memcpy(h->node_id, s_node_id, 6);
        h->seq = 0; h->unix_time = rtc_get_unix();
        memcpy(buf + sizeof(ul_header_t), &g_cal, sizeof(cal_page_t));
        lora_send(buf, sizeof(buf), false, 0);
        break;
    }
    case DL_CMD_REBOOT:
        /* NVIC_SystemReset(); */
        break;
    case DL_CMD_AE_DUMP: {
        /* flush recent AE events from flash to uplink */
        uint8_t buf[64];
        int n = flash_read_log_range(0, 4, buf, sizeof(buf));
        if (n > 0) lora_send(buf, (uint8_t)n, false, 0);
        break;
    }
    case DL_CMD_SET_AE_THRESH: {
        float thr = (float)((uint16_t)((cmd->arg[0] << 8) | cmd->arg[1]));
        ae_set_threshold(thr);
        break;
    }
    default:
        break;
    }
}

/* ----------------------------- Helpers / stubs ---------------------------- */
/* clampf is provided as a static inline in hal.h. */

/* HAL stubs — real firmware implements these against the register layer */
void rtc_set_alarm(uint32_t s) { (void)s; }
uint32_t read_reset_cause(void) { return 0; }
bool comp1_triggered(void) { return false; }

/* declarations to satisfy callers in other modules */
extern void delay_ms(uint32_t);
extern void gpio_set(pin_t, int);