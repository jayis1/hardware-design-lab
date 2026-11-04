/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef DOMINO_ENGINE_H
#define DOMINO_ENGINE_H
#include <stdint.h>
#include <stdbool.h>
#define ABSENT UINT32_MAX
#define BASELINE_MS 500u
#define QUIET_MS 250u
#define DEADLINE_MS 10000u
#define QUALIFY_SAMPLES 5u
enum Phase { WAIT, READY, ACTIVE, DONE, HALT };
enum Outcome { NONE, CLEAN_FULL, CLEAN_PARTIAL, DISTURBED_FULL, DISTURBED_PARTIAL, ABORTED };
enum Fault { F_NONE, F_GAP, F_OVERFLOW, F_CONFIG, F_WATCHDOG };
typedef struct {
    uint32_t now, samples, starts, completed, full, partial, disturbed, aborted;
    uint32_t tx_errors, age, baseline, quiet, offset[4];
    uint8_t phase, outcome, fault, raw, mask, valid, seen, rebound;
    uint8_t candidate[4], count[4];
    bool clocked, prior_up, prior_down;
} Engine;
void engine_init(Engine *e);
void engine_sample(Engine *e, uint32_t now, uint8_t raw);
void engine_fault(Engine *e, uint8_t reason);
void engine_tx_error(Engine *e);
bool engine_valid(const Engine *e);
#endif
