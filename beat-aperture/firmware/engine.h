/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef BEAT_ENGINE_H
#define BEAT_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
#define QUALIFY 5u
#define MAX_CYCLE_MS 30000u
#define MIN_CYCLE_MS 10u
/* All faults terminate correlation until physical reset. */
enum { WAIT, RUN, HALT };
enum { F_GAP=1, F_OVERFLOW=2, F_CONFIG=4, F_WATCHDOG=8,
       F_OVERLAP=16, F_TIMEOUT=32 };
typedef struct {
    uint32_t phase; /* 0 unanchored, 1 left visited, 2 subsequent right visited */
    uint32_t age;
    uint32_t cycles;
} Oscillator;
typedef struct {
    uint32_t now,state,levels,valid,faults,gaps,tx_errors;
    Oscillator side[2];
    uint32_t windows,zero,one,many,assigned,completed_hits;
    uint32_t pending,first,last;
    uint32_t period,last_hits,last_first,last_last;
    uint32_t stamp;
    uint8_t candidate[4],stable[4];
    bool initialized;
} Engine;
void engine_init(Engine *e);
void engine_sample(Engine *e,uint32_t now,uint8_t raw);
void engine_fault(Engine *e,uint32_t fault);
void engine_tx_error(Engine *e);
bool engine_valid(const Engine *e);
#endif
