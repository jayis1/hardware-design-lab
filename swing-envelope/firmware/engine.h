/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef SWING_ENGINE_H
#define SWING_ENGINE_H
#include <stdint.h>
#include <stdbool.h>
#define QUALIFY_MS 3u
#define MAX_PAIR_MS 10000u
#define MAX_CYCLE_MS 60000u
/* Bits: inner left, inner right, outer left, outer right. */
typedef struct {
    uint32_t now;
    uint32_t previous;
    uint32_t stable_age;
    uint32_t pending_age;
    uint32_t cycle_age;
    uint32_t idle_age;
    uint32_t crossings;
    uint32_t left_seen;
    uint32_t right_seen;
    uint32_t left_missed;
    uint32_t right_missed;
    uint32_t faults;
    uint32_t gaps;
    uint32_t timeouts;
    uint32_t transport_losses;
    uint32_t period_ms;
    uint32_t east_ms;
    uint32_t west_ms;
    uint8_t candidate;
    uint8_t levels;
    uint8_t pending;
    uint8_t direction;
    uint8_t outer_seen;
    bool ready;
    bool cycle_anchor;
    bool period_valid;
    bool blocked;
    bool qualified;
} Engine;
void engine_init(Engine *e, uint32_t now);
void engine_sample(Engine *e, uint8_t raw, uint32_t now);
void engine_transport_loss(Engine *e);
uint32_t engine_flags(const Engine *e);
#endif
