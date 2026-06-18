/*
 * afe.c — AeroCast analog front-end & particle-event extractor
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 *
 * Drives the LTC2351-14 4-channel simultaneous-sampling ADC over SPI1
 * at 2 MS/s/channel and extracts particle events from the continuous
 * stream using an adaptive threshold on the forward-scatter (FSC)
 * channel. Each qualifying peak triggers a 64-sample window snapshot
 * across all four channels (FSC, SSC, FL_blue, FL_amber); the per-event
 * feature vector (areas + peaks) is pushed into a ring buffer for the
 * classifier.
 *
 * The ADC is clocked continuously; a hardware analog comparator on the
 * FSC line (wired to a TIM input capture) timestamps candidate peaks
 * with ~150 ns resolution. Here we implement the software side of that
 * pipeline: a sliding baseline + hysteresis threshold on the DMA'd
 * samples, followed by area integration around each peak.
 */
#ifndef AEROCAST_AFE_C
#define AEROCAST_AFE_C
#endif

#include <stdint.h>
#include <string.h>
#include "board.h"
#include "registers.h"
#include "afe.h"

/* ---- DMA ring buffer (4 channels interleaved, 16-bit samples) ---- */
#define DMA_BUF_WORDS  2048   /* 2048 × 4 ch × 2 bytes = 16 KB */
static uint16_t g_dma_buf[DMA_BUF_WORDS * 4];
static volatile uint32_t g_dma_head;   /* written by DMA ISR (simulated here) */

/* ---- Event ring ---- */
static particle_event_t g_event_ring[EVENT_BUF_SIZE];
static volatile uint16_t g_ev_head, g_ev_tail;

/* ---- Adaptive baseline state ---- */
static struct {
    float baseline[4];
    float noise[4];       /* running RMS noise estimate */
    float threshold;      /* current FSC trigger threshold (ADC counts) */
    int32_t refractory;   /* samples remaining before next trigger allowed */
} g_afe;

/* ---- SPI ADC transaction: read 4 × 16-bit samples (LTC2351-14
 * returns all 4 channels simultaneously per CONVST). We pulse CONVST,
 * wait BUSY low, then clock out 8 bytes. */
static void adc_read_quad(uint16_t out[4])
{
    gpio_clr(PORTC, 9);            /* CONVST low */
    for (volatile int i = 0; i < 8; ++i) ;   /* ~50 ns */
    gpio_set(PORTC, 9);            /* CONVST high → start */
    while (gpio_read(PORTC, 10) == 0) ;     /* wait BUSY high (convert done) */

    gpio_clr(PORTC, 8);            /* CS low */
    spi_t *s = SPI(SPI1);
    /* Read 4 × 16-bit; LTC2351 clocks out channel N on each 16 SCLK frame. */
    for (int ch = 0; ch < 4; ++ch) {
        while (!(s->SR & (1u << 1u))) ;   /* TXP */
        s->TXDR = 0x0000u;
        while (!(s->SR & (1u << 2u))) ;   /* RXNE-ish (RXP) */
        uint32_t v = s->RXDR;
        out[ch] = (uint16_t)(v & 0xFFFFu);
    }
    gpio_set(PORTC, 8);            /* CS high */
}

/* ---- Public API ---- */
void afe_init(void)
{
    /* SPI1 master, 18 MHz → 4 MHz PSC divisor (280 MHz / 4 / 4 → ~17.5 MHz).
     * 16-bit words, CPOL=1 CPHA=1 to match LTC2351-14. */
    spi_init_master(SPI1_BASE, /* br_div */ 2u, /* word_sz */ 15u); /* 16-bit = 0b1111 */

    /* Prime baselines by averaging 64 quiescent reads. */
    float acc[4] = {0, 0, 0, 0};
    uint16_t s[4];
    for (int i = 0; i < 64; ++i) {
        adc_read_quad(s);
        for (int c = 0; c < 4; ++c) acc[c] += s[c];
    }
    for (int c = 0; c < 4; ++c) {
        g_afe.baseline[c] = acc[c] / 64.0f;
        g_afe.noise[c] = 8.0f;     /* initial guess */
    }
    g_afe.threshold = g_afe.baseline[0] + 64.0f;  /* ~8× initial noise */
    g_afe.refractory = 0;

    g_ev_head = g_ev_tail = 0;
    g_dma_head = 0;
}

/* Pull a fresh batch of samples from the ADC and run the event extractor.
 * Called every 10 ms from main(); processes as many quads as fit in the
 * 10 ms window (~20 000 samples at 2 MS/s → but we batch ~256 quads). */
#define QUADS_PER_CALL  256

void afe_pump(void)
{
    uint16_t s[4];
    static float win[4][64];
    static int   win_idx = 0;
    static int   armed = 0;
    static int   win_fill = 0;

    for (int q = 0; q < QUADS_PER_CALL; ++q) {
        adc_read_quad(s);
        if (g_afe.refractory > 0) g_afe.refractory--;

        /* Update baseline with slow IIR (τ ≈ 1 s at 2 MS/s) */
        for (int c = 0; c < 4; ++c) {
            float d = (float)s[c] - g_afe.baseline[c];
            g_afe.baseline[c] += d * 0.0005f;
            g_afe.noise[c] = 0.9999f * g_afe.noise[c] +
                             0.0001f * (d < 0.0f ? -d : d);
        }
        g_afe.threshold = g_afe.baseline[0] + 8.0f * g_afe.noise[0] + 32.0f;

        float fsc = (float)s[0] - g_afe.baseline[0];

        if (!armed && g_afe.refractory == 0 && fsc > g_afe.threshold) {
            armed = 1;
            win_idx = 0;
            win_fill = 0;
        }
        if (armed) {
            for (int c = 0; c < 4; ++c)
                win[c][win_idx] = (float)s[c] - g_afe.baseline[c];
            win_idx = (win_idx + 1) & 63;
            win_fill++;
            if (win_fill >= 64) {
                /* Integrate area + find peak per channel → event */
                particle_event_t ev;
                ev.timestamp_us = millis() * 1000u + (uint32_t)q;
                float areas[4] = {0,0,0,0};
                float peaks[4] = {0,0,0,0};
                for (int c = 0; c < 4; ++c) {
                    for (int k = 0; k < 64; ++k) {
                        float v = win[c][k];
                        areas[c] += v;
                        if (v > peaks[c]) peaks[c] = v;
                    }
                }
                ev.fsc_area   = (uint32_t)areas[0];
                ev.ssc_area   = (uint32_t)areas[1];
                ev.fl_blue    = (uint32_t)peaks[2];
                ev.fl_amber   = (uint32_t)peaks[3];
                ev.fsc_peak   = (uint32_t)peaks[0];
                ev.ssc_peak   = (uint32_t)peaks[1];

                /* Push to ring (overwrite oldest if full) */
                uint16_t next = (uint16_t)((g_ev_head + 1u) % EVENT_BUF_SIZE);
                if (next == g_ev_tail) g_ev_tail = (uint16_t)((g_ev_tail + 1u) % EVENT_BUF_SIZE);
                g_event_ring[g_ev_head] = ev;
                g_ev_head = next;

                armed = 0;
                g_afe.refractory = 32;   /* ~16 µs dead time */
            }
        }
    }
}

int afe_pop_event(particle_event_t *out)
{
    if (g_ev_head == g_ev_tail) return 0;
    *out = g_event_ring[g_ev_tail];
    g_ev_tail = (uint16_t)((g_ev_tail + 1u) % EVENT_BUF_SIZE);
    return 1;
}

/* Clean-air blank: run the pump for N seconds with no laser, re-zero
 * baselines and noise so subsequent triggers are genuine. */
void afe_run_blank(uint32_t seconds)
{
    /* Caller (main.c) has already switched the laser off. */
    for (uint32_t s = 0; s < seconds * 100u; ++s) {
        afe_pump();
        delay_ms(10);
    }
    /* Re-prime baseline from current quiet state. */
    for (int c = 0; c < 4; ++c) {
        g_afe.baseline[c] += 4.0f * g_afe.noise[c];
        g_afe.noise[c] = 6.0f;
    }
    g_afe.threshold = g_afe.baseline[0] + 64.0f;
}

/* Diagnostic accessors */
float afe_baseline(int ch) { return g_afe.baseline[ch & 3]; }
float afe_noise(int ch)    { return g_afe.noise[ch & 3]; }
float afe_threshold(void)  { return g_afe.threshold; }
uint32_t afe_events_pending(void) {
    return (uint32_t)((g_ev_head + EVENT_BUF_SIZE - g_ev_tail) % EVENT_BUF_SIZE);
}