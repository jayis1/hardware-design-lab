/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef CASE_ENGINE_H
#define CASE_ENGINE_H
#include <stdint.h>
#include <stdbool.h>
#define LIMIT 1000000000u
#define QUALIFY 5u
#define WINDOW 100u
#define DEADLINE 600000u
/* Pocket A/B/C are bits 0..2; lid CLOSED is bit 3. */
enum { BASELINE, READY, ACTIVE, HALT };
enum { F_CONFIG=1, F_WATCHDOG=2, F_GAP=4, F_OVERFLOW=8, F_SEALED=16 };
typedef struct {
    uint32_t sequence, now, fault, starts, completed, aborted;
    uint32_t age, quiet, gaps, tx_errors, last_age;
    uint8_t raw, qualified, valid, candidate[4], run[4];
    uint8_t phase, reference, touched, absent, opened;
    uint8_t last_before, last_after, last_touched, last_returned;
    bool started;
} Engine;
void engine_init(Engine *e);
void engine_sample(Engine *e, uint32_t now, uint8_t raw);
void engine_fault(Engine *e, uint32_t fault);
void engine_tx_error(Engine *e);
bool engine_valid(const Engine *e);
#endif
