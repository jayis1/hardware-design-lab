/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef PF_ENGINE_H
#define PF_ENGINE_H
#include <stdint.h>
#include <stdbool.h>
#define QUALIFY_MS 5u
#define BASELINE_MS 100u
#define MAX_TRIAL_MS 30000u
#define ABSENT UINT32_MAX
enum State { WAIT, READY, ACTIVE, FAULT };
enum Reason { NONE, COMPLETE, ORDER, RECLOSE, TIMEOUT, GAP };
enum Fault { F_CONFIG=1, F_WATCHDOG=2, F_OVERFLOW=4 };
typedef struct {
    uint32_t id;
    uint32_t reason;
    uint32_t seen;
    uint32_t at[4];
    uint32_t duration;
} Result;
typedef struct {
    uint32_t now, last, start, baseline;
    uint32_t candidate_since[4];
    uint32_t at[4];
    uint32_t gaps, tx_errors, faults;
    uint8_t state, mask, candidate, known, seen;
    bool initialized, baseline_valid;
    Result result;
} Engine;
void engine_init(Engine *e);
void engine_sample(Engine *e, uint32_t now, uint8_t closed);
void engine_fault(Engine *e, uint32_t flag);
void engine_tx_error(Engine *e);
bool engine_valid(const Engine *e);
#endif
