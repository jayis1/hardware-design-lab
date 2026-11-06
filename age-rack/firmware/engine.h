/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef AGE_ENGINE_H
#define AGE_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
#define LIMIT 1000000u
#define QUALIFY 5u
enum { WAIT, RUNNING, HALT };
enum { F_CONFIG=1, F_WATCHDOG=2, F_GAP=4, F_COUNTER=8 };
typedef struct {
    uint32_t sequence, now, state, fault, raw, qualified, valid;
    uint32_t epoch, occupied, insertions, removals, abandoned, ambiguous, last_mask;
    uint32_t rank[4], stamp[4], low[4], high[4];
    uint32_t tx_errors, gaps;
    uint8_t candidate[4], stable[4];
    bool seen;
} Engine;
void engine_init(Engine *e);
void engine_sample(Engine *e, uint32_t now, uint8_t raw);
void engine_fault(Engine *e, uint32_t fault);
void engine_tx_error(Engine *e);
bool engine_valid(const Engine *e);
#endif
