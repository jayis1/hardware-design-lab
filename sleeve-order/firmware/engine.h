/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef SLEEVE_ENGINE_H
#define SLEEVE_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
#define QUALIFY 5u
#define LIMIT 60000u
#define F_CONFIG 1u
#define F_WATCHDOG 2u
#define F_OVERFLOW 4u
/* Endpoint mask weights: outer home=1, outer far=2, inner home=4, inner far=8. */
enum Phase { WAIT, READY, OUTWARD, EXTENDED, INNER_OUT, INNER_FAR,
             INNER_BACK, OUTER_BACK, HALT };
enum Result { RESULT_NONE, RESULT_OUTER, RESULT_NESTED, RESULT_ORDER,
              RESULT_INVALID, RESULT_GAP, RESULT_TIMEOUT, RESULT_FAULT };
typedef struct {
    uint32_t sequence, now, phase, fault, raw, qualified, valid;
    uint32_t starts, completed, nested, aborted, inner_trips, violations;
    uint32_t timeouts, gaps, tx_errors, age, maximum, last, last_age;
    uint32_t candidate, stable, cycle_trips;
    bool initialized;
} Engine;
void engine_init(Engine *e);
void engine_sample(Engine *e, uint32_t now, uint8_t raw);
void engine_fault(Engine *e, uint32_t flag);
void engine_tx_error(Engine *e);
bool engine_valid(const Engine *e);
bool engine_active(const Engine *e);
#endif
