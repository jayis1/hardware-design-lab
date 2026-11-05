/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef VACANCY_ENGINE_H
#define VACANCY_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
#define QUALIFY_MS 5u
#define BASELINE_MS 50u
#define DEADLINE_MS 10000u
#define LIMIT 1000000u
#define ABSENT 4u
/* Bits indicate occupied pockets in clockwise order 0,1,2,3. */
enum { WAIT, READY, CARRY, HALT };
enum { NONE, MOVED, CANCELED, NONADJACENT, AMBIGUOUS, TIMEOUT, LOST, ABORT };
enum { F_CONFIG=1, F_WATCHDOG=2, F_EXHAUST=4, F_INPUT=8, F_GAP=16 };
typedef struct {
    uint32_t sequence, now, starts, moves, canceled, abandoned;
    uint32_t clockwise, counterclockwise, age, last_ms, baseline;
    uint32_t tx_errors, gaps;
    uint8_t state, result, fault, raw, qualified, valid;
    uint8_t vacancy, source, base, candidate, stable;
    bool have_sample, previous_clean;
} Engine;
void engine_init(Engine *e);
void engine_sample(Engine *e, uint32_t now, uint8_t raw);
void engine_fault(Engine *e, uint8_t fault);
void engine_tx_error(Engine *e);
bool engine_valid(const Engine *e);
#endif
