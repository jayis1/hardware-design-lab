/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef FOLD_ENGINE_H
#define FOLD_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
#define STATIONS 4u
#define DEBOUNCE_MS 20u
#define MIN_HOLD_MS 500u
#define CLEAR_MS 1000u
#define CYCLE_MS 30000u
#define MAX_SAMPLE_MS 5u
/* CLEAR also protects startup: all four switches must first open. */
typedef enum { CLEAR, READY, HOLD, RELEASE, REJECT } State;
typedef enum { NONE, ORDER, OVERLAP, SHORT_HOLD, TIMEOUT, SAMPLE_GAP } Reason;
typedef struct {
    bool candidate;
    bool stable;
    uint32_t since;
} Contact;
typedef struct {
    Contact contact[STATIONS];
    State state;
    Reason reason;
    uint8_t step;
    uint8_t mask;
    uint8_t raw;
    uint32_t previous;
    uint32_t cycle_start;
    uint32_t hold_start;
    uint32_t clear_start;
    uint32_t good;
    uint32_t bad;
    uint32_t gaps;
    uint32_t duration;
    uint32_t dwell[STATIONS];
    bool clearing;
} Engine;
void engine_init(Engine *e, uint32_t now);
void engine_sample(Engine *e, uint32_t now, uint8_t closed_mask);
const char *engine_state(State state);
const char *engine_reason(Reason reason);
#endif
