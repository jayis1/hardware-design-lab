/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef DETENT_ENGINE_H
#define DETENT_ENGINE_H
#include <stdint.h>
#include <stdbool.h>
#define QUALIFY_MS 5u
#define DEADLINE_MS 10000u
#define UNKNOWN_STATION 4u
/* Faults halt accounting; ordinary uncertain journeys merely lose anchor. */
enum { F_CONFIG=1, F_WATCHDOG=2, F_EXHAUST=4, F_INPUT=8 };
typedef struct {
    uint32_t now, sequence;
    uint32_t starts, cw, ccw, returns, jumps, aborted;
    uint32_t reversals, cw_laps, ccw_laps;
    uint32_t overlaps, shortcuts, timeouts, gaps, tx_errors;
    uint32_t age, last_ms;
    uint8_t raw, candidate, qualified, stable_age;
    uint8_t station, origin, last_from, last_to;
    uint8_t direction, run, fault;
    bool have_sample, ready, active, last_valid;
} Engine;
void engine_init(Engine *e);
void engine_sample(Engine *e, uint32_t now, uint8_t raw);
void engine_fault(Engine *e, uint8_t fault);
void engine_tx_error(Engine *e);
bool engine_valid(const Engine *e);
#endif
