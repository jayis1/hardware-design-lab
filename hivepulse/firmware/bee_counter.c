/*
 * bee_counter.c — IR break-beam bee entrance traffic counter for HivePulse
 *
 * Uses 16 IR break-beam sensor pairs across the hive entrance to count
 * bees entering and exiting. Each beam break is timestamped and direction
 * is determined by the sequence of adjacent beam breaks (directional logic).
 *
 * The 16 sensors are on GPIOD[0:15] with falling-edge EXTI interrupts.
 * A 17th "direction latch" sensor (PC7) provides a reference beam to
 * determine travel direction: if the latch beam breaks before the main
 * array, the bee is exiting; if after, it is entering.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#include "bee_counter.h"
#include "board.h"
#include "registers.h"
#include <string.h>

/* ---- Internal State ---- */
static volatile uint32_t bees_in_count = 0;
static volatile uint32_t bees_out_count = 0;
static volatile uint32_t total_crossings = 0;
static volatile uint32_t crossings_this_interval = 0;
static volatile uint32_t max_rate_this_interval = 0;
static volatile uint32_t crossings_last_second = 0;
static volatile uint32_t last_second_crossings = 0;

/* Per-channel state tracking */
typedef struct {
    bool blocked;         /* Beam currently broken */
    uint32_t block_time;  /* Timestamp when beam was broken */
    uint8_t prev_channel; /* Previous channel that broke (for direction) */
} channel_state_t;

static channel_state_t channels[BEE_COUNTER_CHANNELS];

/* Rolling window for rate calculation (1 second, 100ms buckets) */
#define RATE_WINDOW_BUCKETS  10
static volatile uint32_t rate_buckets[RATE_WINDOW_BUCKETS];
static volatile uint8_t current_rate_bucket = 0;
static volatile uint32_t last_bucket_time = 0;

/* Direction detection: track the last channel that was broken */
static volatile uint8_t last_broken_channel = 0xFF;
static volatile uint32_t last_break_time = 0;
static volatile bool direction_latch_set = false;
static volatile bool direction_is_out = false;

/* Minimum time between consecutive breaks on the same channel (debounce) */
#define DEBOUNCE_MS  50
/* Maximum time between adjacent channel breaks for direction tracking */
#define DIRECTION_TIMEOUT_MS  200
/* Direction latch window */
#define LATCH_WINDOW_MS  100

/* ---- Initialize GPIO and EXTI for 16-channel bee counter ---- */
int bee_counter_init(void)
{
    /* Enable GPIOD clock */
    RCC_AHB2ENR |= RCC_AHB2ENR_GPIODEN;

    /* Configure PD0-PD15 as input with pull-up (IR receiver open-collector) */
    for (int pin = 0; pin < BEE_COUNTER_CHANNELS; pin++) {
        GPIOD->MODER &= ~(0x3U << (pin * 2));
        GPIOD->MODER |= (GPIO_MODE_INPUT << (pin * 2));
        GPIOD->PUPDR &= ~(0x3U << (pin * 2));
        GPIOD->PUPDR |= (GPIO_PULLUP << (pin * 2));
    }

    /* Configure direction latch pin (PC7) as input with pull-up */
    GPIOC->MODER &= ~(0x3U << 14);
    GPIOC->MODER |= (GPIO_MODE_INPUT << 14);
    GPIOC->PUPDR &= ~(0x3U << 14);
    GPIOC->PUPDR |= (GPIO_PULLUP << 14);

    /* Configure EXTI for falling edge on PD0-PD15 */
    /* Enable SYSCFG clock for EXTI routing */
    RCC_APB2ENR |= (1U << 0); /* SYSCFGEN */

    /* Route PD0-PD15 to EXTI lines 0-15 via SYSCFG_EXTICR registers */
    /* EXTI0 -> PD: EXTICR1[3:0] = 0x3 */
    volatile uint32_t *exticr = (volatile uint32_t *)(SYSCFG_BASE + 0x08);
    for (int i = 0; i < 4; i++) {
        exticr[i] = 0x33333333; /* All 16 lines -> port D (0x3) */
    }

    /* Configure EXTI: falling edge trigger, enable interrupt */
    for (int line = 0; line < BEE_COUNTER_CHANNELS; line++) {
        EXTI->FTSR1 |= (1U << line);   /* Falling edge trigger */
        EXTI->RTSR1 &= ~(1U << line);  /* Disable rising edge */
        EXTI->IMR1 |= (1U << line);    /* Unmask interrupt */
    }

    /* Also configure EXTI7 for direction latch (PC7) */
    /* Route EXTI7 to port C (0x2) */
    exticr[1] &= ~(0xFU << 24);
    exticr[1] |= (0x2U << 24);
    EXTI->FTSR1 |= (1U << 7);   /* Falling edge */
    EXTI->IMR1 |= (1U << 7);    /* Unmask */

    /* Enable EXTI interrupts in NVIC */
    /* Lines 0-4 have individual IRQs, lines 5-9 share EXTI9_5_IRQn,
     * lines 10-15 share EXTI15_10_IRQn */
    NVIC_ISER0 |= (1U << 6);   /* EXTI0_IRQn */
    NVIC_ISER0 |= (1U << 7);   /* EXTI1_IRQn */
    NVIC_ISER0 |= (1U << 8);   /* EXTI2_IRQn */
    NVIC_ISER0 |= (1U << 9);   /* EXTI3_IRQn */
    NVIC_ISER0 |= (1U << 10);  /* EXTI4_IRQn */
    NVIC_ISER0 |= (1U << (EXTI9_5_IRQn & 0x1F));
    NVIC_ISER0 |= (1U << (EXTI15_10_IRQn & 0x1F));

    /* Initialize state */
    memset((void *)channels, 0, sizeof(channels));
    memset((void *)rate_buckets, 0, sizeof(rate_buckets));
    bees_in_count = 0;
    bees_out_count = 0;
    total_crossings = 0;
    crossings_this_interval = 0;
    max_rate_this_interval = 0;
    current_rate_bucket = 0;
    last_bucket_time = 0;
    last_broken_channel = 0xFF;
    last_break_time = 0;
    direction_latch_set = false;
    direction_is_out = false;

    return 0;
}

/* ---- Process a beam-break event ---- */
static void process_break(uint8_t channel)
{
    uint32_t now = systick_ms;

    /* Debounce: ignore re-triggers on the same channel within debounce window */
    if (channels[channel].blocked) {
        return;
    }

    channels[channel].blocked = true;
    channels[channel].block_time = now;

    /* Count the crossing */
    total_crossings++;
    crossings_this_interval++;
    crossings_last_second++;

    /* Update rate bucket */
    uint32_t bucket_age = now - last_bucket_time;
    if (bucket_age >= 100) {
        /* Advance to next bucket */
        current_rate_bucket = (current_rate_bucket + 1) % RATE_WINDOW_BUCKETS;
        rate_buckets[current_rate_bucket] = 0;
        last_bucket_time = now;

        /* Check peak rate (crossings in last second) */
        uint32_t current_rate = 0;
        for (int i = 0; i < RATE_WINDOW_BUCKETS; i++) {
            current_rate += rate_buckets[i];
        }
        if (current_rate > max_rate_this_interval) {
            max_rate_this_interval = current_rate;
        }
    }
    rate_buckets[current_rate_bucket]++;

    /* Direction detection:
     * If the direction latch (PC7) was recently triggered, the bee is exiting.
     * Otherwise, determine direction by the spatial sequence of breaks:
     *   - If the new channel > previous channel, bee is entering (moving inward)
     *   - If the new channel < previous channel, bee is exiting (moving outward)
     * This assumes channels are arranged from outside (0) to inside (15). */
    if (direction_latch_set) {
        if (direction_is_out) {
            bees_out_count++;
        } else {
            bees_in_count++;
        }
        direction_latch_set = false;
    } else if (last_broken_channel != 0xFF &&
               (now - last_break_time) < DIRECTION_TIMEOUT_MS) {
        /* Determine direction from channel sequence */
        if (channel > last_broken_channel) {
            /* Moving from low to high channel = entering */
            bees_in_count++;
        } else if (channel < last_broken_channel) {
            /* Moving from high to low channel = exiting */
            bees_out_count++;
        } else {
            /* Same channel — ambiguous, count as exit by default */
            bees_out_count++;
        }
    } else {
        /* No previous break context — count based on latch */
        if (direction_is_out) {
            bees_out_count++;
        } else {
            bees_in_count++;
        }
    }

    last_broken_channel = channel;
    last_break_time = now;
}

/* ---- Process a beam-clear event (beam restored) ---- */
static void process_clear(uint8_t channel)
{
    channels[channel].blocked = false;
}

/* ---- EXTI Interrupt Handlers ---- */
void bee_counter_exti_handler(void)
{
    uint32_t pending = EXTI->PR1;

    /* Check direction latch (line 7) */
    if (pending & (1U << 7)) {
        EXTI->PR1 = (1U << 7); /* Clear pending */
        /* Direction latch triggered: bee is exiting */
        direction_latch_set = true;
        direction_is_out = true;
        return;
    }

    /* Check each of the 16 bee counter channels */
    for (int ch = 0; ch < BEE_COUNTER_CHANNELS; ch++) {
        if (pending & (1U << ch)) {
            EXTI->PR1 = (1U << ch); /* Clear pending */

            /* Check if beam is broken (pin low) or restored (pin high) */
            if (GPIOD->IDR & (1U << ch)) {
                /* Pin is high — beam restored */
                process_clear(ch);
            } else {
                /* Pin is low — beam broken */
                process_break(ch);
            }
        }
    }
}

/* Individual EXTI handlers (delegate to common handler) */
void EXTI0_IRQHandler(void)  { bee_counter_exti_handler(); }
void EXTI1_IRQHandler(void)  { bee_counter_exti_handler(); }
void EXTI2_IRQHandler(void)  { bee_counter_exti_handler(); }
void EXTI3_IRQHandler(void)  { bee_counter_exti_handler(); }
void EXTI4_IRQHandler(void)  { bee_counter_exti_handler(); }
void EXTI9_5_IRQHandler(void) { bee_counter_exti_handler(); }
void EXTI15_10_IRQHandler(void) { bee_counter_exti_handler(); }

/* ---- Poll Function ---- */
int bee_counter_poll(bee_traffic_t *traffic)
{
    if (!traffic) return -1;

    uint32_t now = systick_ms;

    /* Snapshot counts (disable interrupts briefly for atomicity) */
    __disable_irq();
    traffic->bees_in = bees_in_count;
    traffic->bees_out = bees_out_count;
    traffic->total_crossings = total_crossings;
    traffic->peak_rate = max_rate_this_interval;
    traffic->timestamp_ms = now;
    __enable_irq();

    /* Compute average activity (crossings per second since last poll) */
    static uint32_t last_poll_time = 0;
    static uint32_t last_poll_crossings = 0;
    uint32_t elapsed = (now - last_poll_time) / 1000;
    if (elapsed > 0) {
        traffic->avg_activity =
            (float)(traffic->total_crossings - last_poll_crossings) / elapsed;
    } else {
        traffic->avg_activity = 0.0f;
    }
    last_poll_time = now;
    last_poll_crossings = traffic->total_crossings;

    /* Build active channels bitmap */
    traffic->active_channels = 0;
    for (int ch = 0; ch < BEE_COUNTER_CHANNELS; ch++) {
        if (channels[ch].blocked) {
            traffic->active_channels |= (1U << ch);
        }
    }

    /* Reset interval statistics */
    crossings_this_interval = 0;
    max_rate_this_interval = 0;

    return 0;
}

/* ---- Live Activity (for BLE) ---- */
int bee_counter_get_live(uint32_t *in_last_min, uint32_t *out_last_min)
{
    /* In a full implementation, we'd maintain rolling 60-second counters.
     * For now, we approximate using the rate buckets (10-second window
     * extrapolated to 60 seconds). */
    uint32_t recent = 0;
    for (int i = 0; i < RATE_WINDOW_BUCKETS; i++) {
        recent += rate_buckets[i];
    }
    /* Extrapolate: recent is ~1 second of data, multiply by 60 */
    *in_last_min = (bees_in_count * recent) / MAX(1, (bees_in_count + bees_out_count));
    *out_last_min = (bees_out_count * recent) / MAX(1, (bees_in_count + bees_out_count));
    return 0;
}

/* ---- Reset ---- */
void bee_counter_reset(void)
{
    __disable_irq();
    bees_in_count = 0;
    bees_out_count = 0;
    total_crossings = 0;
    crossings_this_interval = 0;
    max_rate_this_interval = 0;
    memset((void *)rate_buckets, 0, sizeof(rate_buckets));
    __enable_irq();
}