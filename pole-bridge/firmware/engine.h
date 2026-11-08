/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef POLE_ENGINE_H
#define POLE_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
#define LIMIT 1000000u
#define QUALIFY 5u
#define SETTLE 20u
#define DEADLINE 5000u
enum { WAIT, READY, ACTIVE, HALT };
enum { NONE, TRANSFER, RETURN, TIMEOUT };
enum { F_CONFIG=1, F_WATCHDOG=2, F_GAP=4, F_COUNTER=8 };
typedef struct {
    uint32_t origin, outcome, age;
    uint32_t bins[8], unknown[2], mixed;
} Capture;
typedef struct {
    uint32_t sequence, now, state, fault, raw, qualified, valid;
    uint32_t starts, completed, returned, timeouts, aborted, tx_errors, gaps;
    uint32_t anchor, target, settle;
    Capture work, last;
    uint8_t candidate[4], stable[4];
    bool seen;
} Engine;
void engine_init(Engine *e);
void engine_sample(Engine *e, uint32_t now, uint8_t raw);
void engine_fault(Engine *e, uint32_t fault);
void engine_tx_error(Engine *e);
bool engine_valid(const Engine *e);
#endif
