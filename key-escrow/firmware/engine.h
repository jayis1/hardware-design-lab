/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef ESCROW_ENGINE_H
#define ESCROW_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
#define QUALIFY_MS 5u
#define BASELINE_MS 50u
#define DEADLINE_MS 60000u
#define LIMIT 1000000u
/* Low-active electrical inputs become high-active mechanical witnesses. */
enum { RETRACTED=1, EXTENDED=2, KEY=4, CLOSED=8 };
enum { WAIT, READY, ACTIVE, HALT };
enum { NONE, CYCLE, RETURNED, EARLY_KEY, BAD_OPEN, CONTRADICTORY,
       UNKEYED, OPEN_EXTEND, TIMEOUT, LOST, EXHAUSTED, HARDWARE };
enum { F_CONFIG=1, F_WATCHDOG=2, F_EXHAUST=4, F_INPUT=8, F_GAP=16 };
typedef struct {
    uint32_t sequence, now, starts, completed, canceled, aborted;
    uint32_t age, last_ms, baseline, tx_errors, gaps;
    uint8_t state, result, fault, raw, qualified, valid, opened;
    uint8_t candidate, stable[4];
    bool have_sample, previous_clean;
} Engine;
void engine_init(Engine *e);
void engine_sample(Engine *e, uint32_t now, uint8_t raw);
void engine_fault(Engine *e, uint8_t fault);
void engine_tx_error(Engine *e);
bool engine_valid(const Engine *e);
#endif
