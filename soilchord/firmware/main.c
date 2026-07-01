/*
 * main.c — Soilchord firmware entry point and scheduler
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 *
 * The firmware is bare-metal C with no RTOS. main() calls board_init() then
 * scheduler_run(), which never returns. The scheduler is driven by the RTC
 * alarm waking the MCU out of STOP2 each measurement interval.
 */
#include <string.h>
#include <math.h>
#include "soilchord.h"
#include "proto.h"
#include "registers.h"

volatile uint32_t g_seq = 0;
measurement_t     g_last;
uint8_t           g_interval_s = DEFAULT_INTERVAL_S;
uint8_t           g_urgent_remaining = 0;

/* Rolling baseline (EMA) for f0 and Q per chord for anomaly detection. */
static float s_ema_f0[NCHORDS];
static float s_ema_q[NCHORDS];
static float s_var_f0[NCHORDS];
static float s_var_q[NCHORDS];
static uint8_t s_anomaly_count[NCHORDS];

/* Static sample buffers (placed in SRAM; we avoid dynamic allocation). */
static int16_t s_samples[NSAMPLES_PER_CHORD];
static uint8_t s_txbuf[64];

/* -------------------------------------------------------------------------- */
static void features_to_proto(const chord_features_t *f, chord_payload_t *p)
{
    /* Scale f0 to Q8.8-ish representation in 16 bits. */
    float f0 = f->f0;
    if (f0 < 0.0f) f0 = 0.0f;
    if (f0 > 6502.0f) f0 = 6502.0f;     /* 65535/256 - small guard */
    p->f0_q8 = (uint16_t)(f0 * 256.0f + 0.5f);

    float qf = f->q_freq * 4.0f;
    if (qf > 255.0f) qf = 255.0f;
    if (qf < 0.0f)   qf = 0.0f;
    p->q_freq_u8 = (uint8_t)(qf + 0.5f);

    float qt = f->q_time * 4.0f;
    if (qt > 255.0f) qt = 255.0f;
    if (qt < 0.0f)   qt = 0.0f;
    p->q_time_u8 = (uint8_t)(qt + 0.5f);

    /* Temperature: offset by +64 so -64..+191 °C fits in a byte. */
    float t = f->temp_c + 64.0f;
    if (t > 255.0f) t = 255.0f;
    if (t < 0.0f)   t = 0.0f;
    p->temp_c_u8 = (uint8_t)(t + 0.5f);

    p->flags = f->flags & 0x07;
}

/* -------------------------------------------------------------------------- */
static uint8_t build_uplink(const measurement_t *m, uint8_t *out, uint8_t max_len)
{
    ul_header_t *h = (ul_header_t *)out;
    h->magic       = UL_HDR_MAGIC;
    h->ver          = UL_VER;
    h->seq          = m->seq;
    h->unix_time    = m->unix_time;
    h->battery_mv   = m->battery_mv;
    h->solar_mv     = m->solar_mv;
    h->interval_s   = m->interval_s;
    h->reset_cause  = m->reset_cause;
    h->urgent       = m->urgent;
    h->nchords      = NCHORDS;

    uint8_t off = sizeof(ul_header_t);
    for (int i = 0; i < NCHORDS; i++) {
        if (off + sizeof(chord_payload_t) > max_len) break;
        chord_payload_t *cp = (chord_payload_t *)(out + off);
        features_to_proto(&m->chord[i], cp);
        off += sizeof(chord_payload_t);
    }
    return off;
}

/* -------------------------------------------------------------------------- */
static void update_anomaly_state(const chord_features_t *f, uint8_t chord)
{
    /* Welford-style incremental mean/variance over an EMA. */
    float a = ANOMALY_ALPHA;
    float ef = s_ema_f0[chord];
    float eq = s_ema_q[chord];

    s_ema_f0[chord] = (1.0f - a) * ef + a * f->f0;
    s_ema_q[chord]  = (1.0f - a) * eq + a * f->q_freq;

    float df = f->f0 - s_ema_f0[chord];
    float dq = f->q_freq - s_ema_q[chord];
    s_var_f0[chord] = (1.0f - a) * s_var_f0[chord] + a * df * df;
    s_var_q[chord]  = (1.0f - a) * s_var_q[chord]  + a * dq * dq;

    float sf = sqrtf(s_var_f0[chord]) + 1e-6f;
    float sq = sqrtf(s_var_q[chord])  + 1e-6f;
    float zf = fabsf(df) / sf;
    float zq = fabsf(dq) / sq;

    if (zf > ANOMALY_Z_THRESH || zq > ANOMALY_Z_THRESH) {
        if (s_anomaly_count[chord] < 255) s_anomaly_count[chord]++;
    } else {
        if (s_anomaly_count[chord] > 0) s_anomaly_count[chord]--;
    }
}

static bool chord_is_anomalous(uint8_t chord)
{
    return s_anomaly_count[chord] >= ANOMALY_PERSIST;
}

/* -------------------------------------------------------------------------- */
static sc_err_t do_measurement_cycle(measurement_t *m)
{
    memset(m, 0, sizeof(*m));
    m->seq          = ++g_seq;
    m->interval_s   = g_interval_s;
    m->reset_cause  = power_reset_cause();

    /* Read power state first — if the battery is critically low, abort. */
    sc_err_t err = power_read(&m->battery_mv, &m->solar_mv);
    if (err != SC_OK) return err;
    if (m->battery_mv < VBATT_MIN_MV) {
        /* Too low to measure safely; skip this cycle. */
        return SC_ERR_RANGE;
    }

    /* Power-gate the analog chain on. */
    piezo_chain_on();
    /* Allow the rails to settle (~1 ms is plenty). */
    for (volatile int i = 0; i < 8000; i++) __asm__("nop");

    bool any_anomaly = false;

    for (uint8_t c = 0; c < NCHORDS; c++) {
        chord_features_t *cf = &m->chord[c];
        cf->chord = c;

        /* Read temperature first (one-shot; we use the long conversion). */
        temp_read(c, &cf->temp_c);

        /* Pluck the chord. */
        err = actuator_pluck(c);
        if (err != SC_OK) {
            cf->flags |= UL_FLAG_SUSPECT;
            continue;
        }

        /* Brief settle, then capture. */
        for (volatile int i = 0; i < 1600 * PLUCK_SETTLE_MS; i++) __asm__("nop");
        err = piezo_capture(c, s_samples, NSAMPLES_PER_CHORD);
        if (err != SC_OK) {
            cf->flags |= UL_FLAG_SUSPECT;
            continue;
        }

        /* Extract features from the captured waveform. */
        dsp_extract_features(s_samples, NSAMPLES_PER_CHORD, c, cf);

        /* Temperature-compensate f0 for the steel chord's thermal expansion.
         * α_steel ≈ 17 ppm/K; f ∝ sqrt(Tension), and for a fixed-strain wire
         * Δf/f ≈ -0.5·α·ΔT (tension drops as the wire expands). */
        float dt = cf->temp_c - 20.0f;          /* reference 20 °C */
        cf->f0 /= (1.0f - 0.5f * 17e-6f * dt);

        /* Dead-pluck detection: very low RMS + low harmonic ratio. */
        if (cf->rms < 5.0f && cf->h2_h1 < 0.05f) {
            cf->flags |= UL_FLAG_DEAD_PLUCK;
            /* Retry once. */
            actuator_pluck(c);
            for (volatile int i = 0; i < 1600 * PLUCK_SETTLE_MS; i++) __asm__("nop");
            piezo_capture(c, s_samples, NSAMPLES_PER_CHORD);
            dsp_extract_features(s_samples, NSAMPLES_PER_CHORD, c, cf);
            cf->f0 /= (1.0f - 0.5f * 17e-6f * dt);
            if (cf->rms < 5.0f) {
                cf->flags |= UL_FLAG_SUSPECT;
            } else {
                cf->flags &= ~UL_FLAG_DEAD_PLUCK;
            }
        }

        /* Cross-check the two Q estimates. */
        if (cf->q_freq > 1.0f && cf->q_time > 1.0f) {
            float ratio = cf->q_freq / cf->q_time;
            if (ratio > 1.25f || ratio < 0.8f) cf->flags |= UL_FLAG_SUSPECT;
        }

        update_anomaly_state(cf, c);
        if (chord_is_anomalous(c)) {
            cf->flags |= UL_FLAG_ALERT;
            any_anomaly = true;
        }
    }

    piezo_chain_off();

    /* Adaptive scheduling: if anything is flagged alert, drop into fast mode. */
    if (any_anomaly) {
        if (g_urgent_remaining == 0) g_urgent_remaining = URGENT_BURST_CYCLES;
        m->urgent = 1;
        g_interval_s = URGENT_INTERVAL_S;
    } else if (g_urgent_remaining > 0) {
        g_urgent_remaining--;
        m->urgent = 1;
        g_interval_s = URGENT_INTERVAL_S;
        if (g_urgent_remaining == 0) g_interval_s = DEFAULT_INTERVAL_S;
    } else {
        m->urgent = 0;
        g_interval_s = DEFAULT_INTERVAL_S;
    }
    m->interval_s = g_interval_s;

    return SC_OK;
}

/* -------------------------------------------------------------------------- */
static sc_err_t do_telemetry(measurement_t *m)
{
    if (m->battery_mv < VBATT_TX_MIN_MV) {
        /* Battery too low to transmit; just log locally. */
        flash_log_append(m);
        return SC_ERR_RANGE;
    }

    uint8_t n = build_uplink(m, s_txbuf, sizeof(s_txbuf));
    sc_err_t err = lora_send(s_txbuf, n, LORA_RETRIES);
    flash_log_append(m);    /* always log, regardless of TX result */
    return err;
}

/* -------------------------------------------------------------------------- */
static void handle_downlink(void)
{
    uint8_t rxb[16];
    uint8_t rxn = 0;
    sc_err_t err = lora_recv(rxb, &rxn, 200);
    if (err != SC_OK || rxn < sizeof(dl_header_t)) return;

    dl_frame_t dl;
    err = proto_unpack_downlink(rxb, rxn, &dl);
    if (err != SC_OK) return;

    switch (dl.cmd) {
    case DL_SET_INTERVAL:
        if (dl.arg >= 60 && dl.arg <= 3600) {
            g_interval_s = (uint8_t)dl.arg;     /* clamp to 8-bit field for now */
            if (g_urgent_remaining == 0) g_interval_s = (uint8_t)dl.arg;
        }
        break;
    case DL_FORCE_MEASURE:
        /* Force an immediate cycle. */
        g_urgent_remaining = 1;
        g_interval_s = 2;
        break;
    case DL_CALIBRATE_BASELINE:
        /* Reset baseline statistics — next cycles will reseed EMA. */
        for (int i = 0; i < NCHORDS; i++) {
            s_ema_f0[i] = kChordFreeAirHz[i];
            s_ema_q[i]  = 80.0f;
            s_var_f0[i] = 1.0f;
            s_var_q[i]  = 4.0f;
            s_anomaly_count[i] = 0;
        }
        break;
    case DL_REQUEST_BACKFILL:
        /* Stream up to N missed records from flash. The simple strategy: send
         * the most recent flash_log_count() - arg records one per TX slot. */
        {
            uint32_t have = flash_log_count();
            if (dl.arg < have) {
                uint32_t to_send = have - dl.arg;
                if (to_send > 8) to_send = 8;
                for (uint32_t k = 0; k < to_send; k++) {
                    measurement_t old;
                    if (flash_log_read(have - to_send + k, &old) == SC_OK) {
                        uint8_t n = build_uplink(&old, s_txbuf, sizeof(s_txbuf));
                        lora_send(s_txbuf, n, 1);
                    }
                }
            }
        }
        break;
    default:
        break;
    }
}

/* -------------------------------------------------------------------------- */
int main(void)
{
    board_init();
    dsp_init();
    adc_init();
    actuator_init();
    lora_init();
    temp_init();
    power_init();
    flash_log_init();

    /* Seed the anomaly baselines with free-air expectations so the first few
     * cycles don't fire false positives before the EMA settles. */
    for (int i = 0; i < NCHORDS; i++) {
        s_ema_f0[i] = kChordFreeAirHz[i];
        s_ema_q[i]  = 80.0f;
        s_var_f0[i] = 1.0f;
        s_var_q[i]  = 4.0f;
        s_anomaly_count[i] = 0;
    }

    scheduler_run();
    return 0;     /* unreachable */
}

/* -------------------------------------------------------------------------- */
void scheduler_run(void)
{
    /* Initialise the RTC alarm to the first interval. */
    power_enter_stop2(g_interval_s);

    for (;;) {
        /* ---- Wake ---- */
        int watchdog_kick = 0;

        sc_err_t err = do_measurement_cycle(&g_last);
        if (err == SC_OK) {
            do_telemetry(&g_last);
            handle_downlink();
            watchdog_kick = 1;
        } else if (err == SC_ERR_RANGE) {
            /* Battery too low: skip telemetry, just log and sleep longer. */
            flash_log_append(&g_last);
            g_interval_s = DEFAULT_INTERVAL_S * 4;   /* back off */
            watchdog_kick = 1;
        } else {
            /* Unexpected error: log and continue. */
            watchdog_kick = 1;
        }

        if (watchdog_kick) {
            /* Refresh the independent watchdog. In a real build this would be
             * IWDG->KR = 0xAAAA; we expose it via a guarded macro. */
            extern void iwdg_refresh(void);
            iwdg_refresh();
        }

        /* ---- Sleep until the next interval ---- */
        power_enter_stop2(g_interval_s);
    }
}