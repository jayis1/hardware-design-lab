/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef SWEEP_ENGINE_H
#define SWEEP_ENGINE_H
#include <stdint.h>
#include <stdbool.h>
enum { F_NONE, F_TIME, F_INPUT, F_OVERFLOW, F_CONFIG, F_WATCHDOG };
enum { WAIT_BASELINE, READY, OUTWARD, RETURNING, DONE, REJECTED, HALT };
enum { R_NONE, R_JUMP, R_REVERSAL, R_OVERSHOOT, R_GAP, R_TIMEOUT, R_FAULT };
#define SWEEP_STEPS 16u
#define SWEEP_TIMEOUT 60000u
/* Index zero is a qualified arbitrary baseline, not a physical home.
 * Two 17-bit planes preserve X/Y at every outward position. Return samples
 * cover positions 15..0 only; turnaround position 16 has no independent pair. */
typedef struct {
    uint32_t now, samples, gaps, missed, tx_errors;
    uint32_t starts, completed, abandoned, age;
    uint32_t outward_x, outward_y, return_x, return_y;
    uint32_t compared, mismatch_x, mismatch_y;
    uint8_t raw, qualified, valid, fault, state, reason, position;
    uint8_t direction, candidate, stable, started;
} Engine;
void engine_init(Engine *e);
void engine_sample(Engine *e, uint32_t now, unsigned raw);
void engine_fault(Engine *e, unsigned fault);
void engine_tx_error(Engine *e);
bool engine_valid(const Engine *e);
int engine_direction(unsigned old_phase, unsigned new_phase);
#endif
