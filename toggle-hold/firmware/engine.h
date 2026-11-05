/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef TOGGLE_ENGINE_H
#define TOGGLE_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
#define QUALIFY_MS 5u
#define BASELINE_MS 50u
#define HOLD_MS 500u
#define DEADLINE_MS 10000u
#define ABSENT UINT32_MAX
/* Endpoint bits: A=1, B=2. Pressure witnesses: toward B=4, toward A=8. */
enum { WAIT, READY, PUSH, LOADED, HOLD, DONE, HALT };
enum { NONE, RETAINED, EARLY, REBOUND, CONFLICT, TIMEOUT, ABORT };
enum { F_CONFIG=1, F_WATCHDOG=2, F_EXHAUST=4, F_INPUT=8, F_GAP=16 };
typedef struct {
    uint32_t sequence, now, age, baseline, hold;
    uint32_t reached_ms, released_ms, finish_ms, tx_errors, gaps;
    uint8_t state, result, fault, raw, qualified, valid, origin;
    uint8_t candidate, stable[4];
    bool have_sample;
} Engine;
void engine_init(Engine *e);
void engine_sample(Engine *e, uint32_t now, uint8_t raw);
void engine_fault(Engine *e, uint8_t fault);
void engine_tx_error(Engine *e);
bool engine_valid(const Engine *e);
#endif
