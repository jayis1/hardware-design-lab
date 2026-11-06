/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef PALLET_ENGINE_H
#define PALLET_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
#define LIMIT 1000000u
#define DEADLINE 60000u
#define QUALIFY 5u
#define BASELINE 50u
enum { WAIT, READY, ACTIVE, HALT };
enum { F_CONFIG=1, F_WATCHDOG=2, F_GAP=4, F_DEADLINE=8,
       F_BOUNDARY=16, F_COUNTER=32 };
typedef struct {
    uint32_t sequence, now, state, fault, raw, qualified, valid;
    uint32_t starts, completed, aborted, age, baseline;
    uint32_t pulses, a, b, neither, both, uncertain;
    uint32_t aa, ab, ba, bb, anchor, tx_errors, gaps;
    uint8_t candidate[4], stable[4];
    bool seen;
} Engine;
void engine_init(Engine *e);
void engine_sample(Engine *e, uint32_t now, uint8_t raw);
void engine_fault(Engine *e, uint32_t fault);
void engine_tx_error(Engine *e);
bool engine_valid(const Engine *e);
#endif
