/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef BRAID_ENGINE_H
#define BRAID_ENGINE_H
#include <stdint.h>
#include <stdbool.h>
enum { WAIT, READY, ACTIVE, HALT };
enum { F_CONFIG=1, F_WATCHDOG=2, F_OVERFLOW=4, F_GAP=8,
       F_TIMEOUT=16, F_CAPACITY=32 };
#define QUALIFY 5u
#define MAX_PRESS_MS 10000u
#define WORD_LIMIT 32u
typedef struct {
    uint32_t now, state, levels, valid, starts, accepted, rejected, aborted;
    uint32_t gaps, faults, tx_errors, age, tainted, selected, outcome;
    uint32_t depth, cancelled, permutation, positive, negative;
    uint32_t packed[4];
    uint8_t reduced[WORD_LIMIT];
    uint8_t candidate[3], stable[3], paddle_seen;
    uint32_t stamp;
    bool initialized;
} Engine;
void engine_init(Engine *e);
void engine_sample(Engine *e, uint32_t now, uint8_t raw);
void engine_fault(Engine *e, uint32_t fault);
void engine_tx_error(Engine *e);
bool engine_valid(const Engine *e);
#endif
