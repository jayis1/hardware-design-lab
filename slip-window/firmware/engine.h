/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef SLIP_ENGINE_H
#define SLIP_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
#define UP 1u
#define DOWN 2u
#define LATCH 4u
#define MARK 8u
#define QUALIFY_MS 20u
#define ARM_MS 1000u
#define TURN_LIMIT_MS 30000u
#define CYCLE_LIMIT_MS 86400000u
/* These are observational states, never instructions to release a mold. */
typedef enum { CLEAR, READY, CAST, TURN, DRAIN, RETURN, REST, DONE, REJECT } State;
typedef enum {
    NONE, OPEN_EARLY, CONTRADICTION, SAMPLE_GAP, MARK_AGAIN,
    TURN_TIMEOUT, CYCLE_TIMEOUT, COUNTER_LIMIT, BAD_INPUT, NO_DRAIN
} Reason;
typedef struct {
    uint32_t now;
    uint32_t cycle_ms;
    uint32_t residence_ms;
    uint32_t drain_ms;
    uint32_t longest_drain_ms;
    uint32_t segment_ms;
    uint32_t rest_ms;
    uint32_t turn_ms;
    uint32_t arm_ms;
    uint32_t completed;
    uint32_t rejected;
    uint32_t interruptions;
    uint32_t gaps;
    uint32_t transport_losses;
    uint8_t raw;
    uint8_t stable;
    uint8_t candidate;
    uint8_t qualified_ms;
    bool qualified;
    State state;
    Reason reason;
} Engine;
void engine_init(Engine *e, uint32_t now);
void engine_sample(Engine *e, uint8_t raw, uint32_t now);
void engine_transport_loss(Engine *e);
const char *engine_state(State state);
const char *engine_reason(Reason reason);
bool engine_active(const Engine *e);
#endif
